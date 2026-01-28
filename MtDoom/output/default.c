//
// Created by jamestbest on 1/24/26.
//

#include "default.h"

#include "Errors.h"

ByteStream stream;

bool bit_from_raw_stream(const uint8_t* stream, const uint64_t idx) {
    return ((stream[idx / 8]) >> (7 - (idx % 8)) & 1);
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

bool read_bit(ByteStream* stream) {
    return bit_from_raw_stream(stream->raw_stream, stream->pointer++);
}

bool expect_bits(const uint8_t bits, const uint64_t bit_pattern) {
    for (int i = 0; i < bits; ++i) {
        const bool dbit= read_bit(&stream);
        const bool pbit= (bit_pattern >> i) & 1;

        if (dbit != pbit) return false;
    }

    return true;
}
