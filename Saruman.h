//
// Created by james on 29/09/25.
//

#ifndef SARUMAN_H
#define SARUMAN_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct DecodeRet {
    size_t bytes_read;
    bool end_of_code;
} DecodeRet;

int decode_lines(uint8_t* start, void* string_data, void* text_data, uint64_t text_off);

typedef struct LC {
    uint32_t line;
    uint32_t col;
} LC;

typedef struct ARange {
    uintptr_t s;
    uintptr_t e;
} ARange;

extern const LC LC_ERR;
extern const ARange ARange_ERR;

LC addr2line(uintptr_t addr);
ARange line2addr(uint32_t line);

#endif //SARUMAN_H
