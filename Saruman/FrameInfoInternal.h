//
// Created by james on 06/01/26.
//

#ifndef FRAMEINFOINTERNAL_H
#define FRAMEINFOINTERNAL_H

#include "shared/Array.h"
#include "FrameInfo.h"

#define DW_AUG_STARTER_EH_STR "eh"
#define DW_AUG_STARTER_CHR 'z'
#define DW_AUG_OPTION_LSDA_CHR 'L'
#define DW_AUG_OPTION_PERSONAILTY_ROUTINE_CHR 'P'
#define DW_AUG_OPTION_ADDRESS_POINTER_CHR 'R'


typedef struct CIEAndOffset {
    uint64_t offset;
    CIE_Entry* entry;
} CIEAndOffset;

static int read_fde_entry(
    uint8_t* header_base,
    uint8_t* base,
    const uint8_t* end,
    const uint8_t* section_start,
    FDE_Entry* entry
);

static int read_cie_entry(uint8_t** start, MODE mode, uint8_t* header_end, CIE_Entry* entry);

static Instruction decode_op(uint8_t** start, const CIE_Entry* cie);
static void execute_op(Instruction instr, const CIE_Entry* cie);
static void decode_and_execute_op(uint8_t** start, const CIE_Entry* cie);

static int u64_cmp(uint64_t a, uint64_t b) {
    if (a < b) return -1;
    return a > b;
}

ARRAY_PROTO_CMP(CIEAndOffset, CIEO, u64_cmp, offset)
ARRAY_ADD_CMP(CIEAndOffset, CIEO, u64_cmp, offset)

ARRAY_PROTO(FDE_Entry, FDE)
ARRAY_ADD(FDE_Entry, FDE)

static CIE_Entry* get_cie_entry_from_offset(uint64_t offset);

#endif //FRAMEINFOINTERNAL_H
