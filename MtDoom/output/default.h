//
// Created by jamestbest on 1/24/26.
//

#ifndef ANURA_DEFAULT_H
#define ANURA_DEFAULT_H

#include <stdbool.h>
#include <stdint.h>

#include "shared/Vector.h"
#include "shared/Buffer.h"

typedef void(*generator_function)(Buffer* buffer);

typedef struct ByteStream {
    uint8_t* raw_stream;
    uint64_t pointer;
} ByteStream;

extern ByteStream stream;

ByteStream init_stream(uint8_t* raw_stream);
bool read_bit(ByteStream* stream);

typedef struct ParseRet {
    union {
        const char* output_string;
        const char* error_string;
    };
    uint32_t bits_read;
    bool success;
} ParseRet;

typedef struct StrSlice {
    uint8_t* stream;
    uint32_t pos;
    uint32_t len;
} StrSlice;

#define STR(X) #X
#define XSTR(X) STR(X)
#define COUNT_ALT_BASE_DIGITS(num) (sizeof(XSTR(num)) - 1 - 2)
#define ALT_DIGITS(num) COUNT_ALT_BASE_DIGITS(num)
#define EXPECT_BIN_HEX(num) {                                                                           \
    const char *const str= XSTR(num);                                                                   \
    const char first= str[0], second= str[1];                                                           \
    _Static_assert(first == '0' && (second == 'x' || second == 'X' || second == 'b' || second == 'B')); \
}
#define EXPECT_BITS(num) expect_bits(COUNT_ALT_BASE_DIGITS(num), num)

bool expect_bits(uint8_t bits, uint64_t bit_pattern);
int init();

typedef enum AVAL_STATUS {
    AVAL_STATUS_SELECTED=254,
    AVAL_STATUS_NONE=255,
} AVAL_STATUS;

typedef struct AVAL {
    Vector choices;
    char* chosen_val;
    AVAL_STATUS chosen_idx;
    bool parsed_successfully;
} AVAL;

int evaluate_string(char* string, ...);
void set_aval(AVAL* dst, char* data);

#endif //ANURA_DEFAULT_H