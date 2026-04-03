//
// Created by james on 20/11/25.
//

#include "Lexer.h"

#include "Errors.h"
#include "shared/Array.h"
#include "shared/Buffer.h"
#include "shared/Helper_File.h"
#include "shared/Vector.h"

#include <limits.h>
#include <string.h>
#include <math.h>

Buffer line_buff;
Vector lines;

ARRAY_ADD(Token, Token)
static TokenArray tokens;

char* c_char;
char* line_start_char;
const char* c_filepath;
FILE* c_file;

static bool load_line();
static int  lex_line();

static TokenRet lex_comment();
static TokenRet lex_string();
static TokenRet lex_identifier();
static TokenRet lex_number();

static char peek();
static char current();
static char consume();
static void unconsume();
static char prev();
static void inc_line_and_reset_col();

static int error(const char* message, ...);

uint32_t line=0, col= 1;

const LexRet LEX_RET_FAIL= (LexRet) {
    .succ= false,
    .tokens= {0}
};

LexRet lex(const char* filepath) {
    c_filepath= filepath;

    c_file= fopen(c_filepath, "r");

    if (!c_file) {
        error("Unable to open file");
        return LEX_RET_FAIL;
    }

    line_buff= buffer_create(BUFF_MIN);
    lines= vector_create();

    tokens= Token_arr_create();

    int retsucc= true;
    while (load_line()) {
        const int errcode= lex_line();

        if (!errcode)
            retsucc= errcode;
    }

    buffer_destroy(&line_buff);

    for (int i= 0; i < tokens.pos; ++i) {
        Token* t= Token_arr_ptr(&tokens, i);
        print_token(t);
    }

    if (retsucc) {
        return (LexRet) {
            .succ= retsucc,
            .tokens= tokens
        };
    }

    return LEX_RET_FAIL;
}

bool load_line() {
    inc_line_and_reset_col();

    const bool succ= get_line(c_file, &line_buff);

    if (succ) {
        c_char= line_buff.data;
        line_start_char= c_char;
        vector_add(&lines, buffer_copy(&line_buff));
    }

    return succ;
}

TokenMeta token_meta() {
    return (TokenMeta) {
        .line= line,
        .col= col
    };
}

void add_unary_op(UnaryOperator op) {
    Token t= (Token) {
        .type= UNARY_OP,
        .meta= token_meta(),
        .data.unary_op= op
    };

    Token_arr_add(&tokens, t);
}

void add_binary_op(BinaryOperator type) {
    Token t= (Token){
        .type= BINARY_OP,
        .meta= token_meta(),
        .data.bin_op= type
    };

    Token_arr_add(&tokens, t);
}

void add_simple_token(TokenType type) {
    Token t= (Token) {
        .type= type,
        .meta= token_meta()
    };

    Token_arr_add(&tokens, t);
}

Token create_simple_token(TokenType type) {
    return (Token) {
        .type= type,
        .meta= token_meta()
    };
}

Token create_binary_op(BinaryOperator type) {
    return (Token){
        .type= BINARY_OP,
        .meta= token_meta(),
        .data.bin_op= type
    };
}

Token create_unary_op(UnaryOperator op) {
    return (Token) {
        .type= UNARY_OP,
        .meta= token_meta(),
        .data.unary_op= op
    };
}

void set_lex_pos(char* str) {
    c_char= str;
    col= 0;
}

TokenRet lex_token() {
    char c= consume();

    switch (c) {
        case '\n': return TOK_SUCC(create_simple_token(DELIMITER));
        case ' ':
        case '\t':
            return TOK_SUCC_HIDDEN();
        case '=': {
            if (current() == '=') {
                consume();
                return TOK_SUCC(create_binary_op(EQUALITY));
            }
            return TOK_SUCC(create_simple_token(ASSIGN));
        }
        case '(': return TOK_SUCC(create_simple_token(LPAREN));
        case ')': return TOK_SUCC(create_simple_token(RPAREN));
        case '{': return TOK_SUCC(create_simple_token(LBRACE));
        case '}': return TOK_SUCC(create_simple_token(RBRACE));
        case '.': return TOK_SUCC(create_binary_op(DOT));
        case ',': return TOK_SUCC(create_simple_token(COMMA));
        case '*': return TOK_SUCC(create_binary_op(STAR));
        case '?': return TOK_SUCC(create_unary_op(EXISTS));
        case '^': return TOK_SUCC(create_binary_op(POW));
        case '|': return TOK_SUCC(create_binary_op(PIPE));
        case '_': return TOK_SUCC(create_simple_token(UNDERSCORE));
        case '~': return TOK_SUCC(create_simple_token(READINVERT));
        case '+': return TOK_SUCC(create_binary_op(ADD));
        case '-': return TOK_SUCC(create_binary_op(SUB));
        case '&': {
            if (current() == '&') {
                consume();
                return TOK_SUCC(create_binary_op(AND));
            }
        }
        case '!': {
            if (current() != '=') {
                return TOK_SUCC(create_unary_op(NOT));
            }

            consume();
            return TOK_SUCC(create_binary_op(NEQUALITY));
        }
        default: break;
    }

    unconsume();

    if (c == '/' && current() == '/') return lex_comment();
    if (c == '"') return lex_string();
    if (is_alph(c)) return lex_identifier();
    if (is_digit(c)) return lex_number();

    error("Unknown symbol found when lexing `%c` at <%u:%u>", c, line, col);
    consume();
    return TOK_FAIL;
}

int lex_line() {
    int succ= true;

    while (c_char < line_buff.data + line_buff.pos && *c_char != '\0') {
        const TokenRet res= lex_token();
        succ= succ && res.succ;

        if (res.addable)
            Token_arr_add(&tokens, res.token);
    }

    return succ;
}

TokenRet lex_comment() {
    c_char= line_buff.data + line_buff.pos;
    return TOK_SUCC_HIDDEN();
}

TokenRet lex_string() {
    TokenMeta meta= token_meta();

    // the stream will have
    // RAW_STRING
    // EXPR 1's tokens
    // DELIMITER
    // EXPR 2's tokens
    // DELIMITER
    // ...

    consume(); // '"'

    const char* start= c_char;
    const char* end= NULL;

    bool open_brace= false;
    char c= current();
    while (
        c != '\0' &&
        !(
            c == '"' &&
            prev() != '\\'
        )){
        if (c == '{') {
            if (open_brace) {
                error("Brace in string opened before previous was closed");
                return TOK_FAIL;
            }
            open_brace= true;
        }
        if (c == '}') {
            if (!open_brace) {
                error("Brace in string closed before opened");
                return TOK_FAIL;
            }
            open_brace= false;
        }
        consume();
        c=current();
    }

    if (current() != '"') {
        error("String literal does not have end before newline");
        return TOK_FAIL;
    }

    end= c_char - 1;
    size_t size= (end - start + 1);
    size_t bytes= size + 1;
    char* string= malloc(bytes);
    memcpy(string, start, size);
    string[bytes - 1]= '\0';

    Token t= (Token) {
        .type= LIT_STRING,
        .meta= meta,
        .data= (TokenData) {
            .lit_string= string
        }
    };

    consume(); // eat the last '"'



    return TOK_SUCC(t);
}

const char* KEYWORD_STRINGS[KEYWORD_COUNT]= {
    [ALIAS]= "ALIAS",
    [STRUCTURE]= "STRUCTURE",
    [FLAG]= "FLAG",
    [FLAT]= "FLAT",
    [DEFAULT]= "DEFAULT",
    [BYTE]= "BYTE",
    [BYTES]= "BYTES",
    [BIT]= "BIT",
    [BITS]= "BITS",
    [LEFT]= "LEFT",
    [META]= "META",
    [MAP]= "MAP",
    [ON]= "ON",
    [OF]= "OF",
    [RIGHT]= "RIGHT",
    [RULE]= "RULE",
    [IF]= "IF",
    [THEN]= "THEN",
    [WHEN]= "WHEN",
    [WITH]= "WITH",
    [CHOOSE]= "CHOOSE",
    [CALCULATE]= "CALCULATE",
    [DATA]= "DATA",
    [VAR]= "VAR"
};

const char* keyword_string(keyword kw) {
    return KEYWORD_STRINGS[kw];
}

const char* TOKEN_TYPE_STRS[TOKEN_TYPE_COUNT]= {
    [KEYWORD]= "KEYWORD",
    [ASSIGN]= "ASSIGN",
    [IDENTIFIER]= "IDENTIFIER",
    [LBRACE]= "LBRACE",
    [RBRACE]= "RBRACE",
    [DELIMITER]= "DELIMITER",
    [UNDERSCORE]= "UNDERSCORE",
    [READINVERT]= "READ INVERTED",
    [COMMENT]= "COMMENT",
    [LPAREN]= "LPAREN",
    [RPAREN]= "RPAREN",
    [LIT_NUM]= "LIT_NUM",
    [LIT_STRING]= "LIT_STRING",
    [COMMA]= "COMMA",
    [UNARY_OP]= "UNARY OP",
    [BINARY_OP]= "BINARY OP"
};

const char* BINARY_OP_STRINGS[BINARY_OP_COUNT]= {
    [EQUALITY]= "EQUALITY (==)",
    [NEQUALITY]= "NEQUALITY (!=)",
    [DOT]= "DOT (.)",
    [STAR]= "STAR (*)",
    [POW]= "POW (**)",
    [PIPE]="PIPE (|)",
    [AND]="AND (&&)",
    [ADD]= "ADD (+)",
    [SUB]= "SUB (-)"
};

const char* UNARY_OP_STRINGS[UNARY_OP_COUNT]= {
    [NOT]= "NOT (!)",
    [EXISTS]= "EXISTS (?)"
};

int identifier_comp(const void* ppa, const void* ppb) {
    const char* pa= *(const char**)ppa;
    const char* pb= *(const char**)ppb;

    return strcasecmp(pa, pb);
}

keyword map_keyword(keyword kw) {
    switch (kw) {
        case BIT:
        case BITS:
            return BITS;
        case BYTE:
        case BYTES:
            return BYTES;
        case WHEN:
        case IF:
            return IF;
        default:
            return kw;
    }
}

TokenRet lex_identifier_from(char* start) {
    size_t col_diff= start - line_start_char;
    col= col_diff;
    TokenMeta meta= token_meta();

    c_char= start;
    const char* end;

    while (is_alph_numeric(current()) || current() == '_') {
        consume();
    }
    end= c_char;

    if (*(end-1) == '_') {
        error("Identifiers cannot end with `_`");
        return TOK_FAIL;
    }

    const size_t size= end - start;
    const size_t bytes= size + 1;
    char* identifier= malloc(bytes);
    memcpy(identifier, start, size);
    identifier[bytes - 1]= '\0';

    void* res= bsearch(&identifier, KEYWORD_STRINGS, KEYWORD_COUNT, sizeof(KEYWORD_STRINGS[0]), identifier_comp);

    Token t;
    if (res) {
        uint pos= (const char**)res - KEYWORD_STRINGS;
        keyword k= pos;

        t= (Token) {
            .type= KEYWORD,
            .meta= meta,
            .data.keyword= map_keyword(k)
        };

        free(identifier);
    } else {
        t= (Token) {
            .type= IDENTIFIER,
            .meta= meta,
            .data.identifier= identifier
        };
    }

    return TOK_SUCC(t);
}

TokenRet lex_identifier() {
    return lex_identifier_from(c_char);
}

TokenRet lex_number() {
    // we parse as both a base 2 and base 10 and base 16 if needed
    // this is because the default changes
    TokenMeta meta= token_meta();

    char* start= c_char;
    const char* end;

    bool explicit_base10= false;
    if (current() == '0' && char_lower(peek()) == 'x') {
        consume();
        consume();
        explicit_base10= true;
    }

    while (is_digit_base(current(), 16)) {
        consume();
    }

    if (is_alph(current())) {
        // this is actually an identifier
        return lex_identifier_from(start);
    }

    end= c_char;
    char save= peek();
    *(c_char+1)= '\0';

    char* strtoll_end2;
    long long base2res= strtoll(start, &strtoll_end2, 2);
    if (
        strtoll_end2 == start ||
        base2res == LLONG_MAX ||
        base2res == LLONG_MIN ||
        errno == ERANGE ||
        strtoll_end2 != end
    ) {
        base2res= -1;
    }

    char* strtoll_end10;
    bool is_base16= explicit_base10;
    long long base10res= strtoll(start, &strtoll_end10, 0);
    if (
        strtoll_end10 == start ||
        base10res == LLONG_MAX ||
        base10res == LLONG_MIN ||
        errno == ERANGE ||
        strtoll_end10 != end
    ) {
        error("Failed to parse string `%s` into a number via strtoll", start);
        return TOK_FAIL;
    }

    *(c_char + 1)= save;

    TokenData data= (TokenData) {
        .lit_num= {
            .explicit_base10= explicit_base10,
            .base2= {
                .value= base2res,
                .digits= strtoll_end2 - start
            },
            .base10= { //[[todo]] check this
                .value= base10res,
                .digits= is_base16 ? (strtoll_end10 - start) << 2 :
                                     base10res > 0 ? (int)floor(log2((double)base10res)) + 1 :
                                                     1
            }
        }
    };

    const Token t= (Token) {
        .type= LIT_NUM,
        .meta= meta,
        .data= data
    };

    return TOK_SUCC(t);
}

char peek() {
    if (c_char == line_buff.data + line_buff.pos - 1) return '\0';
    return *(c_char + 1);
}

char prev() {
    if (c_char == line_buff.data) return '\0';
    return *(c_char - 1);
}

char current() {
    return *c_char;
}

char consume() {
    col++;
    return *(c_char++);
}

void unconsume() {
    col--;
    c_char--;
}

void inc_line_and_reset_col() {
    line++;
    col= 1;
}

int error(const char* message, ...) {
    va_list args;
    va_start(args, message);
    printf("<<ERROR>> (%u:%u): ", line, col);
    vprintf(message, args);
    newline();
    va_end(args);

    return FAIL;
}

void fprint_lit_num(FILE* file, const struct LitNumData* num) {
    if (num->explicit_base10) {
        fprintf(file, "%#lx", num->base10.value);
    } else {
        fprintf(file, "0b");
        for (int i = 0; i < num->base2.digits; ++i) {
            fprintf(file, "%c", num->base2.value >> i & 1 ? '1' : '0');
        }
    }
}

void print_token(Token* token) {
    if (!token) {
        printf("<<NULL TOKEN>>");
        return;
    }

    printf("Token (%.2u:%.2u) %s {", token->meta.line, token->meta.col, TOKEN_TYPE_STRS[token->type]);

    switch (token->type) {
        case LIT_NUM: {
            const struct LitNumData data= token->data.lit_num;
            if (data.explicit_base10) {
                printf("BASE 10 (EXPL): %ld (%lx)", data.base10.value, data.base10.value);
            } else {
                printf(
                    "BASE 10: %ld (%lx)   BASE 2: %ld WITH %d digits",
                    data.base10.value,
                    data.base10.value,
                    data.base2.value,
                    data.base2.digits
                );
            }
            break;
        }
        case LIT_STRING: {
            printf("\"%s\"", token->data.lit_string);
            break;
        }
        case IDENTIFIER: {
            printf("%s", token->data.identifier);
            break;
        }
        case KEYWORD: {
            printf("%s", KEYWORD_STRINGS[token->data.keyword]);
            break;
        }
        case UNDERSCORE: {
            printf("`_`");
            break;
        }
        case READINVERT: {
            printf("`~`");
            break;
        }

        case ASSIGN:
        case LBRACE:
        case RBRACE:
        case DELIMITER:
        case COMMENT:
        case LPAREN:
        case RPAREN:
        case COMMA:
            break;

        case BINARY_OP:
            printf("`%s`", BINARY_OP_STRINGS[token->data.bin_op]);
            break;

        case UNARY_OP:
            printf("`%s`", UNARY_OP_STRINGS[token->data.unary_op]);
            break;

        default:
            assert(false);
    }

    printf("}\n");
}
