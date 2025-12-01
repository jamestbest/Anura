//
// Created by jamestbest on 9/27/25.
//

#ifndef ANURA_SAURON_H
#define ANURA_SAURON_H

#include <stdio.h>
#include <elf.h>
#include <stdlib.h>

int decode(FILE* elf);

typedef struct ELF64 {
    Elf64_Ehdr header;
    struct ProgHeader {
        Elf64_Phdr* program_headers;

        // meta info
        struct {
            uint header_count;
            uint header_size;
            uint load_segment_count;
        };
    } ProgHeader;

    Elf64_Shdr* sections;

    struct {
        void* text;
        void* debug_line;
        void* debug_line_str;
        void* str;
    } data;
} ELF64;

extern ELF64 ELF;

typedef enum DW_FORM {
    DW_FORM_addr= 1,
    DW_FORM_reserved0,
    DW_FORM_block2,
    DW_FORM_block4,
    DW_FORM_data2,
    DW_FORM_data4,
    DW_FORM_data8,
    DW_FORM_string,
    DW_FORM_block,
    DW_FORM_block1,
    DW_FORM_data1,
    DW_FORM_flag,
    DW_FORM_sdata,
    DW_FORM_strp,
    DW_FORM_udata,
    DW_FORM_ref_addr,
    DW_FORM_ref1,
    DW_FORM_ref2,
    DW_FORM_ref4,
    DW_FORM_ref8,
    DW_FORM_ref_udata,
    DW_FORM_indirect,
    DW_FORM_sec_offset,
    DW_FORM_exprloc,
    DW_FORM_flag_present,
    DW_FORM_strx,
    DW_FORM_addrx,
    DW_FORM_ref_sup4,
    DW_FORM_strp_sup,
    DW_FORM_data16,
    DW_FORM_line_strp,
    DW_FORM_ref_sig8,
    DW_FORM_implicit_const,
    DW_FORM_loclistx,
    DW_FORM_rnglistx,
    DW_FORM_ref_sup8,
    DW_FORM_strx1,
    DW_FORM_strx2,
    DW_FORM_strx3,
    DW_FORM_strx4,
    DW_FORM_addrx1,
    DW_FORM_addrx2,
    DW_FORM_addrx3,
    DW_FORM_addrx4,
} DW_FORM;

extern const char* DW_FORM_STRS[];


#endif //ANURA_SAURON_H
