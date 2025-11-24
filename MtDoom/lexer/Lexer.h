//
// Created by james on 20/11/25.
//

#ifndef LEXER_H
#define LEXER_H
#include <stdint.h>

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
    COMMA,
    NEQ,
    STAR,
    QUESTION,
    PIPE,
    POW,
} TokenType;

typedef enum keyword {
    ALIAS,
    STRUCTURE,
    FLAG,
    DEFAULT,
    BYTE,
    BIT,
    LEFT,
    RIGHT,
    IF,
    THEN,
    WHEN,
    CALCULATE,
    DATA
} keyword;

typedef union TokenData {
    uint64_t lit_num;
    keyword keyword;
    const char* identifier;
} TokenData;

typedef struct Token {
    TokenType type;
    TokenData data;
} Token;

#endif //LEXER_H
