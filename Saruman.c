//
// Created by james on 29/09/25.
//

#include "Saruman.h"

#include "Sauron.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

const LC LC_ERR= {-1, -1};
const ARange ARange_ERR= {-1, -1};

static uint64_t pc;
static uint64_t op_idx;

static uint32_t file;
static uint32_t line;
static uint32_t col;

static bool is_stmt;
static bool basic_block;
static bool end_seq;
static bool prologue_end;
static bool epilogue_begin;

static uint32_t isa;
static uint32_t discrim;

// EXTENDED OPCODES
typedef enum DW_LNE {
    DW_LNE_end_sequence= 0x1,
    DW_LNE_set_address,
    DW_LNE_reserved= 0x3,
    DW_LNE_set_discriminator= 0x4,
    DW_LNE_COUNT= DW_LNE_set_discriminator,

    DW_LNE_lo_user= 0x80,
    DW_LNE_hi_user= 0xFF
} DW_LNE;

typedef enum DW_LNCT {
    DW_LNCT_path= 1,
    DW_LNCT_directory_index,
    DW_LNCT_timestamp,
    DW_LNCT_size,
    DW_LNCT_MD5,
    DW_LNCT_COUNT= DW_LNCT_MD5,

    DW_LNCT_lo_user= 0x2000,
    DW_LNCT_hi_user= 0x3fff
} DW_LNCT;

const char* DW_LNCT_STRS[]= {
    [DW_LNCT_path]= "path",
    [DW_LNCT_directory_index]= "dir idx",
    [DW_LNCT_timestamp]= "timestamp",
    [DW_LNCT_size]= "size",
    [DW_LNCT_MD5]= "MD5"
};

// STANDARD OPCODES
typedef enum DW_LNS {
    DW_LNS_copy= 1,
    DW_LNS_advance_pc,
    DW_LNS_advance_line,
    DW_LNS_set_file,
    DW_LNS_set_column,
    DW_LNS_negate_stmt,
    DW_LNS_set_basic_block,
    DW_LNS_const_add_pc,
    DW_LNS_fixed_advance_pc,
    DW_LNS_set_prologue_end,
    DW_LNS_set_epilogue_begin,
    DW_LNS_set_isa,
    DW_LNS_COUNT= DW_LNS_set_isa
} DW_LNS;

const char* DW_LNS_STRS[]= {
    [DW_LNS_copy]= "copy",
    [DW_LNS_advance_pc]= "advance_pc",
    [DW_LNS_advance_line]= "advance_line",
    [DW_LNS_set_file]= "set_file",
    [DW_LNS_set_column]= "set_column",
    [DW_LNS_negate_stmt]= "negate_stmt",
    [DW_LNS_set_basic_block]= "set_basic_block",
    [DW_LNS_const_add_pc]= "const_add_pc",
    [DW_LNS_fixed_advance_pc]= "fixed_advance_pc",
    [DW_LNS_set_prologue_end]= "set_prologue_end",
    [DW_LNS_set_epilogue_begin]= "set_epilogue_begin",
    [DW_LNS_set_isa]= "set_isa"
};

#include "Array.h"

ARRAY_PROTO(uint8_t, ubyte)
ARRAY_ADD(uint8_t, ubyte)

typedef struct DW_LInfo64 {
    union {
        uint32_t ul_32;
        uint64_t ul_64;
    };
    uint16_t version;
    uint8_t a_size;
    uint8_t seg_sel_size;
    union {
        uint32_t hl_32;
        uint64_t hl_64;
    };
    uint8_t min_instr_length;
    uint8_t max_op_per_inst;
    uint8_t default_is_stmt;
    int8_t line_base;
    uint8_t line_range;
    uint8_t op_base;

    ubyteArray opcode_lengths;

    uint8_t directory_entry_format_count;
    uint8_t* directory_entry_format; // ULEB128 pairs
    uint64_t directories_count; // THIS IS ULEB128 encoded in the file
    uint8_t* directories; // The format of which is described in _entry_format
    // The first entry is the CD

    uint8_t file_name_entry_format_count;
    uint8_t* file_name_entry_format; // ULEB128 pairs
    uint64_t file_names_count; // THIS IS ULEB128 encoded in the file
    uint8_t* file_names;
} DW_LInfo64;

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


#define READ_AND_ADV(type, s) \
    *(type*)s; s+= sizeof (type);

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

DW_LInfo64 info;
uint8_t* text_data= NULL;
uint64_t text_off= 0;

typedef struct MRow {
    uint32_t line;
    uint32_t col;

    uint64_t pc;
} MRow;

ARRAY_PROTO(MRow, MRow)
ARRAY_ADD(MRow, MRow)

MRowArray matrix;

void peek_text_at_addr(uintptr_t addr, uint16_t amount) {
    for (int j = 0; j < amount; ++j) {
        printf("%02X ", text_data[j + addr - text_off]);
        if (j % 16 == 15) putchar('\n');
    }
    putchar('\n');
}

MRow last_row= {0};
MRow row= {0};

void add_new_row_to_matrix() {
    MRow_arr_add(&matrix, row);
}

// this is the default, overwritten if we are are using the header version
void (*on_new_row)()= add_new_row_to_matrix;
void new_row() {
    // 3. Append a row to the matrix using the current values of the state machine
    //   registers.
    // [[TODO]]
    printf("MATRIX ROW line: %u  Col: %u  PC: %p  Discrim: %u\n",
        line, col, (void*)pc, discrim);

    last_row= row;
    row= (MRow){
        .line= line,
        .col= col,
        .pc= pc
    };
    if (on_new_row) on_new_row();

    // printf("Peek at PC: \n");
    // peek_text_at_addr(pc, 128);

    // 4. Set the basic_block register to “false.”
    // 5. Set the prologue_end register to “false.”
    // 6. Set the epilogue_begin register to “ false.” 19
    // 7. Set the discriminator register to 0.
    basic_block= prologue_end= epilogue_begin= false;
    discrim= 0;
}

void addr_op_inc(uint8_t aoc) {
    uint8_t op_adv= aoc / info.line_range;

    uint64_t npc= pc + (info.min_instr_length * ((op_idx + op_adv) / info.max_op_per_inst));
    uint64_t n_op_idx= (op_idx + op_adv) % info.max_op_per_inst;

    // 2. Modify the operation pointer by incrementing the address and op_index
    //  registers as described below.
    op_idx= n_op_idx;
    pc= npc;
}

int special_op(uint8_t op) {
    // this is a special opcode
    uint8_t aoc= op - info.op_base;
    addr_op_inc(aoc);

    uint32_t line_inc= info.line_base + (aoc % info.line_range);

    // FROM https://dwarfstd.org/doc/DWARF5.pdf#page=178
    // 1. Add a signed integer to the line register
    line += line_inc;

    // 3-7
    new_row();

    // all special opcodes are 1 byte
    return 1;
}

// returns bytes read
size_t _decode_op(uint8_t* base) {
    uint8_t c= *base;
    base++;
    if (c == 0) {
        // this is an extended opcode
        ULEB128 bytes= read_uleb128(base);
        base += bytes.size;
        uint8_t op_code= *base;
        base++;
        switch (op_code) {
            case DW_LNE_end_sequence:
                end_seq= true;
                new_row();
                break;
            case DW_LNE_set_address:
                switch (info.a_size) {
                    case 4:
                        pc= *(uint32_t*)base;
                        break;
                    case 8:
                        pc= *(uint64_t*)base;
                        break;
                    default:
                        assert(false);
                }
                break;
            case DW_LNE_set_discriminator: {
                ULEB128 operand = read_uleb128(base);
                discrim = operand.v;
                break;
            }
            default:
                assert(false);
        }
        return bytes.v + bytes.size + 1;
    }
    if (c < info.op_base) {
        // this is a standard opcode
        switch (c) {
            case DW_LNS_copy:
                new_row();
                return 1;
            case DW_LNS_const_add_pc:
                addr_op_inc(255 - info.op_base);
                return 1;
            case DW_LNS_advance_pc:
             {
                ULEB128 operand= read_uleb128(base);
                uint64_t op_adv= operand.v;
                uint8_t op_size= operand.size + 1;

                pc += (info.min_instr_length * ((op_idx + op_adv) / info.max_op_per_inst));
                op_idx= (op_idx + op_adv) % info.max_op_per_inst;
                return op_size; // the operand size + the opcode
            }
            case DW_LNS_advance_line: {
                LEB128 operand= read_leb128(base);
                line += operand.v;
                return operand.size + 1;
            }
            case DW_LNS_set_file: {
                ULEB128 operand= read_uleb128(base);
                file= operand.v;
                return operand.size + 1;
            }
            case DW_LNS_set_column: {
                ULEB128 operand= read_uleb128(base);
                col= operand.v;
                return operand.size + 1;
            }
            case DW_LNS_negate_stmt:
                is_stmt = !is_stmt;
                return 1;
            case DW_LNS_set_basic_block:
                basic_block= true;
                return 1;
            case DW_LNS_fixed_advance_pc: {
                uint16_t operand= *(uint16_t*)base;
                pc += (op_idx + operand) / info.max_op_per_inst;
                op_idx= 0;
                return 2 + 1;
            }
            case DW_LNS_set_prologue_end:
                prologue_end= true;
                return 1;
            case DW_LNS_set_epilogue_begin:
                epilogue_begin= true;
                return 1;
            case DW_LNS_set_isa: {
                ULEB128 operand= read_uleb128(base);
                isa= operand.v;
                return operand.size + 1;
            }
            default:
                assert(false);
        }
    }

    return special_op(c);
}

DecodeRet decode_op(uint8_t* base) {
    return (DecodeRet) {
        .bytes_read= _decode_op(base),
        .end_of_code= end_seq
    };
}

int read_header(uint8_t* start, char* string_data) {
    uint8_t* base= start;

    void* bc_start; // byte code start
    void* bc_end;

    bool bit64= false;
    // The base is a sequence of bytes that can be read at the start
    RAA(info.ul_32);
    if (info.ul_32 == 0xFFFFFFFF) {
        bit64= true;
        RAA(info.ul_64);
    }
    bc_end= (void*)(base + (uint64_t)(bit64 ? info.ul_64 : info.ul_32));

    // THE HEADER INFORMATION IS DIFFERENT BETWEEN VERSIONS
    RAA(info.version);

    if (info.version >= 5) {
        RAA(info.a_size);
        RAA(info.seg_sel_size);
    } else {
        // we have to fetch the CU's address size from the .debug_info section
        info.a_size= 8;
        info.seg_sel_size= 0; // we assume no segment selector for now, although a dwarf version this old may be a 16-bit platform
    }

    RAA(info.hl_32)
    if (info.hl_32 == 0xFFFFFFFF) {
        RAA(info.hl_64);
    }

    bc_start= (void*)(base + (uint64_t)(bit64 ? info.hl_64 : info.hl_32));
    printf("ByteCode starts at addr %p from base of %p\n", bc_start, start);

    RAA(info.min_instr_length);
    if (info.version >= 4) {
        RAA(info.max_op_per_inst);
    } else {
        info.max_op_per_inst= 1;
    }

    RAA(info.default_is_stmt);
    RAA(info.line_base);
    RAA(info.line_range);
    RAA(info.op_base);


    // The rest we have to create
    size_t standard_opcodes= info.op_base - 1;
    info.opcode_lengths= ubyte_arr_construct(standard_opcodes);
    for (size_t i= 0; i < standard_opcodes; ++i) {
        ULEB128 res= read_uleb128(base);
        base += res.size;
        ubyte_arr_add(&info.opcode_lengths, res.v);
    }

    printf("There are %zu standard op codes\n", info.opcode_lengths.pos);
    for (size_t i= 0; i < info.opcode_lengths.pos; ++i) {
        uint8_t size= ubyte_arr_get(&info.opcode_lengths, i);

        // opcode names start at 1
        printf(" Op %s %.2zu has %u operand(s)\n",
            DW_LNS_STRS[i + 1], i + 1, size);
    }

    matrix= MRow_arr_create();
    create_header();
    while (true) {
        DecodeRet res= decode_op(bc_start);
        bc_start+= res.bytes_read;

        if (res.end_of_code) break;
    }

    for (int i= 26; i < 50; ++i) {
        ARange addr_r= line2addr(i);
        printf("line[%d] is addr: %p-%p %s\n", i, (void*)addr_r.s, (void*)addr_r.e, addr_r.s == -1 ? "(NO LINE)" : "\nWith peek: ");
        if (addr_r.s != -1) peek_text_at_addr(addr_r.s, addr_r.e - addr_r.s);
    }

    if (info.version < 5) {
        return 0;
    }

    // for now ignore this part of DWARF v5
    RAA(info.directory_entry_format_count)
    for (int i= 0; i < info.directory_entry_format_count; ++i) {
        ULEB128 l= read_uleb128(base);
        base += l.size;

        ULEB128 r= read_uleb128(base);
        base += r.size;

        printf("Dir entries contain %s as a %s\n", DW_LNCT_STRS[l.v], DW_FORM_STRS[r.v]);
    }

    ULEB128 dfc= read_uleb128(base);
    base += dfc.size;

    printf("There are %lu directories.\n", dfc.v);
    for (size_t i= 0; i < dfc.v; ++i) {
        uint32_t off; // todo this offset can be uint64_t (bit64 mode)
        RAA(off);
        printf("[%zu] is called %s\n", i, &string_data[off]);
    }
    return 0;

    RAA(info.file_name_entry_format_count)
    for (int i= 0; i < info.file_name_entry_format_count; ++i) {
        ULEB128 l= read_uleb128(base);
        base += l.size;

        ULEB128 r= read_uleb128(base);
        base += r.size;
    }

    ULEB128 fnc= read_uleb128(base);
    base += fnc.size;

    info.file_names_count= fnc.v;
}

void init() {
    pc= op_idx= 0;
    file= line= 1;
    col= 0;
    is_stmt= false; // THIS SHOULD BE LINKED TO default_is_stmt in PROG HEADER
    basic_block= end_seq= prologue_end= epilogue_begin= false;
    isa= discrim= 0;
}


// int s_op(S_OP op) {
//     basic_block= prologue_end= epilogue_begin= false;
//     discrim= 0;
//
//     DW_LNS_advance_pc
// }

int decode_lines(uint8_t* start, void* string_data, void* t_data, uint64_t t_off) {
    init();

    text_data= t_data;
    text_off= t_off;

    read_header(start, string_data);

    return 0;
}

ARange line2addr(uint32_t line) {
    for (int i= 0; i < matrix.pos; ++i) {
        MRow* row= MRow_arr_ptr(&matrix, i);
        if (i == matrix.pos - 1) {
            if (line == row->line) return (ARange){
                .s= row->pc,
                .e= row->pc
            };
            return ARange_ERR;
        }


        if (row->line == line) {
            int j= i;
            MRow* next;
            MRow* end;

            // todo this misses dis-joint lines
            //  to link them though would be n^2
            //  so more likely is a linear bucket putter-er
            while (next= MRow_arr_ptr(&matrix, ++j), next->line == line) {}
            end=next;

            return (ARange){
                .s= row->pc,
                .e= end->pc
            };
        }
    }

    return ARange_ERR;
}

// addresses and op indexes only increase
//  so finding the value of an address needs only
//  to find the first value greater than and then step back
//  once. If this is the start then there is no
LC addr2line(uintptr_t addr) {
    for (int i= 0; i < matrix.pos; ++i) {
        MRow* row= MRow_arr_ptr(&matrix, i);

        if (addr == row->pc) return (LC) {
            .line= row->line,
            .col= row->col
        };

        if (addr < row->pc) {
            if (i == 0) return LC_ERR;
            MRow* prev= MRow_arr_ptr(&matrix, i - 1);
            return (LC) {
                .line= prev->line,
                .col= prev->col
            };
        }
    }

    return LC_ERR;
}

void on_new_row_header();
LNInfo LN_info;
#define ASSUMED_LINE_COUNT 500
LNHeader create_header() {
    // we assume a lower bound of 500 lines to start (at worst we waste 1/2kb)
    void* data= calloc(ASSUMED_LINE_COUNT, sizeof (uint32_t));
    LN_info.header= (LNHeader) {
        .data= data,
        .lines= (uint32_t*)data + 1,
        .max_line= ASSUMED_LINE_COUNT
    };

    // this is the next free slot in the data entries
    //  the structure is 3 uint16_t's [[todo]] this could be ULEB but adding gets complicated
    //  being the count, the start, and the size
    LN_info.next_free= malloc(sizeof (uint16_t) * 3 * ASSUMED_LINE_COUNT);

    // [[todo]] could store the free parts of what is essentially a heap
    //  and fill them first. It is unlikely that there will be lots of gaps I'd wager

    on_new_row= on_new_row_header;
}

void on_new_row_header() {
    // we go through the line number program
    // as we get new rows we check if they are contiguous (same line as last)
    //  if they are we can just update the last data in the entry
    //  if they are not then we either add a new entry, or a new data entry
    if (row.line > LN_info.header.max_line) assert(false);

    if (last_row.line != 0) {
        if (last_row.line == row.line) {
            // we are contiguous
            uint32_t entry_off= LN_info.header.lines[row.line];

            // if we're the same as the previous line then it should
            //  already have an allocation area!
            if (entry_off == 0) assert(false);

            LNEntry* entry= &LN_info.entries[entry_off];
            if (entry->cc == 0) assert(false);

            LNData* child= &((LNData*)(entry + 1))[entry->cc - 1];
            child->size= row.pc - child->start_offset;

            printf("Consecutive row %u found and updated with new size\n", row.line);
        } else {
            // here we are not contigous
            //  and so we either add a new entry
            uint32_t entry_off= LN_info.header.lines[row.line];

            if (entry_off == 0) {
                // add a new entry
                LNEntry* entry= LN_info.next_free++;
                entry->cc= 1;
                *(LNData*)(entry + 1)= (LNData) {
                    .start_offset= row.pc,
                    .size= 1 // [[todo]] should this be the min instr size?
                };
            } else {
                // add another entry (>1) so we might have to move others
                // around
                LNEntry* entry= &LN_info.entries[entry_off];
                uint32_t neighbour_size= 0;
                if (entry->max_size < entry->cc + 1) {
                    // we have to take over the next line
                    LNEntry* neighbour_entry= entry + 1;
                    uint32_t neighbour_line= -1;

                    for (size_t i= row.line; i <= LN_info.header.max_line; ++i) {
                        if (LN_info.header.lines[i] == (uint64_t)neighbour_entry) {
                            neighbour_line= i;
                            break;
                        }
                    }

                    if (neighbour_line == -1) assert(false);

                    neighbour_size= 2 + 2 + 4 * neighbour_entry->cc;
                    memcpy(LN_info.next_free, neighbour_entry, neighbour_size);
                    LN_info.next_free+= neighbour_size;
                }

                ((LNData*)(entry + 1))[entry->cc - 1]= (LNData) {
                    .start_offset=row.pc,
                    .size= 1
                };
                entry->cc++;
                entry->max_size+= neighbour_size;
            }
        }
    } else {
        // this is a special case for the first row being added
        //  we just find the line entry and add the default data
        LNEntry* entry= LN_info.next_free;
        *entry= (LNEntry) {
            .cc= 1,
            .max_size= 1
        };

        *(LNData*)(entry + 1)= (LNData) {
            .start_offset= row.pc,
            .size= 1
        };

        LN_info.header.lines[row.line]= entry - LN_info.entries;

        LN_info.next_free+= sizeof(LNEntry) + sizeof(LNData) * 1;
    }
}
