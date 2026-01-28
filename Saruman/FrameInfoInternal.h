//
// Created by james on 06/01/26.
//

#ifndef FRAMEINFOINTERNAL_H
#define FRAMEINFOINTERNAL_H

#include "shared/Array.h"

#define DW_AUG_STARTER_EH_STR "eh"
#define DW_AUG_STARTER_CHR 'z'
#define DW_AUG_OPTION_LSDA_CHR 'L'
#define DW_AUG_OPTION_PERSONAILTY_ROUTINE_CHR 'P'
#define DW_AUG_OPTION_ADDRESS_POINTER_CHR 'R'

typedef enum AugDataType {
    AUG_DATA_NONE,
    AUG_DATA_EH,
    AUG_DATA_AUG
} AugDataType;

typedef struct CIE_Entry {
    uint64_t offset_location;
    uint64_t length;
    uint8_t version;

    struct AugData {
        uint8_t* aug_string;
        AugDataType type;

        union {
            uint64_t eh_data;
            struct {
                Pointer P_personality_routine_handler;

                PointerEncoding L_pointer_encoding;
                PointerEncoding P_pointer_encoding;
                PointerEncoding R_pointer_encoding;

                bool has_fde_L_pointer_encoding; // L in aug
                bool has_personality_routine_handler; // P in aug
                bool has_fde_address_pointer_encoding; // R in aug
            };
        };
    } aug_data;

    uint8_t address_size;
    uint8_t segment_selector_size;
    uint64_t code_alignment_factor;
    int64_t data_alignment_factor;
    uint64_t return_address_register;
    uint8_t* initial_instructions;
    uint32_t instructions_size;
    uint8_t* padding;
} CIE_Entry;

typedef struct FDE_Entry {
    uint64_t offset_location;
    uint64_t length;
    CIE_Entry* cie_entry;
    uint64_t segment_selector;
    uint64_t initial_location;
    uint64_t address_range;
    uint8_t* instructions;
    uint32_t instructions_size;
    uint8_t* padding;
} FDE_Entry;

typedef struct CIEAndOffset {
    uint64_t offset;
    CIE_Entry* entry;
} CIEAndOffset;

static int read_fde_entry(
    uint8_t* header_base,
    uint8_t* base,
    const uint8_t* end,
    FDE_Entry* entry
);

static int read_cie_entry(uint8_t** start, MODE mode, CIE_Entry* entry);

static Instruction decode_op(uint8_t** start, CIE_Entry* cie);
static void execute_op(Instruction instr, CIE_Entry* cie);
static void decode_and_execute_op(uint8_t** start, CIE_Entry* cie);

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
