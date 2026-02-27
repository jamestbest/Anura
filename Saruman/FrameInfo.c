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

#include "Errors.h"
#include "Helper_String.h"
#include "Sauron.h"

static CIEOArray CIEs;
static FDEArray FDEs;

static void print_fde(const FDE_Entry* entry);
static void print_cie(const CIE_Entry* entry);
static void decode_and_execute_ops(uint8_t* base, uint64_t size, const CIE_Entry* cie);

static int read_header(uint8_t** start, uint8_t* section_start);

static void add_row(uint64_t address);
static void create_matrix();

static void print_matrix();

CIE_Entry* get_cie_entry_from_offset(uint64_t offset) {
    CIEAndOffset* res= CIEO_arr_search_ie(&CIEs, offset);
    if (!res) return NULL;

    assert(res->offset == offset);
    return res->entry;
}

void add_cie_entry(CIE_Entry* entry, uint64_t offset) {
    CIEO_arr_add_sorted_i(&CIEs, (CIEAndOffset) {.entry= entry, .offset= offset});
}

void frame_info_init() {
    CIEs= CIEO_arr_create();
    FDEs= FDE_arr_create();

    create_matrix();
}

void frame_info_destroy() {
    CIEO_arr_destroy(&CIEs);
    FDE_arr_destroy(&FDEs);
}

typedef struct FrameInfoHdr {
    uint8_t version;
    PointerEncoding eh_frame_ptr_encoding;
    PointerEncoding fde_count_encoding;
    PointerEncoding table_encoding;

    Pointer eh_frame_ptr;
    Pointer fde_count;
} FrameInfoHdr;
FrameInfoHdr header;

int parse_frame_info_hdr(Section* hdr) {
    uint8_t* base= hdr->data;

    header.version= raa_uint(&base, 1);
    header.eh_frame_ptr_encoding= raa_pointer_encoding(&base);
    header.fde_count_encoding= raa_pointer_encoding(&base);
    header.table_encoding= raa_pointer_encoding(&base);

    header.eh_frame_ptr= raa_pointer_value_from_PE(&base, header.eh_frame_ptr_encoding, hdr, hdr, 0);
    header.fde_count= raa_pointer_value_from_PE(&base, header.fde_count_encoding, hdr, hdr, 0);

    // todo the search table

    return SUCCESS;
}

int parse_frame_info(Section* section, Section* hdr) {
    parse_frame_info_hdr(hdr);

    frame_info_init();

    uint8_t* base= section->data;
    while (base < section->data + section->header->sh_size) {
        const int res= read_header(&base, section->data);
        if (res != SUCCESS) return res;
    }

    return SUCCESS;
}

int read_header(uint8_t** start, uint8_t* section_start) {
    uint64_t length;
    MODE mode;
    uint8_t* base= *start;

    base= read_initial_length(base, &length, &mode);
    if (length == 0) {
        *start= base;
        return SUCCESS;
    }
    uint8_t* content_base= base;
    uint8_t* header_end= content_base + length;

    bool is_cie= false;
    int64_t fde_offset= 0;

    if (mode == MODE_32bit) {
        int32_t cie_id;
        RAA(cie_id);

        if (cie_id == 0) {
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

    int res= SUCCESS;
    if (is_cie) {
        CIE_Entry* cie= malloc(sizeof(CIE_Entry));
        *cie= (CIE_Entry) {.length= length, .offset_location= *start-section_start};
        res= read_cie_entry(&base, mode, header_end, cie);

        if (res != SUCCESS) {
            free(cie);
            return res;
        }

        add_cie_entry(cie, (uintptr_t)*start);
        print_cie(cie);
    } else {
        CIE_Entry* cie= get_cie_entry_from_offset((uintptr_t)*start - fde_offset + 4);
        FDE_Entry* fde= FDE_arr_add_i(&FDEs);
        *fde= (FDE_Entry) {.length= length, .cie_entry= cie, .offset_location= (uint64_t)start};
        res= read_fde_entry(*start, base, header_end, section_start, fde);

        print_fde(fde);
    }

    *start= header_end;

    return res;
}

// https://refspecs.linuxbase.org/LSB_3.1.0/LSB-Core-generic/LSB-Core-generic/ehframechpt.html
int read_cie_entry(uint8_t** start, MODE mode, uint8_t* header_end, CIE_Entry* entry) {
    uint8_t* base= *start;

    RAA(entry->version);

    uint8_t* augmentation= raa_null_term_string(&base);

    entry->aug_data= (struct AugData) {
        .has_fde_L_pointer_encoding= false,
        .has_fde_address_pointer_encoding= false,
        .has_personality_routine_handler= false,
        .type= AUG_DATA_NONE
    };

    uint8_t* aug_eatable= augmentation;
    uint32_t fchr= raa_utf8(&aug_eatable);

    entry->code_alignment_factor= raa_uleb128(&base).v;
    entry->data_alignment_factor= raa_leb128(&base).v;
    entry->return_address_register= raa_uleb128(&base).v;

    uint8_t* aug_data= base;

    if (fchr == '\0') {
      entry->aug_data.type= AUG_DATA_NONE;
    } else if (strcmp((const char*)augmentation, DW_AUG_STARTER_EH_STR) == 0) {
        // this is an old augmentation string that means
        //  there is some data postceding

        // the documentation on the eh data is as follows
        //On 32-bit architectures, this is a 4 byte value that... On 64-bit architectures, this is an 8 byte value that... This field is only present if the Augmentation String contains the string "eh".
        // that is to say, there isn't any
        uint8_t eh_data_bytes= mode == MODE_32bit ? 4 : 8;

        entry->aug_data.type= AUG_DATA_EH;
        entry->aug_data.eh_data= raa_uint(&base, eh_data_bytes);

    } else if (fchr == DW_AUG_STARTER_CHR) {
        // this character is required for there to be data

        //todo: use this
        ULEB128 aug_len= raa_uleb128(&aug_data);

        uint32_t chr;
        while (chr= raa_utf8(&aug_eatable), chr != '\0') {
            // this augmentation data area is utf-8 according to the specification
            // Sec. 6.4.1 of v5, It seems that all producers just use z/eh, PLR,
            // and are all encoded just as ascii (utf-8 1 bytes), so full parsing
            // probably isn't needed, but I will use it here
            switch (chr) {
                case DW_AUG_OPTION_LSDA_CHR: {
                    // there is an argument in BOTH the CIE (this) and the FDE
                    entry->aug_data.has_fde_L_pointer_encoding= true;
                    entry->aug_data.L_pointer_encoding= raa_pointer_encoding(&aug_data);
                    break;
                }
                case DW_AUG_OPTION_PERSONAILTY_ROUTINE_CHR: {
                    // there are TWO arguments in the CIE (this)
                    entry->aug_data.has_personality_routine_handler= true;
                    entry->aug_data.P_pointer_encoding= raa_pointer_encoding(&aug_data);
                    entry->aug_data.P_personality_routine_handler= raa_pointer_value_from_PE(
                        &aug_data,
                        entry->aug_data.P_pointer_encoding,
                        &ELF.section_map.eh_frame,
                        &ELF.section_map.data,
                        0
                    );
                    break;
                }
                case DW_AUG_OPTION_ADDRESS_POINTER_CHR: {
                    // There is an argument in the CIE (this) and describes the address pointers in the FDEs
                    entry->aug_data.has_fde_address_pointer_encoding= true;
                    entry->aug_data.R_pointer_encoding= raa_pointer_encoding(&aug_data);
                    break;
                }
                default:
                    break;
            }
        }
    }

    base= aug_data;

    // RAA(entry->address_size);
    // RAA(entry->segment_selector_size);

    entry->initial_instructions= base;
    entry->instructions_size= header_end - base;

    return SUCCESS;
}

int read_fde_entry(
    uint8_t* header_base,
    uint8_t* base,
    const uint8_t* end,
    const uint8_t* section_start,
    FDE_Entry* entry
) {
    if (entry->cie_entry->segment_selector_size != 0) {
        uint64_t segment_selector= raa_uint(&base, entry->cie_entry->segment_selector_size);
        log("Decoding of Frame info FDE entry uses segment selector %lu (is this an old program?)\n", segment_selector);

        entry->segment_selector= segment_selector;
    }

    const PointerEncoding encoding= entry->cie_entry->aug_data.R_pointer_encoding;

    uint8_t* init_loc_base= base;
    entry->initial_location= raa_pointer_value_from_PE(&base, encoding, &ELF.section_map.eh_frame, &ELF.section_map.eh_frame_hdr, 0);
    entry->address_range= raa_pointer_value_without_app(&base, encoding);

    entry->instructions= base;
    entry->instructions_size= end - base;

    //init_loc_base - section_start + 0x000020d8 + -4088

    // while (base < end) {
    //     Instruction instr= decode_op(&base, entry->cie_entry);
    // }

    return SUCCESS;
}

uint64_t read_address(uint8_t** base_ptr, CIE_Entry* cie) {
    uint8_t addr_size= cie->address_size;

    return raa_uint(base_ptr, addr_size);
}

typedef union RRData {
    int64_t offset;
    uint8_t reg;
    DW_EXPR expr;
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
    DW_EXPR expr;
    struct {
        uint8_t register_id;
        int64_t offset;
    } reg_off;
} CFAData;

typedef enum CFADataType {
    CFA_DT_EXPRESSION,
    CFA_DT_REG_OFF
} CFADataType;

typedef struct CFARule {
    CFADataType type;
    CFAData data;
} CFARule;

typedef struct MRow {
    uint64_t address;
    CFARule cfa_rule;
    RegisterDataArray register_rules;
} FrameRow;

ARRAY_PROTO(FrameRow, FrameRow)
ARRAY_ADD(FrameRow, FrameRow)

typedef FrameRowArray Matrix;

static Matrix matrix;
static FrameRowArray row_stack;

void create_matrix() {
    if (matrix.arr) FrameRow_arr_destroy(&matrix);
    matrix= FrameRow_arr_create();
    add_row(0);
    row_stack= FrameRow_arr_create();
}

void add_row(uint64_t address) {
    FrameRow row= (FrameRow) {
        .address= address,
        .register_rules= RegisterData_arr_create()
    };

    FrameRow_arr_add(&matrix, row);
}

void add_row_copied(const uintptr_t address) {
    FrameRow* prev= FrameRow_arr_peek(&matrix);
    if (!prev) assert(false);

    FrameRow row= (FrameRow) {
        .address= address,
        .register_rules= RegisterData_arr_construct(prev->register_rules.pos),
        .cfa_rule= prev->cfa_rule
    };

    for (int i = 0; i < prev->register_rules.pos; ++i) {
        RegisterData_arr_add(&row.register_rules, RegisterData_arr_get(&prev->register_rules, i));
    }

    FrameRow_arr_add(&matrix, row);
}

RegisterData* get_register_data(uint8_t register_id) {
    FrameRow* c_row= FrameRow_arr_peek(&matrix);

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
    FrameRow* c_row= FrameRow_arr_peek(&matrix);

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
        Instruction instr= decode_op(&op, cie);
        uint8_t reg= instr.data.register_id;

        if (reg == (uint8_t)-1) {
            // this is an instruction that does not contain register information
            continue;
        }

        if (reg == register_id) {
            // this is a matching instruction and so we should execute it
            execute_op(instr, cie);
            break;
        }
    }
}

Instruction decode_op(uint8_t** start, const CIE_Entry* cie) {
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

    *start += 1;

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
            instr.data.d_delta= raa_uint(start, 1) * code_align;
            break;
        case DW_CFA_advance_loc2:
            instr.data.d_delta= raa_uint(start, 2) * code_align;
            break;
        case DW_CFA_advance_loc4:
            instr.data.d_delta= raa_uint(start, 4) * code_align;
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
            DW_EXPR expr= raa_expr(start);

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
            instr.data.d_offset= off.v * data_align;

            break;
        }

        case DW_CFA_offset_extended:
        case DW_CFA_val_offset: {
            ULEB128 reg= raa_uleb128(start);
            ULEB128 off= raa_uleb128(start);

            instr.data.register_id= reg.v;
            instr.data.d_offset= off.v * data_align;

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
            DW_EXPR expr= raa_expr(start);

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

    return instr;
}

void execute_op(Instruction instr, const CIE_Entry* cie) {
    switch (instr.opcode) {
        case DW_CFA_set_loc: {
            add_row_copied(instr.data.d_addr);
            break;
        }

        case DW_CFA_advance_loc:
        case DW_CFA_advance_loc1:
        case DW_CFA_advance_loc2:
        case DW_CFA_advance_loc4: {
            const FrameRow* c_row= FrameRow_arr_peek(&matrix);
            const uint64_t n_addr= instr.data.d_delta + c_row->address;
            add_row_copied(n_addr);

            break;
        }

        case DW_CFA_def_cfa:
        case DW_CFA_def_cfa_sf: {
            FrameRow* c_row= FrameRow_arr_peek(&matrix);

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
            FrameRow* c_row= FrameRow_arr_peek(&matrix);

            if (c_row->cfa_rule.type != CFA_DT_REG_OFF) {
                log("Decoding DWARF frame info provided invalid decoded instructions. DW_CFA_def_cfa_register expects cfa data type `reg-off` got `expression`\n");
            }

            c_row->cfa_rule.data.reg_off.register_id= instr.data.d_register;
            break;
        }

        case DW_CFA_def_cfa_offset:
        case DW_CFA_def_cfa_offset_sf: {
            FrameRow* c_row= FrameRow_arr_peek(&matrix);

            if (c_row->cfa_rule.type != CFA_DT_REG_OFF) {
                log("Decoding DWARF frame info provided invalid decoded instructions. DW_CFA_def_cfa_offset.* expects cfa data type `reg-off` got `expression`\n");
            }

            c_row->cfa_rule.data.reg_off.offset= instr.data.d_offset;
            break;
        }

        case DW_CFA_def_cfa_expression: {
            FrameRow* c_row= FrameRow_arr_peek(&matrix);

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
                (RRData){0}
            );

            break;
        }

        case DW_CFA_same_value: {
            set_register(
                instr.data.register_id,
                DW_CFA_same_value,
                (RRData){0}
            );

            break;
        }

        case DW_CFA_offset:
        case DW_CFA_offset_extended:
        case DW_CFA_offset_extended_sf: {
            set_register(
                instr.data.register_id,
                DW_CFA_offset,
                (RRData){.offset= instr.data.d_offset}
            );

            break;
        }

        case DW_CFA_val_offset:
        case DW_CFA_val_offset_sf: {
            set_register(
                instr.data.register_id,
                DW_CFA_val_offset,
                (RRData){.offset= instr.data.d_offset}
            );

            break;
        }

        case DW_CFA_register: {
            set_register(
                instr.data.register_id,
                DW_CFA_register,
                (RRData){.reg= instr.data.d_register}
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
                (RRData){.expr= instr.data.d_expr}
            );

            break;
        }

        case DW_CFA_restore:
        case DW_CFA_restore_extended: {
            execute_initial_instr_for(instr.data.register_id, cie);

            break;
        }

        case DW_CFA_remember_state: {
            FrameRow_arr_add(&row_stack, *FrameRow_arr_peek(&matrix));

            break;
        }
        case DW_CFA_restore_state: {
            FrameRow popped= FrameRow_arr_pop(&row_stack);
            FrameRow* c_row= FrameRow_arr_peek(&matrix);
            uint64_t c_addr= c_row->address;

            *c_row= (FrameRow) {
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

void decode_and_execute_op(uint8_t** start, const CIE_Entry* cie) {
    const Instruction instr= decode_op(start, cie);
    execute_op(instr, cie);
}

void decode_and_execute_ops(uint8_t* base, uint64_t size, const CIE_Entry* cie) {
    const uint8_t* end= base + size;
    while (base < end) {
        decode_and_execute_op(&base, cie);
    }

    return;
}

void create_matrix_of(const FDE_Entry* fde) {
    create_matrix();

    decode_and_execute_ops(fde->cie_entry->initial_instructions, fde->cie_entry->instructions_size, fde->cie_entry);
    FrameRow* first= FrameRow_arr_ptr(&matrix, 0);
    first->address= fde->initial_location.ptr_s;
    decode_and_execute_ops(fde->instructions, fde->instructions_size, fde->cie_entry);
}

uint64_t cfa_value_at(uintptr_t pc) {
    
}

FDE_Entry* get_fde_for_pc(const uintptr_t pc) {
    if (!pointer_is_omit(header.table_encoding)) {
        printf("Test\n");
    }

    for (int i = 0; i < FDEs.pos; ++i) {
        FDE_Entry* fde= FDE_arr_ptr(&FDEs, i);
        if (fde->initial_location.value <= pc && fde->initial_location.value + fde->address_range.value <= pc) {
            return fde;
        }
    }

    return NULL;
}

const char* instruction_op_str(FI_OPCODE op) {
    switch (op) {
        case DW_CFA_set_loc: return "DW_CFA_set_loc";
        case DW_CFA_advance_loc: return "DW_CFA_advance_loc";
        case DW_CFA_advance_loc1: return "DW_CFA_advance_loc1";
        case DW_CFA_advance_loc2: return "DW_CFA_advance_loc2";
        case DW_CFA_advance_loc4: return "DW_CFA_advance_loc4";
        case DW_CFA_def_cfa: return "DW_CFA_def_cfa";
        case DW_CFA_def_cfa_sf: return "DW_CFA_def_cfa_sf";
        case DW_CFA_def_cfa_register: return "DW_CFA_def_cfa_register";
        case DW_CFA_def_cfa_offset: return "DW_CFA_def_cfa_offset";
        case DW_CFA_def_cfa_offset_sf: return "DW_CFA_def_cfa_offset_sf";
        case DW_CFA_def_cfa_expression: return "DW_CFA_def_cfa_expression";
        case DW_CFA_undefined: return "DW_CFA_undefined";
        case DW_CFA_same_value: return "DW_CFA_same_value";
        case DW_CFA_offset: return "DW_CFA_offset";
        case DW_CFA_offset_extended: return "DW_CFA_offset_extended";
        case DW_CFA_offset_extended_sf: return "DW_CFA_offset_extended_sf";
        case DW_CFA_val_offset: return "DW_CFA_val_offset";
        case DW_CFA_val_offset_sf: return "DW_CFA_val_offset_sf";
        case DW_CFA_register: return "DW_CFA_register";
        case DW_CFA_expression: return "DW_CFA_expression";
        case DW_CFA_val_expression: return "DW_CFA_val_expression";
        case DW_CFA_restore: return "DW_CFA_restore";
        case DW_CFA_restore_extended: return "DW_CFA_restore_extended";
        case DW_CFA_remember_state: return "DW_CFA_remember_state";
        case DW_CFA_restore_state: return "DW_CFA_restore_state";
        case DW_CFA_nop: return "DW_CFA_nop";
        default: return "<<ERROR>> Invalid op code <<ERROR>>";
    }
}

void print_instruction(Instruction* instr, const FDE_Entry* fde) {
    printf("Instr (%s)", instruction_op_str(instr->opcode));
    if (fde && fde->cie_entry == NULL) printf("<<ERROR>> FDE's CIE entry is invalid unable to print some data <<ERROR>>");
    switch (instr->opcode) {
        case DW_CFA_set_loc:
            printf(": Create new table row with location");
            if (fde && fde->cie_entry && fde->segment_selector != 0) printf("SS%lu:", fde->segment_selector);
            printf("%lu", instr->data.d_addr);
            break;

        case DW_CFA_advance_loc:
        case DW_CFA_advance_loc1:
        case DW_CFA_advance_loc2:
        case DW_CFA_advance_loc4:
            printf(": Create new table row with delta %u", instr->data.d_delta);
            break;

        case DW_CFA_def_cfa:
        case DW_CFA_def_cfa_sf:
            printf(": Set CFA Rule to Register %u + Offset %ld", instr->data.register_id, instr->data.d_offset);
            break;

        case DW_CFA_def_cfa_register:
            printf(": Set CFA Rule to Register %u + Offset; Keep old offset", instr->data.d_register);
            break;

        case DW_CFA_def_cfa_offset:
        case DW_CFA_def_cfa_offset_sf:
            printf(": Set CFA Rule to Register + Offset %ld; Keep old register", instr->data.d_offset);
            break;

        case DW_CFA_def_cfa_expression:
            printf(": Set CFA to expression:  ");
            print_expression(&instr->data.d_expr);
            break;

        case DW_CFA_undefined:
            printf(": Set Register %u to undefined", instr->data.register_id);
            break;

        case DW_CFA_same_value:
            printf(": Set Register %u to same value", instr->data.register_id);
            break;

        case DW_CFA_offset:
        case DW_CFA_offset_extended:
        case DW_CFA_offset_extended_sf:
            printf(": Set Register %u to Offset %ld", instr->data.register_id, instr->data.d_offset);
            break;

        case DW_CFA_val_offset:
        case DW_CFA_val_offset_sf:
            printf(": Set Register %u to Value %ld", instr->data.register_id, instr->data.d_offset);
            break;

        case DW_CFA_register:
            printf(": Set Register %u to Register %u", instr->data.register_id, instr->data.d_register);
            break;

        case DW_CFA_expression:
            printf(": Set Register %u to Expression: ", instr->data.register_id);
            print_expression(&instr->data.d_expr);
            break;

        case DW_CFA_val_expression:
            printf(": Set Register %u to Value Expression: ", instr->data.register_id);
            print_expression(&instr->data.d_expr);
            break;

        case DW_CFA_restore:
        case DW_CFA_restore_extended:
            printf(": Restore value from initial instructions for Register %u", instr->data.register_id);
            break;

        case DW_CFA_remember_state:
            printf(": Push state to stack");
            break;

        case DW_CFA_restore_state:
            printf(": Pop state from stack");
            break;

        case DW_CFA_nop:
            printf(": Nop");
            break;
    }
}

void print_instructions(const char* prefix, uint8_t* instructions, uint64_t instructions_size, const CIE_Entry* entry, const FDE_Entry* fde) {
    uint8_t* base= instructions;

    if (entry == NULL) {
        printf("%sEntry %p has NULL CIE; unable to print instruction", prefix, entry);
        return;
    }

    while (base < instructions + instructions_size) {
        Instruction instr= decode_op(&base, entry);
        putz(prefix);
        print_instruction(&instr, fde);
        newline();
    }
}

void print_fde(const FDE_Entry* entry) {
    if (!entry) {
        printf("FDE Entry <<ERROR>> NULL <<ERROR>>\n");
        return;
    }
    printf("FDE Entry @%lx | %lx bytes\n", entry->offset_location, entry->length);
    if (entry->cie_entry == NULL) printf(" - Linked CIE: <ERROR> Invalid linked cie <ERROR>\n");
    else printf(" - Linked CIE: @%lx\n", entry->cie_entry->offset_location);

    printf(" - Addr range: ");
    if (entry->cie_entry && entry->cie_entry->segment_selector_size != 0) {
        printf("SS%lu:", entry->segment_selector);
    }
    printf("[");
    print_pointer(&entry->initial_location);
    printf(" range ");
    print_pointer(&entry->address_range);
    printf("]\n");
    printf(" - Instructions (%u bytes):\n", entry->instructions_size);
    print_instructions("      ", entry->instructions, entry->instructions_size, entry->cie_entry, entry);

    create_matrix_of(entry);
    print_matrix();
}

void print_cie(const CIE_Entry* entry) {
    if (!entry) {
        printf("FDE Entry <<ERROR>> NULL <<ERROR>>\n");
        return;
    }
    printf("CIE Entry v.%u | %lx bytes\n", entry->version, entry->length);
    printf(" - Addr size: %u\n", entry->address_size);
    printf(" - Alignments: \n");
    printf("    - Code: %lu\n", entry->code_alignment_factor);
    printf("    - Data: %ld\n", entry->data_alignment_factor);
    printf(" - Return address register: %lu\n", entry->return_address_register);
    printf(" - Segment selector size: %u\n", entry->segment_selector_size);
    printf(" - Initial instructions (%u bytes):\n", entry->instructions_size);
    print_instructions("      ", entry->initial_instructions, entry->instructions_size, entry, NULL);
}

void print_cfa_rule(const CFARule* cfa) {
    switch (cfa->type) {
        case CFA_DT_EXPRESSION:
            print_expression(&cfa->data.expr);
            break;
        case CFA_DT_REG_OFF:
            printf("R%u + %ld", cfa->data.reg_off.register_id, cfa->data.reg_off.offset);
            break;
        default: assert(false);
    }
}

void print_register_data(const RegisterData* data) {
    printf("R%u= ", data->register_id);
    switch (data->rule.op) {
        case DW_CFA_undefined: printf("undefined"); break;
        case DW_CFA_same_value: printf("same value"); break;
        case DW_CFA_offset: printf("@(cfa + %ld)", data->rule.data.offset); break;
        case DW_CFA_val_offset: printf("cfa + %ld (val)", data->rule.data.offset); break;
        case DW_CFA_register: printf("R%u", data->rule.data.reg); break;
        case DW_CFA_expression: {
            // todo
            printf("@(Expr)");
            break;
        }
        case DW_CFA_val_expression: {
            printf("Expr (val)");
            break;
        }
        default: assert(false);
    }
}

void print_matrix() {
    for (int i = 0; i < matrix.pos; ++i) {
        const FrameRow* row= FrameRow_arr_ptr(&matrix, i);
        printf("@%#lX: ", row->address);
        printf(" <CFA= ");
        print_cfa_rule(&row->cfa_rule);
        printf(">");

        for (int j = 0; j < row->register_rules.pos; ++j) {
            RegisterData* rule= RegisterData_arr_ptr(&row->register_rules, j);
            printf(" <");
            print_register_data(rule);
            printf(">");
        }
        newline();
    }
}
