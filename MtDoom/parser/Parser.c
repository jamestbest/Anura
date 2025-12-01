//
// Created by jamestbest on 11/24/25.
//

#include "Parser.h"

#include "Errors.h"

#include <stdio.h>

const ParseRet PARSE_RET_FAIL= (ParseRet) {.succ= false, .root= {0}};

static RootNode create_root();
static Vector create_children();

static Token* current();
static Token* consume();
static Token* peek();
static Token* peer(uint distance);

static int parse_toplevel();

static int error(const char* message, ...);
static int error_with_token(const char* message, Token* tok, ...);

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

//    switch (t->data.keyword) {
//        case ALIAS: return parse_alias();
//        case CALCULATE: return parse_calc();
//        case DATA: return parse_data();
//        case FLAG: return parse_flag();
//        case STRUCTURE: return parse_structure();
//        default:
//            return error_with_token("Expected keyword token to be in (ALIAS, CALCULATE, DATA, FLAG, STRUCTURE)",t);
//    }
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

RootNode create_root() {
    return (RootNode) {
        .base= {
            .type= NT_ROOT
        },
        .child_nodes= create_children()
    };
}

Vector create_children() {
    return vector_create();
}
