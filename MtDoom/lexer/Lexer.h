//
// Created by james on 20/11/25.
//

#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "shared/Array.h"

typedef enum TokenType {
    KEYWORD,
    ASSIGN,
    IDENTIFIER,
    LBRACE,
    RBRACE,
    DELIMITER,
    UNDERSCORE,
    READINVERT,
    COMMENT,
    LPAREN,
    RPAREN,
    LIT_NUM,
    LIT_STRING,
    COMMA,
    BINARY_OP,
    UNARY_OP,
    TOKEN_TYPE_COUNT
} TokenType;

typedef enum BinaryOperator {
    EQUALITY,
    NEQUALITY,
    DOT,
    STAR,
    POW,
    PIPE,
    AND,
    BINARY_OP_COUNT
} BinaryOperator;

typedef enum UnaryOperator {
    NOT, // !
    EXISTS, // ?
    UNARY_OP_COUNT
} UnaryOperator;

typedef enum keyword {
    ALIAS,
    BIT, // DOES NOT EXIST PAST LEXER
    BITS,
    BYTE, // DOES NOT EXIST PAST LEXER
    BYTES,
    CALCULATE,
    CHOOSE,
    DATA,
    DEFAULT,
    FLAG,
    FLAT,
    IF,
    LEFT,
    MAP,
    META,
    OF,
    ON,
    RIGHT,
    RULE,
    STRUCTURE,
    THEN,
    VAR,
    WHEN, // CURRENTLY SEMANTICALLY THE SAME AS IF
    WITH,
    KEYWORD_COUNT
} keyword;

typedef struct BaseNumInfo {
    uint8_t digits;
    uint64_t value;
} BaseNumInfo;

typedef struct LitNumData {
    bool explicit_base10;
    BaseNumInfo base2;
    BaseNumInfo base10;
} LitNumData;

typedef union TokenData {
    LitNumData lit_num;
    keyword keyword;
    BinaryOperator bin_op;
    UnaryOperator unary_op;
    const char* identifier;
    char* lit_string;
} TokenData;

typedef struct TokenMeta {
    uint16_t line;
    uint16_t col;
} TokenMeta;

typedef struct Token {
    TokenType type;
    TokenMeta meta;
    TokenData data;
} Token;

ARRAY_PROTO(Token, Token)

typedef struct LexRet {
    bool succ;
    TokenArray tokens;
} LexRet;

typedef struct TokenRet {
    bool succ;
    bool addable;
    Token token;
} TokenRet;

#define TOK_SUCC(tok) (TokenRet){.succ=true, .addable=true, .token=tok}
#define TOK_SUCC_HIDDEN() (TokenRet){.succ=true, .addable=false, .token={0}}
#define TOK_FAIL (TokenRet){.succ=false, .token={0}}

LexRet lex(const char* filepath);
TokenRet lex_token();
void set_lex_pos(char* str);
void print_token(Token* token);
const char* keyword_string(keyword kw);

void fprint_lit_num(FILE* file, const struct LitNumData* num);

extern const LexRet LEX_RET_FAIL;
extern const char* BINARY_OP_STRINGS[BINARY_OP_COUNT];
extern const char* UNARY_OP_STRINGS[UNARY_OP_COUNT];

#endif //LEXER_H
