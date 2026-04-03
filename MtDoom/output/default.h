//
// Created by James Coward on 1/24/26.
//

#ifndef ANURA_DEFAULT_H
#define ANURA_DEFAULT_H

#include <stdbool.h>
#include <stdint.h>
#include "math.h"

#include "shared/Vector.h"
#include "shared/Buffer.h"

typedef void(*generator_function)(Buffer* buffer);

typedef struct ByteStream {
    uint8_t* raw_stream;
    uint64_t pointer;
    uint64_t max_pointer;
    uint64_t size;
} ByteStream;

extern ByteStream top_stream;

bool read_bit(ByteStream* stream);

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

extern const AVAL AVAL_EMPTY;

typedef struct ParseRet {
    AVAL aval;
    const char* error_string;
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
#define EXPECT_BITS(num, stream) expect_bits(COUNT_ALT_BASE_DIGITS(num), num, stream)
#define EXPECT_BYTE(num, stream) expect_bits(8, num, stream)

bool expect_bits(uint8_t bits, uint64_t bit_pattern, ByteStream* stream);
int init();
int disassemble(const char** output);

uint64_t read_bits(ByteStream* stream, uint32_t bits);

char* evaluate_string(char* string, ...);
void set_aval(AVAL* dst, char* data);
AVAL to_aval(char* data);
AVAL data_to_aval(uint64_t value);
char* data_to_string(int64_t value);
void clear_aval(AVAL* aval);

void stream_add_bit(ByteStream* stream, bool value);
void stream_add(ByteStream* stream, uint64_t value, uint16_t bits);

ByteStream stream_construct(size_t size);
ByteStream stream_create();
void stream_destroy(ByteStream* stream);

ByteStream stream_from_bytes(const uint8_t* bytes, size_t byte_count);void reset();

#define PARSE_SUCC(aval_) (ParseRet){.aval=aval_, .success= true}
#define PARSE_SUCC_HIDDEN (ParseRet){.aval=AVAL_EMPTY, .success= true}
#define PARSE_FAIL (ParseRet){.success=false}

#endif //ANURA_DEFAULT_H