//
// Created by james on 29/09/25.
//

#ifndef SARUMAN_H
#define SARUMAN_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "Target.h"

typedef struct DecodeRet {
    size_t bytes_read;
    bool end_of_code;
} DecodeRet;

int decode_lines(uint8_t* start, void* string_data, void* text_data, uint64_t text_off);

typedef struct LC {
    uint32_t line;
    uint32_t col;
} LC;

// Address range; mainly used for source line -> address which through the byte code is INCLUSIVE, EXCLUSIVE
typedef struct ARange {
    uintptr_t s; // INCLUSIVE
    uintptr_t e; // EXCLUSIVE
} ARange;

extern const LC LC_ERR;
extern const ARange ARange_ERR;

// OLD OLD DEPRICATED DON@T USE
LC addr2line(uintptr_t addr);
ARange line2addr(uint32_t line);

// THESE USE THE HEADER
LineAddrRes line2startaddr(uint32_t l);

typedef struct LNData {
    uint16_t start_offset;
    uint16_t size;
} LNData;

typedef struct LNEntry {
    uint16_t max_size;
    uint16_t cc;
} LNEntry;

// LNEntry (2 + 2 + 4 * cc bytes)
//  cc (2 bytes)
//  max_size (2 bytes)
//  LIST OF LNData
//   (start (2 bytes), size (2 bytes)) (4 bytes)

// The structure of the header is:
//  - Max lines
//  - Line information, which is at lines, and can be indexed by the line
//    - a value of -1 means that there is no addresses for that line
//    - a value means index of data i.e. start of header
//  - Each valid line entry contains a LNEntry
//    - most of the data within is stored as a ULEB128 that must be parsed
typedef struct LNHeader {
    uint8_t* data;

    uint32_t max_line;
    uint32_t* lines;
} LNHeader;

typedef struct LNInfo {
    LNHeader header;

    uint8_t* entries;
    uint8_t* next_free;
    uint32_t last_entry;
    uint64_t max_size;
} LNInfo;

void create_header();
void print_header();

#endif //SARUMAN_H
