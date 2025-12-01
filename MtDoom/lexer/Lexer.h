//
// Created by james on 20/11/25.
//

#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>
#include <stdbool.h>
#include "shared/Array.h"

typedef enum TokenType {
    KEYWORD,
    EQUALITY,
    ASSIGN,
    IDENTIFIER,
    LBRACE,
    RBRACE,
    DELIMITER,
    COMMENT,
    DOT,
    LPAREN,
    RPAREN,
    LIT_NUM,
    LIT_STRING,
    COMMA,
    NEQ,
    STAR,
    QUESTION,
    PIPE,
    POW,
    TOKEN_TYPE_COUNT
} TokenType;

typedef enum keyword {
    ALIAS,
    BIT,
    BITS,
    BYTE,
    BYTES,
    CALCULATE,
    DATA,
    DEFAULT,
    FLAG,
    IF,
    LEFT,
    RIGHT,
    STRUCTURE,
    THEN,
    WHEN,
    KEYWORD_COUNT
} keyword;

typedef union TokenData {
    struct LitNumData {
        bool explicit_base10;
        struct Base2NumInfo {
            uint8_t digits;
            uint64_t value;
        } base2;
        uint64_t base10;
    } lit_num;
    keyword keyword;
    const char* identifier;
    const char* lit_string;
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

extern const LexRet LEX_RET_FAIL;

#endif //LEXER_H
