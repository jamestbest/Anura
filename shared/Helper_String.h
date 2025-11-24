//
// Created by jamescoward on 11/11/2023.
//

#ifndef ATOMIC_HELPER_STRING_H
#define ATOMIC_HELPER_STRING_H

#include "../Commons.h"
#include <stdio.h>

typedef unsigned int uint;

#define SPACE_C ' '
#define NUM_DIGITS 10
#define NUM_ALPH 26 //just in case :)

#define ASCII_ALPH_LOW_MIN 97
#define ASCII_ALPH_CAP_MIN 65
#define ASCII_DIGIT_MIN 48

typedef struct LenSize {
    uint32_t len; //glyph count
    uint32_t size; //byte count
} LenSize;

typedef struct {
    uint32_t value;
    uint bytes;
} UTF8Pos;

typedef struct marked_string {
    char* str;
    bool is_heap;
} marked_string;

int find_last(const char* string, char pattern);
int starts_with_ips(const char* string, const char* pattern);
int starts_with(const char* string, const char* pattern);
int starts_with_ic(const char* string, const char* pattern);
int word_match_alphnumeric(const char* word, const char* match);
bool str_eq(const char* stra, const char* strb);

char char_lower_unsafe(char a);
char char_lower(char a);

uint len(const char* string);
uint len_with(const char* string, uint offset);
uint len_from(const char* string, uint offset);
uint len_from_to(const char* string, uint offset, char to);

LenSize len_size(const char* string);

bool str_contains(const char* str, uint from, uint to, char c);

char* str_cpy(const char* string);
char* str_cpy_replace(const char* string, char find, char replace);

bool is_digit(uint32_t a);
bool is_digit_base(uint32_t a, uint base);
bool is_alph_cap(uint32_t a);
bool is_alph_low(uint32_t a);
bool is_alph(uint32_t a);
bool is_alph_numeric(uint32_t a);
bool is_whitespace(uint32_t a);
bool is_newline(uint32_t a);

void fputz(FILE* file, const char* string);
void putz(const char* string);
void putz_santitize(char* string);

size_t fputs_upper(FILE* stream, const char* string);

char* remove_ws_prefix(char* string);

void newline();
void fnewline(FILE* f);

char* str_cat_dyn(const char* stra, const char* strb);

#endif //ATOMIC_HELPER_STRING_H
