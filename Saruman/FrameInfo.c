//
// Created by james on 01/01/26.
//

#include "FrameInfo.h"

#include "DWARFParsing.h"
#include "eh_header.h"
#include "FrameInfoInternal.h"
#include "shared/Array.h"
#include "Tolkien.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


typedef enum AugDataType {
    AUG_DATA_NONE,
    AUG_DATA_EH,
    AUG_DATA_AUG
} AugDataType;

typedef struct CIE_Entry {
    uint64_t length;
    uint8_t version;

    struct {
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
    uint64_t length;
    CIE_Entry* cie_entry;
    uint64_t segment_selector;
    uint64_t initial_location;
    uint64_t address_range;
    uint8_t* instructions;
    uint32_t instructions_size;
    uint8_t* padding;
} FDE_Entry;

static FDE_Entry read_fde_entry(
    uint8_t* header_base,
    uint8_t* base,
    const uint8_t* end,
    uint64_t cie_offset
);

static CIE_Entry read_cie_entry(uint8_t** base);

static Instruction decode_op(uint8_t** start);
static void execute_op(Instruction instr);
static void decode_and_execute_op(uint8_t** start);

int read_header(uint8_t* start) {
    uint64_t length;
    MODE mode;
    uint8_t* base= start;

    base= read_initial_length(base, &length, &mode);
    uint8_t* content_base= base;
    uint8_t* header_end= content_base + length;

    bool is_cie= false;
    int64_t fde_offset= 0;

    if (mode == MODE_32bit) {
        int32_t cie_id;
        RAA(cie_id);

        if (cie_id == (int32_t)-1) {
            is_cie= true;
        } else {
            fde_offset= cie_id;
        }
    } else {
        int64_t cie_id;
        RAA(cie_id);

        if (cie_id == (int64_t)-1) {
            is_cie= true;
        } else {
            fde_offset= cie_id;
        }
    }

    if (is_cie) {
        read_cie_entry(base);
    } else {
        read_fde_entry(start, base, header_end, fde_offset);
    }
}

CIE_Entry read_cie_entry(uint8_t** start, MODE mode) {
    uint8_t* base= *start;

    uint8_t version;
    RAA(version);

    const uint8_t* augmentation= raa_null_term_string(&base);
    const uint8_t* aug_data= base;

    CIE_Entry entry= (CIE_Entry) {
        .aug_data= {
            .has_fde_L_pointer_encoding= false,
            .has_fde_address_pointer_encoding= false,
            .has_personality_routine_handler= false,
            .type= AUG_DATA_NONE
        }
    };

    const uint8_t* aug_eatable= augmentation;
    uint32_t fchr= raa_utf8(&aug_eatable);

    if (fchr == '\0') {
      entry.aug_data.type= AUG_DATA_NONE;
    } else if (strcmp((const char*)augmentation, DW_AUG_STARTER_EH_STR) == 0) {
        // this is an old augmentation string that means
        //  there is some data postceding

        // the documentation on the eh data is as follows
        //On 32 bit architectures, this is a 4 byte value that... On 64 bit architectures, this is a 8 byte value that... This field is only present if the Augmentation String contains the string "eh".
        // that is to say, there isn't any
        uint8_t eh_data_bytes= mode == MODE_32bit ? 4 : 8;

        entry.aug_data.type= AUG_DATA_EH;
        entry.aug_data.eh_data= raa_uint(&base, eh_data_bytes);

    } else if (fchr == DW_AUG_STARTER_CHR) {
        // this character is required for there to be data

        uint32_t chr;
        while (chr= raa_utf8(&aug_eatable), chr != '\0') {
            // this augmentation data area is utf-8 according to the specification
            // Sec. 6.4.1 of v5, It seems that all producers just use z/eh, PLR,
            // and are all encoded just as ascii (utf-8 1 bytes), so full parsing
            // probably isn't needed, but I will use it here
            switch (chr) {
                case DW_AUG_OPTION_LSDA_CHR: {
                    // there is an argument in BOTH the CIE (this) and the FDE
                    entry.aug_data.has_fde_L_pointer_encoding= true;
                    entry.aug_data.L_pointer_encoding= raa_pointer_encoding(&aug_data);
                    break;
                }
                case DW_AUG_OPTION_PERSONAILTY_ROUTINE_CHR: {
                    // there are TWO arguments in the CIE (this)
                    entry.aug_data.has_personality_routine_handler= true;
                    entry.aug_data.P_pointer_encoding= raa_pointer_encoding(&aug_data);
                    entry.aug_data.P_personality_routine_handler= raa_pointer_value_from_PE(&aug_data, entry.aug_data.P_pointer_encoding);
                    break;
                }
                case DW_AUG_OPTION_ADDRESS_POINTER_CHR: {
                    // There is an argument in the CIE (this) and describes the address pointers in the FDEs
                    entry.aug_data.has_fde_address_pointer_encoding= true;
                    entry.aug_data.R_pointer_encoding= raa_pointer_encoding(&aug_data);
                    break;
                }
                default:
                    break;
            }
        }
    }

    RAA(entry.address_size);
    RAA(entry.segment_selector_size);

    entry.code_alignment_factor= raa_uleb128(&base).v;
    entry.data_alignment_factor= raa_leb128(&base).v;
    entry.return_address_register= raa_uleb128(&base).v;
    entry.initial_instructions= base;

    return entry;
}

typedef struct CIEAndOffset {
    uint64_t offset;
    CIE_Entry* entry;
} CIEAndOffset;

int u64_cmp(uint64_t a, uint64_t b) {
    if (a < b) return -1;
    return a > b;
}

ARRAY_PROTO_CMP(CIEAndOffset, CIEO, u64_cmp, offset)
ARRAY_ADD_CMP(CIEAndOffset, CIEO, u64_cmp, offset)

CIEOArray CIEs;

CIE_Entry* get_cie_entry_from_offset(uint64_t offset) {
    CIEAndOffset* res= CIEO_arr_search_ie(&CIEs, offset);
    if (!res) return NULL;

    assert(res->offset == offset);
    return res->entry;
}

void add_cie_entry(CIE_Entry* entry, uint64_t offset) {
    CIEO_arr_add_sorted_i(&CIEs, (CIEAndOffset) {.entry= entry, .offset= offset});
}

FDE_Entry read_fde_entry(
    uint8_t* header_base,
    uint8_t* base,
    const uint8_t* end,
    uint64_t cie_offset
) {
    FDE_Entry entry;

    entry.cie_entry= get_cie_entry_from_offset(cie_offset);

    if (entry.cie_entry->segment_selector_size != 0) {
        uint64_t segment_selector= raa_uint(&base, entry.cie_entry->segment_selector_size);
        log("Decoding of Frame info FDE entry uses segment selector %lu (is this an old program?)\n", segment_selector);

        entry.segment_selector= segment_selector;
    }

    uint8_t addr_size= entry.cie_entry->address_size;

    entry.initial_location= raa_uint(&base, addr_size);
    entry.address_range= raa_uint(&base, addr_size);

    while (base < end) {
        decode_and_execute_op(&base);
    }

    return entry;
}

uint64_t read_address(uint8_t** base_ptr, CIE_Entry* cie) {
    uint8_t addr_size= cie->address_size;

    return raa_uint(base_ptr, addr_size);
}

typedef union RRData {
    int64_t offset;
    uint8_t reg;
    DW_EXPR* expr;
} RRData;

typedef struct RegisterRule {
    FI_OPCODE op;
    RRData data;
} RegisterRule;

typedef struct RegisterData {
    uint8_t register_id;
    RegisterRule rule;
} RegisterData;

static void add_register(uint8_t register_id, FI_OPCODE op, RRData data);

ARRAY_PROTO(RegisterData, RegisterData)
ARRAY_ADD(RegisterData, RegisterData)

typedef union CFAData {
    DW_EXPR* expr;
    struct {
        uint8_t register_id;
        int64_t offset;
    } reg_off;
} CFAData;

typedef enum CFADataType {
    CFA_DT_EXPRESSION,
    CFA_DT_REG_OFF
} CFADataType;

typedef union CFARule {
    CFADataType type;
    CFAData data;
} CFARule;

typedef struct MRow {
    uint64_t address;
    CFARule cfa_rule;
    RegisterDataArray register_rules;
} MRow;

ARRAY_PROTO(MRow, MRow)
ARRAY_ADD(MRow, MRow)

MRowArray matrix;
MRowArray row_stack;

void create_matrix() {
    matrix= MRow_arr_create();
    row_stack= MRow_arr_create();
}

void add_row(uint64_t address) {
    MRow row= (MRow) {
        .address= address,
        .register_rules= RegisterData_arr_create()
    };

    MRow_arr_add(&matrix, row);
}

RegisterData* get_register_data(uint8_t register_id) {
    MRow* c_row= MRow_arr_peek(&matrix);

    for (int i= 0; i < c_row->register_rules.pos; ++i) {
        RegisterData* data= RegisterData_arr_ptr(&c_row->register_rules, i);

        if (data->register_id == register_id) return data;
    }

    return NULL;
}

void set_register_data(RegisterData* rd, FI_OPCODE op, RRData data) {
    rd->rule= (RegisterRule) {
        .op= op,
        .data= data
    };
}

void set_register(uint8_t register_id, FI_OPCODE op, RRData data) {
    RegisterData* existing= get_register_data(register_id);

    if (existing) {
        set_register_data(existing, op, data);
    } else {
        add_register(register_id, op, data);
    }
}

void add_register(uint8_t register_id, FI_OPCODE op, RRData data) {
    MRow* c_row= MRow_arr_peek(&matrix);

    RegisterData rd= (RegisterData) {
        .register_id= register_id,
        .rule= (RegisterRule) {
            .op= op,
            .data= data
        }
    };

    RegisterData_arr_add(&c_row->register_rules, rd);
}

void execute_initial_instr_for(uint8_t register_id, CIE_Entry* cie) {
    // the cie contains a list of initial instructions
    // usually just looks like
    //   0 DW_CFA_def_cfa r7 8
    //   3 DW_CFA_offset r16 - 8
    //   5 DW_CFA_nop
    //   6 DW_CFA_nop

    // this functions basically just exists for the restore functions
    // which take a register and set it's value to that which the initial
    // instruction would set, this is quite annoying, have to go through each
    // initial instruction and find the register that it is linked with
    uint8_t* op= cie->initial_instructions;

    while (op < cie->initial_instructions + cie->instructions_size) {
        Instruction instr= decode_op(&op);
        uint8_t reg= instr.data.register_id;

        if (reg == (int16_t)-1) {
            // this is an instruction that does not contain register information
            continue;
        }

        if (reg == register_id) {
            // this is a matching instruction and so we should execute it
            execute_op(instr);
            break;
        }
    }
}

Instruction decode_op(uint8_t** start, CIE_Entry* cie) {
    uint64_t code_align, data_align;

    code_align= cie->code_alignment_factor;
    data_align= cie->data_alignment_factor;

    Instruction instr= {
        .data= {
            .register_id= -1,
            .d_addr= 0
        },
        .opcode= -1
    };

    uint8_t opcode= **start;

    uint8_t hi, low;
    hi= opcode & 0b11000000;
    low= opcode & 0b00111111;

    if (hi == 0) {
        instr.opcode= (FI_OPCODE)(low);
    } else {
        instr.opcode= (FI_OPCODE)(hi);
    }

    switch (instr.opcode) {
        case DW_CFA_set_loc: {
            uint64_t addr= read_address(start, cie);

            instr.data.d_addr= addr;

            break;
        }
        case DW_CFA_advance_loc:
            instr.data.d_delta= low * code_align;
            break;
        case DW_CFA_advance_loc1:
            instr.data.d_delta= raa_uint(start, 1);
            break;
        case DW_CFA_advance_loc2:
            instr.data.d_delta= raa_uint(start, 2);
            break;
        case DW_CFA_advance_loc4:
            instr.data.d_delta= raa_uint(start, 4);
            break;

        case DW_CFA_def_cfa: {
            ULEB128 reg= raa_uleb128(start);
            ULEB128 off= raa_uleb128(start);

            instr.data.register_id= reg.v;
            instr.data.d_offset= off.v;

            break;
        }
        case DW_CFA_def_cfa_sf: {
            ULEB128 reg= raa_uleb128(start);
            LEB128 off= raa_leb128(start);

            instr.data.register_id= reg.v;
            instr.data.d_offset= off.v * data_align;

            break;
        }

        case DW_CFA_def_cfa_register: {
            ULEB128 reg= raa_uleb128(start);

            instr.data.d_register= reg.v;

            break;
        }

        case DW_CFA_def_cfa_offset: {
            ULEB128 off= raa_uleb128(start);

            instr.data.d_offset= off.v;

            break;
        }

        case DW_CFA_def_cfa_offset_sf: {
            LEB128 off= raa_leb128(start);

            instr.data.d_offset= off.v * data_align;

            break;
        }

        case DW_CFA_def_cfa_expression: {
            DW_EXPR* expr= raa_expr(start);

            instr.data.d_expr= expr;

            break;
        }

        case DW_CFA_undefined: {
            ULEB128 reg= raa_uleb128(start);

            instr.data.register_id= reg.v;

            break;
        }

        case DW_CFA_same_value: {
            ULEB128 reg= raa_uleb128(start);

            instr.data.register_id= reg.v;

            break;
        }

        case DW_CFA_offset: {
            uint8_t reg= low;
            ULEB128 off= raa_uleb128(start);

            instr.data.register_id= reg;
            instr.data.d_offset= off.v;

            break;
        }

        case DW_CFA_offset_extended:
        case DW_CFA_val_offset: {
            ULEB128 reg= raa_uleb128(start);
            ULEB128 off= raa_uleb128(start);

            instr.data.register_id= reg.v;
            instr.data.d_offset= off.v;

            break;
        }

        case DW_CFA_offset_extended_sf:
        case DW_CFA_val_offset_sf: {
            ULEB128 reg= raa_uleb128(start);
            LEB128 off= raa_leb128(start);

            instr.data.register_id= reg.v;
            instr.data.d_offset= off.v * data_align;

            break;
        }

        case DW_CFA_register: {
            ULEB128 rega= raa_uleb128(start);
            ULEB128 regb= raa_uleb128(start);

            instr.data.register_id= rega.v;
            instr.data.d_register= regb.v;

            break;
        }

        case DW_CFA_expression:
        case DW_CFA_val_expression:{
            ULEB128 reg= raa_uleb128(start);
            DW_BLOCK* block= raa_block(start);
            // DW_EXPR* expr= create_expr(DW_EXPR_PUSH, );
            DW_EXPR* expr= block;

            instr.data.register_id= reg.v;
            instr.data.d_expr= expr;

            break;
        }

        case DW_CFA_restore: {
            instr.data.register_id= low;

            break;
        }
        case DW_CFA_restore_extended: {
            ULEB128 reg= raa_uleb128(start);

            instr.data.register_id= reg.v;

            break;
        }

        case DW_CFA_remember_state:
        case DW_CFA_restore_state:
        case DW_CFA_nop:
            break;
    }
}

void execute_op(Instruction instr, CIE_Entry* cie) {
    switch (instr.opcode) {
        case DW_CFA_set_loc: {
            add_row(instr.data.d_addr);
            break;
        }

        case DW_CFA_advance_loc:
        case DW_CFA_advance_loc1:
        case DW_CFA_advance_loc2:
        case DW_CFA_advance_loc4: {
            MRow* c_row= MRow_arr_peek(&matrix);
            uint64_t n_addr= instr.data.d_delta + c_row->address;
            add_row(n_addr);

            break;
        }

        case DW_CFA_def_cfa:
        case DW_CFA_def_cfa_sf: {
            MRow* c_row= MRow_arr_peek(&matrix);

            c_row->cfa_rule= (CFARule) {
                .type= CFA_DT_REG_OFF,
                .data.reg_off= {
                    .register_id= instr.data.register_id,
                    .offset= instr.data.d_offset
                }
            };

            break;
        }

        case DW_CFA_def_cfa_register: {
            MRow* c_row= MRow_arr_peek(&matrix);

            if (c_row->cfa_rule.type != CFA_DT_REG_OFF) {
                log("Decoding DWARF frame info provided invalid decoded instructions. DW_CFA_def_cfa_register expects cfa data type `reg-off` got `expression`\n");
            }

            c_row->cfa_rule.data.reg_off.register_id= instr.data.d_register;
            break;
        }

        case DW_CFA_def_cfa_offset:
        case DW_CFA_def_cfa_offset_sf: {
            MRow* c_row= MRow_arr_peek(&matrix);

            if (c_row->cfa_rule.type != CFA_DT_REG_OFF) {
                log("Decoding DWARF frame info provided invalid decoded instructions. DW_CFA_def_cfa_offset.* expects cfa data type `reg-off` got `expression`\n");
            }

            c_row->cfa_rule.data.reg_off.offset= instr.data.d_offset;
            break;
        }

        case DW_CFA_def_cfa_expression: {
            MRow* c_row= MRow_arr_peek(&matrix);

            c_row->cfa_rule= (CFARule) {
                .type= CFA_DT_EXPRESSION,
                .data.expr= instr.data.d_expr
            };
            break;
        }

        case DW_CFA_undefined: {
            set_register(
                instr.data.register_id,
                DW_CFA_undefined,
                {0}
            );

            break;
        }

        case DW_CFA_same_value: {
            set_register(
                instr.data.register_id,
                DW_CFA_same_value,
                {0}
            );

            break;
        }

        case DW_CFA_offset:
        case DW_CFA_offset_extended:
        case DW_CFA_offset_extended_sf: {
            set_register(
                instr.data.register_id,
                DW_CFA_offset,
                {.offset= instr.data.d_offset}
            );

            break;
        }

        case DW_CFA_val_offset:
        case DW_CFA_val_offset_sf: {
            set_register(
                instr.data.register_id,
                DW_CFA_val_offset,
                {.offset= instr.data.d_offset}
            );

            break;
        }

        case DW_CFA_register: {
            set_register(
                instr.data.register_id,
                DW_CFA_register,
                {.reg= instr.data.d_register}
            );

            break;
        }

        case DW_CFA_expression:
        case DW_CFA_val_expression: {
            // The value of the CFA is pushed on
            //  the DWARF evaluation stack prior to execution of the DWARF expression.

            set_register(
                instr.data.register_id,
                instr.opcode,
                {.expr= instr.data.d_expr}
            );

            break;
        }

        case DW_CFA_restore:
        case DW_CFA_restore_extended: {
            execute_initial_instr_for(instr.data.register_id, cie);

            break;
        }

        case DW_CFA_remember_state: {
            MRow_arr_add(&row_stack, *MRow_arr_peek(&matrix));

            break;
        }
        case DW_CFA_restore_state: {
            MRow popped= MRow_arr_pop(&row_stack);
            MRow* c_row= MRow_arr_peek(&matrix);
            uint64_t c_addr= c_row->address;

            *c_row= (MRow) {
                .address= c_addr,
                .register_rules= popped.register_rules,
                .cfa_rule= popped.cfa_rule
            };

            break;
        }
        case DW_CFA_nop:
            break;
    }
}

void decode_and_execute_op(uint8_t** start) {
    Instruction instr= decode_op(start);
    execute_op(instr);
}
