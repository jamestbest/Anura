//
// Created by jamestbest on 1/24/26.
//

#include "default.h"

#include "Errors.h"

ByteStream stream;

bool bit_from_raw_stream(const uint8_t* stream, const uint64_t idx) {
    return ((stream[idx / 8]) >> (7 - (idx % 8)) & 1);
}

bool bit_from_stream(const ByteStream* stream, const uint64_t idx, bool* succ) {
    if (idx > stream->max_pointer) {
        *succ= false;
        return false;
    }

    *succ= true;
    return bit_from_raw_stream(stream->raw_stream, idx);
}

int consume_lit_binary(
    ByteStream* bytes,
    const uint8_t* binary_pattern,
    const uint16_t pattern_size
) {
    for (int i = 0; i < pattern_size; ++i) {
        const bool dbit= read_bit(bytes);
        const bool pbit= bit_from_raw_stream(binary_pattern, i);
        if (dbit != pbit) return FAIL;
    }

    return SUCCESS;
}

ByteStream init_stream(uint8_t* raw_stream) {
    return (ByteStream) {
        .raw_stream= raw_stream,
        .pointer= 0
    };
}

// [[todo]] add the error checking from bit_from_stream
bool read_bit(ByteStream* stream) {
    return bit_from_raw_stream(stream->raw_stream, stream->pointer++);
}

uint64_t read_bits(ByteStream* stream, uint32_t bits) {
    uint64_t res= 0;
    for (int i = 0; i < bits; ++i) {
        const bool bit= read_bit(stream);
        res= (res << 1) | bit;
    }
    return res;
}

bool expect_bits(const uint8_t bits, const uint64_t bit_pattern) {
    for (int i = 0; i < bits; ++i) {
        const bool dbit= read_bit(&stream);
        const bool pbit= (bit_pattern >> i) & 1;

        if (dbit != pbit) return false;
    }

    return true;
}

char* evaluate_string(char* string, ...) {
    va_list args;
    va_start(args, string);

    Buffer buffer;

    size_t idx= 0;
    size_t start= 0;
    while (string[idx] != '\0') {
        const char c= string[idx];

        if (c == '{') {
            string[idx]= '\0';
            buffer_concat(&buffer, &string[start]);
            string[idx]= '{';
            while (string[idx] != '}') idx++;
            idx++;
            start= idx;

            const generator_function f= va_arg(args, generator_function);
            f(&buffer);
        } else {
            idx++;
        }
    }

    if (string[start] != '\0') {
        buffer_concat(&buffer, &string[start]);
    }

    va_end(args);

    char* res= buffer_steal(&buffer, 0);
    buffer_destroy(&buffer);

    return res;
}

void set_aval(AVAL* dst, char* data) {
    *dst= to_aval(data);
}

AVAL to_aval(char* data) {
    return (AVAL) {
        .chosen_idx= AVAL_STATUS_SELECTED,
        .chosen_val= data,
        .parsed_successfully= true
    };
}
