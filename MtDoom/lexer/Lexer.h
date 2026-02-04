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
    DATA,
    DEFAULT,
    FLAG,
    IF,
    LEFT,
    META,
    RIGHT,
    STRUCTURE,
    THEN,
    WHEN, // CURRENTLY SEMANTICALLY THE SAME AS IF
    KEYWORD_COUNT
} keyword;

typedef struct BaseNumInfo {
    uint8_t digits;
    uint64_t value;
} BaseNumInfo;

typedef union TokenData {
    struct LitNumData {
        bool explicit_base10;
        BaseNumInfo base2;
        BaseNumInfo base10;
    } lit_num;
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

LexRet lex(const char* filepath);
void print_token(Token* token);
const char* keyword_string(keyword kw);

void fprint_lit_num(FILE* file, const struct LitNumData* num);

extern const LexRet LEX_RET_FAIL;

#endif //LEXER_H
