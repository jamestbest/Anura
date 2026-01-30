//
// Created by jamestbest on 11/24/25.
//

#include "Parser.h"

#include "Buffer.h"
#include "Errors.h"

#include <stdio.h>
#include <string.h>

const ParseRet PARSE_RET_FAIL= (ParseRet) {.succ= false, .root= {0}};

static RootNode create_root();
static Vector create_children();

static AliasOrNode* add_alias_list();

Token* current();
Token* consume();
static Token* expect(TokenType type);
static Token* expect_keyword(keyword kw);
static Token* expect_binary_op(BinaryOperator op);
static void skip(TokenType type);
static Token* peek();
static Token* peer(uint distance);

static int parse_toplevel();

static int error(const char* message, ...);
static int error_with_token(const char* message, Token* tok, ...);
static int unexpected(const char* context, Token* unexpected_token);
static int unexpected_keyword(const char* context, keyword kw);

VECTOR_ADD(IdentNode, IdentNode)

TokenArray* tokens;
size_t t_i;

ParseRet parse(TokenArray* ts) {
    RootNode root= create_root();
    int ret_code= SUCCESS;

    tokens= ts;
    t_i= 0;

    while (t_i < tokens->pos) {
        int res;
        if (res= parse_toplevel(), res != SUCCESS) {
            ret_code= res;
            break;
        }
    }

    if (ret_code != SUCCESS) return PARSE_RET_FAIL;
    return (ParseRet) {
        .succ= true,
        .root= root
    };
}

int parse_toplevel() {
    Token* t= current();

    if (t->type != KEYWORD) {
        return error_with_token("Expected keyword at toplevel", t);
    }

    switch (t->data.keyword) {
        case ALIAS: return parse_alias();
        case CALCULATE: return parse_calc();
        case DATA: return parse_data();
        case FLAG: return parse_flag();
        case STRUCTURE: return parse_structure();
        default:
            return error_with_token("Expected keyword token to be in (ALIAS, CALCULATE, DATA, FLAG, STRUCTURE)",t);
    }
}

Node* parse_expr() {
    // operators: ==, !=, ., and, or
    // constants: Enums, int literals
    // keyword: default which is just True
    // identifiers: flags, aliases
}

int parse_if_rule(RuleVector* rules) {
    consume(); // eat the 'if'

    Node* expr= parse_expr();

    if (!expr) return FAIL;

    if (!expect_keyword(THEN)) {
        return unexpected_keyword("If rule after expression", current()->data.keyword);
    }

    Node* right= parse_right_rule();

    if (!right) return FAIL;

    skip(DELIMITER);

    Rule r;
    r.rule_if= create_alias_if();
    r.rule_if.condition= expr;
    r.rule_if.output= right;

    Rule_vec_add(rules, &r);

    return SUCCESS;
}

int parse_braced_rule(RuleVector* rules) {
    // Either an
    // L rule
    // or an LR rule
    // or an if rule
    // or a when rule

    Token* c= current();
    if (c->type == KEYWORD) {
        if (c->data.keyword == IF) return parse_if_rule(rules);
        if (c->data.keyword == WHEN) return parse_when_rule(rules);

        return unexpected_keyword("Braced rule, expected either IF, WHEN, or L/LR rule", current()->data.keyword);
    }

    return parse_l_or_lr_rule(rules);
}

int parse_l_or_lr_rule(RuleVector* rules) {
    LeftRule l_rule= parse_left_rule();

    if (expect_binary_op(EQUALITY)) {
        RightRule r_rule= parse_right_rule();

        Rule_vec_add(rules, (Rule){.rule_lr=
            {
                .base= {.type= NT_},
                .left= l_rule,
                .right= r_rule
            }
        });

        return SUCCESS;
    }

    Rule_vec_add(rules, (Rule) {
        .rule_l= {
            .base= {.type= NT_},
            .left= l_rule
        }
    });

    return SUCCESS;
}

int parse_braced_rules(BracedRules* results) {
    if (!expect(LBRACE)) {
        return unexpected("Braced rules statement after equality sign", current());
    }

    expect(DELIMITER);

    while (!expect(RBRACE)) {
        int res;
        if (res= parse_braced_rule(&results->rules), res != SUCCESS) {
            return res;
        }
    }

    if (results->rules.pos == 0) {
        return error("Cannot have an empty braced rules list");
    }

    return SUCCESS;
}

int parse_alias_list(Alias a_base) {
    //  ALIAS <ident>= {
    //   <IDENT>
    //   <IDENT>
    //   <IDENT> i.e. an alias list (just says on of them)

    AliasOrNode* alias_node= add_alias_list();

    alias_node->a_base= a_base;

    if (!expect_binary_op(EQUALITY)) {
        return unexpected("Alias list statement after alias identifier", current());
    }

    return parse_braced_rules(&alias_node->rules);
}

uint64_t convert_lit_num_to_base10(const struct LitNumData lit_num, const bool expecting_base10) {
    if (lit_num.explicit_base10) {
        return lit_num.base10;
    }

    if (expecting_base10) {
        return lit_num.base10;
    }

    return lit_num.base2.value;
}

int parse_alias() {
    consume(); // eat the ALIAS keyword

//    ALIAS_TYPE type;

    // ONE OF
    //  ALIAS <ident>= {
    //   <IDENT>
    //   <IDENT>
    //   <IDENT> i.e. an alias list (just says on of them)

    // OR
    //  ALIAS <IDENT> <SIZE>= {
    //     <fields> e.g. 0100 .w .r .x .b

    // OR
    //  ALIAS <IDENT> <SIZE>= {
    //     <LEFT RULE>= <RIGHT RULE>  e.g.   0xF0= LOCK

    Token* ident= expect(IDENTIFIER);

    if (!ident) {
        return unexpected("Expected identifier after alias keyword for alias name", current());
    }

    bool is_list= false;

    Token* size= expect(LIT_NUM);

    Alias a_base;
    if (size) {
        is_list= true;

        a_base.bits= convert_lit_num_to_base10(size->data.lit_num, true);
    } else {
        a_base.bits= -1;
    }

    a_base.identifier= ident->data.identifier;


    if (is_list) return parse_alias_list(a_base);

}

Token* current() {
    return Token_arr_ptr(tokens, t_i);
}

Token* peek() {
    return peer(1);
}

Token* peer(uint distance) {
    return Token_arr_ptr(tokens, t_i + distance);
}

Token* consume() {
    return Token_arr_ptr(tokens, t_i++);
}

Token* expect_keyword(keyword kw) {
    if (current()->type != KEYWORD || current()->data.keyword != kw) {
        return NULL;
    }

    return consume();
}

Token* expect_binary_op(BinaryOperator op) {
    if (current()->type != BINARY_OP || current()->data.bin_op != op) {
        return NULL;
    }

    return consume();
}

void skip(TokenType type) {
    expect(type);
}

Token* expect(TokenType type) {
    if (current()->type != type) {
        return NULL;
    }

    return consume();
}

int unexpected_keyword(const char* context, keyword kw) {
    printf("<<ERROR>> unexpeceted keyword in %s got %s\n", context, keyword_string(kw));

    return FAIL;
}

int unexpected(const char* context, Token* unexpected_token) {
    printf("<<ERROR>> Unexpected token in %s got", context);
    print_token(unexpected_token);
    printf("\n");

    return FAIL;
}

int error_with_token(const char* message, Token* tok, ...) {
    va_list args;

    va_start(args, tok);
    printf("<<ERROR>> AT ");
    print_token(tok);
    printf(" :");
    vprintf(message, args);
    va_end(args);

    return FAIL;
}

int error(const char* message, ...) {
    va_list args;

    va_start(args, message);
    printf("<<ERROR>>: ");
    vprintf(message, args);
    va_end(args);

    return FAIL;
}

uint8_t* node_buffer;
size_t node_buffer_capacity;
size_t node_buffer_size;

#define MIN_NODE_BUFFER_SIZE 10 // [[todo]]: change to ~500 after testing

void create_node_buffer() {
    node_buffer= calloc(1, MIN_NODE_BUFFER_SIZE);
    node_buffer_size= 0;
    node_buffer_capacity= MIN_NODE_BUFFER_SIZE;
}

void resize_node_buffer() {
    void* new_ptr= realloc(node_buffer, node_buffer_capacity << 1);

    if (!new_ptr) {
        error("Unable to realloc node buffer, this may be because of my `<< 1` on full buffer :)");

        node_buffer= NULL; // push the problem down the line :)
        return;
    }

    node_buffer= new_ptr;
    node_buffer_capacity <<= 1;
}

void* add_node(void* node_ptr, size_t node_bytes) {
    if (node_buffer_size + node_bytes > node_buffer_capacity) {
        resize_node_buffer();
    }

    void* res= node_buffer + node_buffer_size;

    memcpy(res, node_ptr, node_bytes);
    node_buffer_size += node_bytes;

    return res;
}

RootNode create_root() {
    return (RootNode) {
        .base= {
            .type= NT_ROOT
        },
        .child_nodes= create_children()
    };
}

AliasOrNode* add_alias_list() {
    AliasOrNode alias= (AliasOrNode) {
        .a_base= {
            .base= {
                .type= NT_ALIAS_OR_STMT
            },
            .identifier= NULL,
            .bits= -1
        },
        .rules= Rule_vec_create()
    };

    return add_node(&alias, sizeof(alias));
}

LitNode* add_lit_node(NodeType lit_type) {
    LitNode lit= (LitNode) {
        .base= {
            .type= lit_type
        },
        .data= NULL,
        .token= {0}
    };

    return add_node(&lit, sizeof(lit));
}

Vector create_children() {
    return vector_create();
}

void fprint_simple_num(FILE* file, const SimpleNumData* num) {
    if (num->show_as_bin) {
        fprintf(file, "0b");
        for (int i = 0; i < num->bits; ++i) {
            fprintf(file, "%c", num->value >> i & 1 ? '1' : '0');
        }
    } else {
        fprintf(file, "%#lx", num->value);
    }
}

