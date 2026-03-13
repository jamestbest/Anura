//
// Created by james on 01/01/26.
//

#ifndef FRAMEINFO_H
#define FRAMEINFO_H

// They are encoded as 2 bits high
//  6 bits low, to allow some operand data
//  for some instructions
#define FI_OP_ENCODING(hi, low) (((hi << 6) & 0b11000000) | (low & 0b00111111))

#include "DWARFParsing.h"

#include <stdint.h>
#include <elf.h>

#include "Sauron.h"

typedef enum FI_OPCODE {
    DW_CFA_set_loc= FI_OP_ENCODING(0, 1),
    DW_CFA_advance_loc= FI_OP_ENCODING(0x01, 0),
    DW_CFA_advance_loc1= FI_OP_ENCODING(0, 0x02),
    DW_CFA_advance_loc2= FI_OP_ENCODING(0, 0x03),
    DW_CFA_advance_loc4= FI_OP_ENCODING(0, 0x04),

    DW_CFA_def_cfa= FI_OP_ENCODING(0, 0x0c),
    DW_CFA_def_cfa_sf= FI_OP_ENCODING(0, 0x12),
    DW_CFA_def_cfa_register= FI_OP_ENCODING(0, 0x0d),
    DW_CFA_def_cfa_offset= FI_OP_ENCODING(0, 0x0e),
    DW_CFA_def_cfa_offset_sf= FI_OP_ENCODING(0, 0x13),
    DW_CFA_def_cfa_expression= FI_OP_ENCODING(0, 0x0f),

    DW_CFA_undefined= FI_OP_ENCODING(0, 0x07),
    DW_CFA_same_value= FI_OP_ENCODING(0, 0x08),
    DW_CFA_offset= FI_OP_ENCODING(0x2, 0),
    DW_CFA_offset_extended= FI_OP_ENCODING(0, 0x05),
    DW_CFA_offset_extended_sf= FI_OP_ENCODING(0, 0x11),
    DW_CFA_val_offset= FI_OP_ENCODING(0, 0x14),
    DW_CFA_val_offset_sf= FI_OP_ENCODING(0, 0x15),
    DW_CFA_register= FI_OP_ENCODING(0, 0x09),
    DW_CFA_expression= FI_OP_ENCODING(0, 0x10),
    DW_CFA_val_expression= FI_OP_ENCODING(0, 0x16),

    DW_CFA_restore= FI_OP_ENCODING(0x3, 0),
    DW_CFA_restore_extended= FI_OP_ENCODING(0, 0x06),

    DW_CFA_remember_state= FI_OP_ENCODING(0, 0x0a),
    DW_CFA_restore_state= FI_OP_ENCODING(0, 0x0b),

    DW_CFA_nop= FI_OP_ENCODING(0, 0x0),
} FI_OPCODE;

typedef struct IData {
    uint8_t register_id;

    union {
        int64_t d_offset;
        uint8_t d_register;
        DW_EXPR d_expr;
        uint8_t d_delta;
        uint64_t d_addr;
    };
} IData;

typedef struct Instruction {
    FI_OPCODE opcode;
    IData data;
} Instruction;

void frame_info_init();
void frame_info_destroy();

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
    Pointer initial_location;
    Pointer address_range;
    uint8_t* instructions;
    uint32_t instructions_size;
    uint8_t* padding;
} FDE_Entry;

int parse_frame_info(Section* section, Section* hdr);
FDE_Entry* get_fde_for_pc(uintptr_t pc);
uint64_t cfa_value_at(uintptr_t pc, bool* succ);
uint64_t reg_value_at(uint64_t pc, bool* succ, uint16_t register_id);

#endif //FRAMEINFO_H
