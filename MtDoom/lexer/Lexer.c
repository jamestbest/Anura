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

Buffer line_buff;
Vector lines;

ARRAY_ADD(Token, Token)
TokenArray tokens;

char* c_char;
char* line_start_char;
const char* c_filepath;
FILE* c_file;

static bool load_line();
static int  lex_line();

static void lex_comment();
static int  lex_string();
static void lex_identifier();
static int  lex_number();

static char peek();
static char current();
static char consume();
static char prev();
static void inc_line_and_reset_col();

static int error(const char* message, ...);

uint32_t line=0, col= 0;

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

    int retcode= SUCCESS;
    while (load_line()) {
        const int errcode= lex_line();

        if (errcode != SUCCESS) retcode= errcode;
    }

    buffer_destroy(&line_buff);

    for (int i= 0; i < tokens.pos; ++i) {
        Token* t= Token_arr_ptr(&tokens, i);
        print_token(t);
    }

    if (retcode == SUCCESS) {
        return (LexRet) {
            .succ= retcode,
            .tokens= tokens
        };
    }
    else return LEX_RET_FAIL;
}

bool load_line() {
    inc_line_and_reset_col();

    const bool succ= get_line(c_file, &line_buff);

    if (succ) {
        c_char= line_buff.data;
        line_start_char= c_char;
        vector_add(&lines, buffer_steal(&line_buff, BUFF_MIN));
    }

    return succ;
}

TokenMeta token_meta() {
    return (TokenMeta) {
        .line= line,
        .col= col
    };
}

void add_simple_token(TokenType type) {
    Token t= (Token) {
        .type= type,
        .meta= token_meta()
    };

    Token_arr_add(&tokens, t);
}

int lex_line() {
    int retcode= SUCCESS;

    while (c_char < line_buff.data + line_buff.pos && *c_char != '\0') {
        char c= *c_char;

        switch (c) {
            case '\n': add_simple_token(DELIMITER); break;
            case ' ':
            case '\t':
                break;
            case '=': {
                if (peek() == '=') {
                    add_simple_token(EQUALITY);
                    consume();
                    break;
                }
                add_simple_token(ASSIGN);
                break;
            }
            case '(': add_simple_token(LPAREN); break;
            case ')': add_simple_token(RPAREN); break;
            case '{': add_simple_token(LBRACE); break;
            case '}': add_simple_token(RBRACE); break;
            case '.': add_simple_token(DOT); break;
            case ',': add_simple_token(COMMA); break;
            case '*': add_simple_token(STAR); break;
            case '?': add_simple_token(QUESTION); break;
            case '|': add_simple_token(PIPE); break;
            case '^': add_simple_token(POW); break;
            case '!': {
                if (peek() != '=') goto lex_line_no_simple;

                add_simple_token(NEQ);
                break;
            }
            default: goto lex_line_no_simple;
        }

        consume();
        continue;

    lex_line_no_simple:
        if (c == '/' && peek() == '/') lex_comment();
        else if (c == '"') lex_string();
        else {
            if (is_alph(c)) lex_identifier();
            else if (is_digit(c)) lex_number();
            else {
                error("Unknown symbol found when lexing `%c`", c);
                retcode= FAIL;
                consume();
            }
        }
    }

    return retcode;
}

void lex_comment() {
    c_char= line_buff.data + line_buff.pos;
}

int lex_string() {
    TokenMeta meta= token_meta();

    consume(); // '"'

    const char* start= c_char;
    const char* end= NULL;

    while (
        current() != '\0' &&
        !(
            current() == '"' &&
            prev() != '\\'
        )){
        consume();
    }

    if (current() != '"')
        return error("String literal does not have end before newline");

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

    Token_arr_add(&tokens, t);

    consume(); // eat the last '"'

    return SUCCESS;
}

const char* KEYWORD_STRINGS[KEYWORD_COUNT]= {
    [ALIAS]= "ALIAS",
    [STRUCTURE]= "STRUCTURE",
    [FLAG]= "FLAG",
    [DEFAULT]= "DEFAULT",
    [BYTE]= "BYTE",
    [BYTES]= "BYTES",
    [BIT]= "BIT",
    [BITS]= "BITS",
    [LEFT]= "LEFT",
    [RIGHT]= "RIGHT",
    [IF]= "IF",
    [THEN]= "THEN",
    [WHEN]= "WHEN",
    [CALCULATE]= "CALCULATE",
    [DATA]= "DATA"
};

const char* TOKEN_TYPE_STRS[TOKEN_TYPE_COUNT]= {
    [KEYWORD]= "KEYWORD",
    [EQUALITY]= "EQUALITY",
    [ASSIGN]= "ASSIGN",
    [IDENTIFIER]= "IDENTIFIER",
    [LBRACE]= "LBRACE",
    [RBRACE]= "RBRACE",
    [DELIMITER]= "DELIMITER",
    [COMMENT]= "COMMENT",
    [DOT]= "DOT",
    [LPAREN]= "LPAREN",
    [RPAREN]= "RPAREN",
    [LIT_NUM]= "LIT_NUM",
    [LIT_STRING]= "LIT_STRING",
    [COMMA]= "COMMA",
    [NEQ]= "NEQ",
    [STAR]= "STAR",
    [QUESTION]= "QUESTION",
    [PIPE]= "PIPE",
    [POW]= "POW"
};

int identifier_comp(const void* ppa, const void* ppb) {
    const char* pa= *(const char**)ppa;
    const char* pb= *(const char**)ppb;

    return strcasecmp(pa, pb);
}

void lex_identifier_from(const char* start) {
    int col_diff= start - line_start_char;
    col= col_diff;
    TokenMeta meta= token_meta();

    c_char= start;
    const char* end;

    while (is_alph_numeric(current())) {
        consume();
    }
    end= c_char;

    size_t size= end - start;
    size_t bytes= size + 1;
    char* identifier= malloc(bytes);
    memcpy(identifier, start, size);
    identifier[bytes - 1]= '\0';

    void* res= bsearch(&identifier, KEYWORD_STRINGS, KEYWORD_COUNT, sizeof(KEYWORD_STRINGS[0]), identifier_comp);

    if (res) {
        uint pos= (const char**)res - KEYWORD_STRINGS;
        keyword k= pos;

        Token t= (Token) {
            .type= KEYWORD,
            .meta= meta,
            .data.keyword= k
        };

        free(identifier);

        Token_arr_add(&tokens, t);
    } else {
        Token t= (Token) {
            .type= IDENTIFIER,
            .meta= meta,
            .data.identifier= identifier
        };

        Token_arr_add(&tokens, t);
    }
}

void lex_identifier() {
    lex_identifier_from(c_char);
}

int lex_number() {
    // we parse as both a base 2 and base 10 and base 16 if needed
    // this is because the default changes
    TokenMeta meta= token_meta();

    const char* start= c_char;
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
        lex_identifier_from(start);
        return SUCCESS;
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
    long long base10res= strtoll(start, &strtoll_end10, 0);
    if (
        strtoll_end10 == start ||
        base10res == LLONG_MAX ||
        base10res == LLONG_MIN ||
        errno == ERANGE ||
        strtoll_end10 != end
    ) {
        return error("Failed to parse string `%s` into a number via strtoll", start);
    }

    *(c_char + 1)= save;

    TokenData data= (TokenData) {
        .lit_num= {
            .explicit_base10= explicit_base10,
            .base2= {
                .value= base2res,
                .digits= strtoll_end2 - start
            },
            .base10= base10res
        }
    };

    Token t= (Token) {
        .type= LIT_NUM,
        .meta= meta,
        .data= data
    };

    Token_arr_add(&tokens, t);

    return SUCCESS;
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

void inc_line_and_reset_col() {
    line++;
    col= 0;
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

void print_token(Token* token) {
    printf("Token (%.2u:%.2u) %s {", token->meta.line, token->meta.col, TOKEN_TYPE_STRS[token->type]);

    switch (token->type) {
        case LIT_NUM: {
            struct LitNumData data= token->data.lit_num;
            if (data.explicit_base10) {
                printf("BASE 10 (EXPL): %ld (%lx)", data.base10, data.base10);
            } else {
                printf(
                    "BASE 10: %ld (%lx)   BASE 2: %ld WITH %d digits",
                    data.base10,
                    data.base10,
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
    }

    printf("}\n");
}
