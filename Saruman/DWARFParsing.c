//
// Created by james on 01/01/26.
//

#include "DWARFParsing.h"

#include "Helper_File.h"
#include "Helper_String.h"
#include "Sauron.h"

#include <stddef.h>

uint64_t raa_uint(uint8_t** start, uint8_t size) {
    uint64_t res= 0;
    uint8_t* base= *start;

    for (int i= 0; i < size; ++i) {
        res |= ((uint64_t)base[i]) << (i << 3);
    }

    *start += size;

    return res;
}

int64_t raa_int(uint8_t** start, uint8_t size) {
    uint64_t base= raa_uint(start, size);

    // extending the sign bit
    //              v find the sign bit of the first byte
    // 0b...00000000xyyyyyyy...
    uint64_t sign_bit_mask= (uint64_t)1 << ((size << 3) - 1);
    if (base & sign_bit_mask) {
        // all to 1 then shift left to clear all the set bits to 0. then everything above is a 1
        base |= ~((uint64_t)0) << (size << 3);
    }

    return (int64_t)base;
}

PointerEncoding raa_pointer_encoding(uint8_t** start) {
    uint8_t combined= **start;

    *start += 1;

    const PEValueFormat vf= combined & 0x0f;
    const PEApplication ap= combined & 0xf0;

    return (PointerEncoding) {
        .value= vf,
        .application= ap
    };
}

bool pointer_is_omit(const PointerEncoding pe) {
    return pe.value == DW_EH_PE_V_omit && pe.application == DW_EH_PE_A_omit;
}

Pointer raa_pointer_value_without_app(uint8_t** start, const PointerEncoding pe) {
    Pointer res= (Pointer) {
        .application= pe.application,
        .signed_ptr= false,
        .ptr_u= 0x0,
        .omitted= false
    };;

    if (pe.value == DW_EH_PE_V_omit && pe.application == DW_EH_PE_A_omit) {
        res.omitted= true;
        return res;
    }

    switch (pe.value) {
        case DW_EH_PE_absptr: {
            // the size is from the architecture
            const uint8_t class= ELF.header.e_ident[EI_CLASS];
            uint8_t pointer_size;
            switch (class) {
                case ELFCLASS32: pointer_size= 4; break;
                case ELFCLASS64: pointer_size= 8; break;
                case ELFCLASSNONE:
                default:
                    assert(false);
            }

            res.ptr_u= raa_uint(start, pointer_size);
            break;
        }
        case DW_EH_PE_uleb128:
            res.ptr_u= raa_uleb128(start).v;
            break;
        case DW_EH_PE_udata2:
            res.ptr_u= raa_uint(start, 2);
            break;
        case DW_EH_PE_udata4:
            res.ptr_u= raa_uint(start, 4);
            break;
        case DW_EH_PE_udata8:
            res.ptr_u= raa_uint(start, 8);
            break;

        case DW_EH_PE_sleb128:
            res.ptr_s= raa_leb128(start).v;
            res.signed_ptr= true;
            break;
        case DW_EH_PE_sdata2:
            res.ptr_s= raa_int(start, 2);
            res.signed_ptr= true;
            break;
        case DW_EH_PE_sdata4:
            res.ptr_s= raa_int(start, 4);
            res.signed_ptr= true;
            break;
        case DW_EH_PE_sdata8:
            res.ptr_s= raa_int(start, 8);
            res.signed_ptr= true;
            break;
    }

    if (res.signed_ptr) res.value= res.ptr_s;
    else res.value= res.ptr_u;

    return res;
}

Pointer raa_pointer_value_from_PE(uint8_t** start, const PointerEncoding pe, const Section* section, const Section* data_section, uintptr_t virtual_func_start) {
    uint8_t* base= *start;
    Pointer res= raa_pointer_value_without_app(start, pe);

    switch (pe.application) {
        case DW_EH_PE_pcrel: {
            res.value= section->header->sh_addr + (base - section->data);
            break;
        }
        case DW_EH_PE_textrel: {
            res.value= ELF.section_map.text.header->sh_addr;
            break;
        }
        case DW_EH_PE_datarel: {
            res.value= data_section->header->sh_addr;
            break;
        }
        case DW_EH_PE_funcrel: {
            if (!virtual_func_start) assert(false);
            res.value= virtual_func_start;
        }
            break;
        case DW_EH_PE_aligned: assert(false);
    }

    if (res.signed_ptr) res.value += res.ptr_s;
    else res.value += res.ptr_u;

    return res;
}

uint8_t* read_initial_length(uint8_t* start, uint64_t* length, MODE* mode) {
    uint32_t l32= *(uint32_t*)start;
    uint64_t l64;

    start += sizeof(l32);

    if (l32 == 0xFFFFFFFF) {
        *mode= MODE_64bit;
        l64= *(uint64_t*)start;
        start += sizeof(l64);
    } else {
        *mode= MODE_32bit;
        l64= l32;
    }

    *length= l64;

    return start;
}

uint64_t raa_initial_length(uint8_t** start, MODE* mode) {
    const uint32_t l32= raa_uint(start, 4);
    uint64_t l64;

    if (l32 == 0xFFFFFFFF) {
        *mode= MODE_64bit;
        l64= raa_uint(start, 8);
    } else {
        *mode= MODE_32bit;
        l64= l32;
    }

    return l64;
}

uint64_t raa_offset_by_mode(uint8_t** start, const MODE mode) {
    if (mode == MODE_32bit) {
        return raa_uint(start, 4);
    }
    if (mode == MODE_64bit) {
        return raa_uint(start, 8);
    }
    assert(false);
}

uint64_t decode_uleb128(uint8_t* start) {
    uint64_t res= 0;
    uint64_t shift= 0;
    uint8_t byte;
    size_t bi= 0;
    do {
        byte= start[bi++];
        res |= (byte & 0b01111111) << shift;
        shift += 7;
    } while ((byte & 0b10000000) != 0);

    return res;
}

ULEB128 raa_uleb128(uint8_t** start) {
    ULEB128 res= read_uleb128(*start);

    *start += res.size;

    return res;
}

ULEB128 read_uleb128(uint8_t* start) {
    uint64_t res= 0;
    uint8_t i= 0, s= 0, b;
    do {
        b= start[i++];
        res |= (b & 0x7f) << s;
        s += 7;
    } while ((b & 0x80) != 0);

    return (ULEB128) {
        .size= i,
        .v= res
    };
}

LEB128 raa_leb128(uint8_t** start) {
    LEB128 res= read_leb128(*start);

    *start += res.size;

    return res;
}

LEB128 read_leb128(uint8_t* start) {
    int64_t res= 0;
    uint8_t shift= 0;

    uint8_t size= 64; // 64 bits in the result variable
    uint8_t b, i=0;

    do {
        b= start[i++];
        res |= (b & 0x7f) << shift;
        shift += 7;
    } while ((b & 0x80) != 0);

    if ((shift < size) && ((b & 0x40) != 0))
        res |= (~0 << shift);

    return (LEB128) {
        .v= res,
        .size= i
    };
}

DW_EXPR raa_expr(uint8_t** start) {
    const ULEB128 length= raa_uleb128(start);
    //[[todo]]: actual expression parsing

    uint8_t* base= *start;
    *start+= length.v;

    return (DW_EXPR) {
        .length= length.v,
        .data= base
    };
}

DW_BLOCK raa_block(uint8_t** start) {
    const ULEB128 length= raa_uleb128(start);

    // [[todo]] actual block parsing?
    uint8_t* base= *start;
    *start+= length.v;

    return (DW_BLOCK) {
        .length= length.v,
        .data= base
    };
}

DW_BLOCK raa_block_x(uint8_t** start, uint8_t size) {
    const uint64_t length= raa_uint(start, size);

    return (DW_BLOCK) {
        .length= length,
        .data= *start
    };
}

void raa_const_array(uint8_t** start, uint8_t** dst, size_t length) {
    for (size_t i = 0; i < length; i++) {
        *dst[i]= raa_uint(start, 1);
    }
}

uint8_t* raa_null_term_string(uint8_t** start) {
    uint8_t* base= *start;
    uint8_t* res= base;

    while (*base != '\0') base++;

    *start= base + 1;

    return res;
}

uint32_t raa_utf8(uint8_t** start) {
    UTF8Pos res= getutf8((const char*)*start);

    *start+= res.bytes;

    return res.value;
}

void print_expression(const DW_EXPR* expression) {
    printf("<<TODO>> expr <<TODO>>");
}

void print_pointer(const Pointer* pointer) {
    switch (pointer->application) {
        case DW_EH_PE_pcrel:
            printf("[PC-rel]");
            break;
        case DW_EH_PE_textrel:
            printf("[Text-rel]");
            break;
        case DW_EH_PE_datarel:
            printf("[Data-rel]");
            break;
        case DW_EH_PE_funcrel:
            printf("[Func-rel]");
            break;
        case DW_EH_PE_aligned:
            printf("[Aligned]");
            break;
    }
    if (pointer->signed_ptr) {
        printf(" %ld (s)", pointer->ptr_s);
    } else {
        printf(" %lu", pointer->ptr_u);
    }

    printf(" V: %#lx", pointer->value);
}
