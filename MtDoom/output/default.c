//
// Created by James Coward on 1/24/26.
//

#include "default.h"

#include <stdlib.h>

#include "Errors.h"
#include "shared.h"

ByteStream top_stream;

const AVAL AVAL_EMPTY= (AVAL) {
    .chosen_val= "",
    .chosen_idx= AVAL_STATUS_SELECTED,
    .parsed_successfully= true
};

ByteStream stream_construct(size_t size) {
    ByteStream stream;
    stream.raw_stream= malloc(size * sizeof(uint8_t));
    stream.size= size * sizeof(uint8_t) * 8;
    stream.max_pointer= 0;
    stream.pointer= 0;
    return stream;
}

ByteStream stream_create() {
    return stream_construct(5);
}

void stream_destroy(ByteStream* stream) {
    free(stream->raw_stream);
    stream->size= 0;
    stream->max_pointer= 0;
}

void stream_resize(ByteStream* stream) {
    const size_t new_size= stream->size << 1;
    void* new_stream= realloc(stream->raw_stream, new_size);
    if (!new_stream) assert(false);

    stream->size= new_size;
    stream->raw_stream= new_stream;
}

void stream_add_bit(ByteStream* stream, bool value) {
    if (stream->max_pointer == stream->size) stream_resize(stream);

    const size_t idx= stream->max_pointer >> 3;
    const size_t bit= 7 - (stream->max_pointer & 7);
    stream->raw_stream[idx] &= ~(1 << bit);
    stream->raw_stream[idx] |= value << bit;
    stream->max_pointer++;
}

void stream_add(ByteStream* stream, uint64_t value, uint16_t bits) {
    for (int i = bits - 1; i >= 0; i--) {
        const bool bit= value >> i & 1;
        stream_add_bit(stream, bit);
    }
}

ByteStream stream_from_bytes(const uint8_t* bytes, const size_t byte_count) {
    ByteStream stream= stream_construct(byte_count << 3);
    for (size_t i = 0; i < byte_count; i++) {
        stream.raw_stream[i]= bytes[i];
    }
    stream.max_pointer= byte_count << 3;
    return stream;
}

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

// [[todo]] add the error checking from bit_from_stream
bool read_bit(ByteStream* stream) {
    return bit_from_raw_stream(stream->raw_stream, stream->pointer++);
}

ENDIANNESS endianness= ENDIAN_LITTLE;

uint64_t read_bits(ByteStream* stream, uint32_t bits) {
    if (bits % 8 == 0 && bits > 8) {
        // here endianness comes into play
        if (endianness == ENDIAN_LITTLE) {
            uint64_t res= 0;
            for (int i = 0; i < bits >> 3; ++i) {
                res |= (read_bits(stream, 8) << (i << 3));
            }
            return res;
        }
    }
    uint64_t res= 0;
    for (int i = 0; i < bits; ++i) {
        const bool bit= read_bit(stream);
        res= (res << 1) | bit;
    }
    return res;
}

bool expect_bits(const uint8_t bits, const uint64_t bit_pattern, ByteStream* stream) {
    const size_t old_pointer= stream->pointer;
    for (int i = 0; i < bits; ++i) {
        const bool dbit= read_bit(stream);
        const bool pbit= (bit_pattern >> (bits - i - 1)) & 1;

        if (dbit != pbit) {
            stream->pointer= old_pointer;
            return false;
        }
    }

    return true;
}

char* evaluate_string(char* string, ...) {
    va_list args;
    va_start(args, string);

    Buffer buffer= buffer_create(BUFF_MIN);

    size_t idx= 0;
    size_t start= 0;
    while (string[idx] != '\0') {
        const char c= string[idx];

        if (c == '{') {
            buffer_nconcat(&buffer, &string[start], idx - start);
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

AVAL data_to_aval(uint64_t value) {
    return to_aval(data_to_string((int64_t)value));
}

#define HEX_DIGITS(num) log2(num) / 4
#define HEX_EXTRA_SIZE sizeof("-0x")
#define MAX_HEX_CHARS 20

char* data_to_string(int64_t value) {
    char* buff= malloc(MAX_HEX_CHARS);
    if (value < 0) {
        snprintf(buff, MAX_HEX_CHARS, "-0x%lx", -value);
    } else {
        snprintf(buff, MAX_HEX_CHARS, "%#lx", value);
    }
    return buff;
}

void clear_aval(AVAL* aval) {
    aval->parsed_successfully= false;
    aval->chosen_idx= AVAL_STATUS_NONE;
    aval->chosen_val= NULL;

    aval->choices= vector_construct(0);
    aval->choices.pos= 0;
}
