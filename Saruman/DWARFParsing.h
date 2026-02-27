//
// Created by james on 01/01/26.
//

#ifndef DWARFPARSING_H
#define DWARFPARSING_H

#include <stdint.h>
#include <stddef.h>

#include "eh_header.h"
#include "Sauron.h"

// Read and advance
#define RAA(elem) \
    {typeof(&elem)temp=(void*)base; elem=*temp; base+= sizeof (elem);}

typedef struct ULEB128 {
    uint64_t v;
    uint8_t size;
} ULEB128;

typedef struct LEB128 {
    int64_t v;
    uint8_t size;
} LEB128;

LEB128 read_leb128(uint8_t* start);
ULEB128 read_uleb128(uint8_t* start);
uint64_t decode_uleb128(uint8_t* start);

ULEB128 raa_uleb128(uint8_t** start);
LEB128 raa_leb128(uint8_t** start);

typedef enum MODE {
    MODE_64bit,
    MODE_32bit
} MODE;

uint8_t* read_initial_length(uint8_t* start, uint64_t* length, MODE* mode);
uint64_t raa_initial_length(uint8_t** start, MODE* mode);

uint64_t raa_offset_by_mode(uint8_t** start, MODE mode);
uint64_t raa_uint(uint8_t** start, uint8_t size);
void raa_const_array(uint8_t** start, uint8_t** dst, size_t length);

typedef struct DW_EXPR {
    uint64_t length;
    uint8_t* data;
} DW_EXPR;

typedef struct DW_BLOCK {
    uint64_t length;
    uint8_t* data;
} DW_BLOCK;;

DW_EXPR raa_expr(uint8_t** start);
DW_BLOCK raa_block(uint8_t** start);
DW_BLOCK raa_block_x(uint8_t** start, uint8_t size);

uint8_t* raa_null_term_string(uint8_t** start);
uint32_t raa_utf8(uint8_t** start);

PointerEncoding raa_pointer_encoding(uint8_t** start);
Pointer raa_pointer_value_from_PE(uint8_t** start, PointerEncoding pe, const Section* section, const Section* data_section, uintptr_t virtual_func_start);
Pointer raa_pointer_value_without_app(uint8_t** start, PointerEncoding pe);
bool pointer_is_omit(PointerEncoding pe);

void print_expression(DW_EXPR* expression);
void print_pointer(const Pointer* pointer);

#endif //DWARFPARSING_H
