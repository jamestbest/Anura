//
// Created by james on 16/02/26.
//

#include "Balin.h"

#include "Errors.h"
#include "shared/Array.h"

#include <stdbool.h>
#include <stdint.h>

#include "Helper_String.h"
#include "main.h"

ARRAY_ADD(ATData, ATData)
ARRAY_ADD(TagData, TagData)

void print_die(const DIE* die);
static int parse_cu_dies(uint8_t** data, const uint8_t* section_end, CU_HEADER* header, TableArray* abbrev_tables);
static const char* die_data_get_str(const FORM_DATA* data, DW_FORM form);

typedef enum ParseRes {
    PARSE_FAIL=-1,
    PARSE_SUCC_CONTINUE=0,
    PARSE_SUCC_END=1
} ParseRes;

Table create_table(uint64_t offset) {
    return (Table) {
        .abbrevs= TagData_arr_create(),
        .offset= offset
    };
}

TagData create_tag_data(const size_t id, const DW_TAG tag, const bool has_children) {
    return (TagData) {
        .id= id,
        .tag= tag,
        .has_children= has_children,
        .attributes= ATData_arr_create()
    };
}

ATData create_at_data(const DW_AT attr, const DW_FORM form) {
    return (ATData) {
        .attr= attr,
        .form= form,
        .impl_const= 0
    };
}

ParseRes parse_attribute(ATDataArray* arr, uint8_t** base) {
    const ULEB128 attr= raa_uleb128(base);
    const ULEB128 form= raa_uleb128(base);

    if (attr.v == 0 && form.v == 0) return PARSE_SUCC_END;

    ATData data= create_at_data(attr.v, form.v);

    if (form.v == DW_FORM_implicit_const) {
        const LEB128 value= raa_leb128(base);
        data.impl_const= value.v;
    }

    ATData_arr_add(arr, data);

    return PARSE_SUCC_CONTINUE;
}

ParseRes parse_tag(Table* table, uint8_t** base) {
    const ULEB128 code= raa_uleb128(base);
    if (code.v == 0) return PARSE_SUCC_END;

    const ULEB128 tag= raa_uleb128(base);
    const uint8_t has_child= raa_uint(base, 1);

    TagData data= create_tag_data(code.v, tag.v, has_child);

    ParseRes res;
    do {
        res= parse_attribute(&data.attributes, base);
    } while (res == PARSE_SUCC_CONTINUE);

    if (res == PARSE_FAIL) return PARSE_FAIL;

    TagData_arr_add(&table->abbrevs, data);
    return PARSE_SUCC_CONTINUE;
}

ARRAY_ADD(Table, Table)
int parse_table(uint8_t** start, const uint8_t* section_end, const uint8_t* section_start, TableArray* tables) {
    Table table= create_table(*start - section_start);
    while (*start < section_end) {
        const ParseRes res= parse_tag(&table, start);
        switch (res) {
            case PARSE_SUCC_END: goto end;
            case PARSE_SUCC_CONTINUE: continue;
            case PARSE_FAIL: return FAIL;
        }
    }

end:
    Table_arr_add(tables, table);
    return SUCCESS;
}

int parse_abbrev(Section* section, TableArray* tables) {
    uint8_t* section_start= section->data;
    const uint8_t* section_end= section_start + section->header->sh_size;

    uint8_t* data= section->data;
    while (data < section_end) {
        const int res= parse_table(&data, section_end, section_start, tables);
        if (res != SUCCESS) return FAIL;
    }

    print_abbrev_tables(tables);

    return SUCCESS;
}

int read_cu_header(uint8_t** start, CU_HEADER* header) {
    header->length= raa_initial_length(start, &header->mode);
    header->version= raa_uint(start, 2);

    if (header->version >= 5) {
        header->type= raa_uint(start, 1);
    }

    // crazy stuff changing order over versions >:[
    if (header->version >= 5) {
        header->address_size= raa_uint(start, 1);
        header->abbrev_offset= raa_offset_by_mode(start, header->mode);
    } else {
        header->abbrev_offset= raa_offset_by_mode(start, header->mode);
        header->address_size= raa_uint(start, 1);
    }

    return SUCCESS;
}

Table* find_table(TableArray* tables, uint64_t offset) {
    for (int i = 0; i < tables->pos; ++i) {
        Table* table= Table_arr_ptr(tables, i);

        if (table->offset == offset) return table;
    }
    return NULL;
}

ARRAY_ADD(DIE, DIE)
ARRAY_PROTO(uint64_t, u64)
ARRAY_ADD(uint64_t, u64)
ARRAY_ADD(FORM_DATA, DIEDATA)

static DIEArray dies;
static DIEArray cus;
static DIEArray subprogs;
static VSubArray vsubprogs;

const char* get_subprog_name(const DIE* subprog);

int parse_info(Section* section, TableArray* abbrev_tables) {
    dies= DIE_arr_create();
    cus= DIE_arr_create();
    subprogs= DIE_arr_create();
    vsubprogs= VSub_arr_create();

    uint8_t* section_start= section->data;
    const uint8_t* section_end= section_start + section->header->sh_size;

    uint8_t* data= section_start;
    while (data < section_end) {
        CU_HEADER header;
        int res= read_cu_header(&data, &header);
        if (res != SUCCESS) return res;

        res= parse_cu_dies(&data, section_end, &header, abbrev_tables);
        if (res != SUCCESS) return res;
    }

    return SUCCESS;
}

DIE* get_main_cu() {
    if (cus.pos == 0) return NULL;

    return DIE_arr_ptr(&cus, 0);
}

bool has_attr(const TagData* data, const DW_AT attr) {
    for (int i = 0; i < data->attributes.pos; ++i) {
        const ATData* a= ATData_arr_ptr(&data->attributes, i);

        if (a->attr == attr) return true;
    }
    return false;
}

int16_t get_attr_pos(const TagData* data, const DW_AT attr) {
    for (size_t i = 0; i < data->attributes.pos; ++i) {
        const ATData* a= ATData_arr_ptr(&data->attributes, i);

        if (a->attr == attr) return i;
    }
    return -1;
}

VSub to_vsub(const DIE* subprog, bool* succ) {
    const Table* abbrevs= subprog->type.table;
    const TagData* data= TagData_arr_ptr(&abbrevs->abbrevs, subprog->type.abbrev);

    if (has_attr(data, DW_AT_ranges)) {
        show_err("Cannot get subprogram at location as it requires DW_AT_ranges\n");
        goto to_vsub_fail;
    }

    const int16_t low= get_attr_pos(data, DW_AT_low_pc);
    if (low == -1) goto to_vsub_fail;

    const int16_t hi= get_attr_pos(data, DW_AT_high_pc);
    if (hi == -1) goto to_vsub_fail;

    const uintptr_t low_pc= DIEDATA_arr_ptr(&subprog->data, low)->address;
    const uintptr_t hi_pc= low_pc + DIEDATA_arr_ptr(&subprog->data, hi)->constant_m64_u;

    const char* name= get_subprog_name(subprog);

    *succ= true;
    return (VSub) {
        .vaddr_start= low_pc,
        .vaddr_end= hi_pc,
        .subprog_name= name
    };

to_vsub_fail:
    *succ= false;
    return (VSub){0};
}

VSub next_sub(SubIter* iter, bool* succ) {
    if (iter->idx >= vsubprogs.pos) {
        *succ= false;
        return (VSub){0};
    }

    *succ= true;
    return VSub_arr_get(&vsubprogs, iter->idx++);
}

DIE* get_subprog_at(const uintptr_t addr) {
    // ignoring external functions
    for (int i = 0; i < subprogs.pos; ++i) {
        DIE* subprog= DIE_arr_ptr(&subprogs, i);

        const Table* abbrevs= subprog->type.table;
        const TagData* data= TagData_arr_ptr(&abbrevs->abbrevs, subprog->type.abbrev);

        if (has_attr(data, DW_AT_ranges)) {
            show_err("Cannot get subprogram at location as it requires DW_AT_ranges\n");
            return NULL;
        }

        const int16_t low= get_attr_pos(data, DW_AT_low_pc);
        if (low == -1) continue;

        const int16_t hi= get_attr_pos(data, DW_AT_high_pc);
        if (hi == -1) continue;

        const uintptr_t low_pc= DIEDATA_arr_ptr(&subprog->data, low)->address;
        const uintptr_t hi_pc= low_pc + DIEDATA_arr_ptr(&subprog->data, hi)->constant_m64_u;

        if (addr >= low_pc && addr <= hi_pc) return subprog;
    }

    return NULL;
}

const char* get_subprog_name(const DIE* subprog) {
    const Table* abbrevs= subprog->type.table;
    const TagData* data= TagData_arr_ptr(&abbrevs->abbrevs, subprog->type.abbrev);

    const int16_t name_pos= get_attr_pos(data, DW_AT_name);
    if (name_pos == -1) return NULL;

    const FORM_DATA* die_data= DIEDATA_arr_ptr(&subprog->data, name_pos);
    const DW_FORM data_form= ATData_arr_ptr(&data->attributes, name_pos)->form;
    return die_data_get_str(die_data, data_form);
}

const char* get_subprog_name_at(const uintptr_t addr) {
    DIE* subprog= get_subprog_at(addr);
    if (!subprog) return NULL;

    return get_subprog_name(subprog);
}

const char* die_data_get_str(const FORM_DATA* data, const DW_FORM form) {
    switch (form) {
        case DW_FORM_string: return data->string;
        case DW_FORM_strp: return (char*)&ELF.section_map.debug_str.data[data->offset];
        case DW_FORM_line_strp: return (char*)&ELF.section_map.debug_line_str.data[data->offset];

        default: return NULL;
    }
}

const char* cu_get_filename(const DIE* die) {
    const Table* abbrev= die->type.table;
    const TagData* tag= TagData_arr_ptr(&abbrev->abbrevs, die->type.abbrev);

    printf("Looking for main filename");
    if (tag->tag != DW_TAG_compile_unit) return NULL;

    for (int i = 0; i < tag->attributes.pos; ++i) {
        const ATData* attr= ATData_arr_ptr(&tag->attributes, i);

        if (attr->attr == DW_AT_name) {
            const FORM_DATA* data= DIEDATA_arr_ptr(&die->data, i);
            return die_data_get_str(data, attr->form);
        }
    }

    return NULL;
}

static uint8_t offset_size_from_mode(MODE mode) {
    switch (mode) {
        case MODE_32bit: return 4;
        case MODE_64bit: return 8;
        default: assert(false);
    }
}

int parse_cu_dies(uint8_t** data, const uint8_t* section_end, CU_HEADER* header, TableArray* abbrev_tables) {
    uint32_t depth= 0;

    const Table* table= find_table(abbrev_tables, header->abbrev_offset);

    while (*data < section_end) {
        uint64_t abbrev_code= raa_uleb128(data).v;

        if (abbrev_code == 0) {
            if (depth <= 0) break;

            depth--;
            continue;
        }

        abbrev_code--;

        if (abbrev_code >= table->abbrevs.pos) {
            printf("Invalid abbrev code (%lu), unable to parse die", abbrev_code);
            return FAIL;
        }

        const TagData* tag= TagData_arr_ptr(&table->abbrevs, abbrev_code);

        const bool is_cu= tag->tag == DW_TAG_compile_unit;
        const bool is_sub= tag->tag == DW_TAG_subprogram;

        DIE die= (DIE) {
            .type= (DIE_TYPE){.abbrev= abbrev_code, .table= table},
            .nesting= depth,
            .data= DIEDATA_arr_construct(tag->attributes.pos)
        };

        for (int i = 0; i < tag->attributes.pos; ++i) {
            const ATData* attr= ATData_arr_ptr(&tag->attributes, i);
            const FORM_DATA die_data= raa_form_data(data, attr->form, header->address_size, offset_size_from_mode(header->mode));
            DIEDATA_arr_add(&die.data, die_data);
        }

        DIE_arr_add(&dies, die);
        if (is_cu) DIE_arr_add(&cus, die);
        if (is_sub) DIE_arr_add(&subprogs, die);

        print_die(&die);
        if (tag->has_children) depth++;
    }

    bool succ;
    for (int i = 0; i < subprogs.pos; ++i) {
        const DIE* sub= DIE_arr_ptr(&subprogs, i);
        const VSub vsub= to_vsub(sub, &succ);
        if (!succ) {
            const char* name= get_subprog_name(sub);
            show_err("Unable to convert subprogram (%s) into vsub\n", name);
            continue;
        }
        VSub_arr_add(&vsubprogs, vsub);
    }

    VSub_arr_sort_i(&vsubprogs);

    return SUCCESS;
}

void print_form_data(const FORM_DATA* data, DW_FORM form, uint64_t impl_const) {
    switch (form) {
        case DW_FORM_addr: {
            printf("%#lX", data->address);
            break;
        }

        case DW_FORM_addrx:
        case DW_FORM_addrx1:
        case DW_FORM_addrx2:
        case DW_FORM_addrx3:
        case DW_FORM_addrx4: {
            printf("%#lX", data->address_x);
            break;
        }

        case DW_FORM_block:
        case DW_FORM_block1:
        case DW_FORM_block2:
        case DW_FORM_block4: {
            printf("Block size: %lu @ %p", data->block.length, data->block.data);
            break;
        }


        case DW_FORM_data1:
        case DW_FORM_data2:
        case DW_FORM_data4:
        case DW_FORM_data8:
        case DW_FORM_udata: {
            printf("%lu", data->constant_m64_u);
            break;
        }

        case DW_FORM_data16: {
            printf("0x");
            for (int i = 0; i < sizeof(data->constant_m128_u); ++i) {
                printf("%x", data->constant_m128_u[i]);
            }
            break;
        }

        case DW_FORM_sdata: {
            printf("%ld", data->constant_m64_s);
            break;
        }


        case DW_FORM_flag_present:
        case DW_FORM_flag: {
            printf("Flag: %u", data->flag);
            break;
        }

        case DW_FORM_ref_addr:
        case DW_FORM_ref1:
        case DW_FORM_ref2:
        case DW_FORM_ref4:
        case DW_FORM_ref8:
        case DW_FORM_ref_udata: {
            printf("%lu", data->reference);
            break;
        }

        case DW_FORM_ref_sig8: {
            printf("%lu", data->typesig);
            break;
        }

        case DW_FORM_ref_sup4:
        case DW_FORM_ref_sup8: {
            printf("%lu", data->offset);
        }

        case DW_FORM_string: {
            printf("%s", data->string);
            break;
        }

        case DW_FORM_strp: {
            printf("%s", (char*)&ELF.section_map.debug_str.data[data->offset]);
            break;
        }
        case DW_FORM_strp_sup: {
            printf("String offset into supplementary %lu", data->offset);
            break;
        }
        case DW_FORM_line_strp: {
            printf("%s", (char*)&ELF.section_map.debug_line_str.data[data->offset]);
            break;
        }

        case DW_FORM_strx:
        case DW_FORM_strx1:
        case DW_FORM_strx2:
        case DW_FORM_strx3:
        case DW_FORM_strx4: {
            printf("%lu", data->offset);
            break;
        }

        case DW_FORM_exprloc: {
            printf("Expr size: %lu @ %p", data->exprloc.length, data->exprloc.data);
            break;
        }

        case DW_FORM_indirect: {
            printf("No value for indirect");
            break;
        }

        case DW_FORM_implicit_const: {
            printf("%ld", impl_const);
            break;
        }

        case DW_FORM_sec_offset:
        case DW_FORM_loclistx:
        case DW_FORM_rnglistx: {
            printf("%lu", data->offset);
            break;
        }
        default: assert(false);
    }
}

void print_nesting(const uint8_t nesting) {
    printf("%*.*s", nesting << 1, nesting << 1, "");
}

void print_die(const DIE* die) {
    print_nesting(die->nesting);

    const Table* table= die->type.table;
    const TagData* tag= TagData_arr_ptr(&table->abbrevs, die->type.abbrev);
    printf("<%.2u> %s\n", die->nesting, tag_string(tag->tag));

    for (int i = 0; i < tag->attributes.pos; ++i) {
        const ATData* attr= ATData_arr_ptr(&tag->attributes, i);

        print_nesting(die->nesting + 1);
        printf("%-30s", attribute_str(attr->attr));
        print_form_data(DIEDATA_arr_ptr(&die->data, i), attr->form, attr->impl_const);
        newline();
    }
}

FORM_DATA raa_form_data(uint8_t** start, const DW_FORM form, uint8_t addr_size, uint8_t offset_size) {
    switch (form) {
        case DW_FORM_addr: {
            //  An object of appropriate size to hold an address on the target machine
            //  The size is encoded in the compilation unit header
            return (FORM_DATA) {.address= raa_uint(start, addr_size)};
        }
        case DW_FORM_addrx: return (FORM_DATA) {.address_x= raa_uleb128(start).v};
        case DW_FORM_addrx1: return (FORM_DATA) {.address_x= raa_uint(start, 1)};
        case DW_FORM_addrx2: return (FORM_DATA) {.address_x= raa_uint(start, 2)};
        case DW_FORM_addrx3: return (FORM_DATA) {.address_x= raa_uint(start, 3)};
        case DW_FORM_addrx4: return (FORM_DATA) {.address_x= raa_uint(start, 4)};

        case DW_FORM_sec_offset: {
            // Form DW_FORM_sec_offset is a member of more than one class, namely
            // addrptr, lineptr, loclist, loclistsptr, macptr, rnglist, rnglistsptr, and stroffsetsptr;
            return (FORM_DATA) {.offset= raa_uint(start, offset_size)};
        }
        case DW_FORM_block: return (FORM_DATA) {.block= raa_block(start)};
        case DW_FORM_block1: return (FORM_DATA) {.block= raa_block_x(start, 1)};
        case DW_FORM_block2: return (FORM_DATA) {.block= raa_block_x(start, 2)};
        case DW_FORM_block4: return (FORM_DATA) {.block= raa_block_x(start, 4)};

        case DW_FORM_data1: return (FORM_DATA) {.constant_m64_u = raa_uint(start, 1)};
        case DW_FORM_data2: return (FORM_DATA) {.constant_m64_u = raa_uint(start, 2)};
        case DW_FORM_data4: return (FORM_DATA) {.constant_m64_u = raa_uint(start, 4)};
        case DW_FORM_data8: return (FORM_DATA) {.constant_m64_u = raa_uint(start, 8)};
        case DW_FORM_data16: {
            FORM_DATA data;
            raa_const_array(start, (uint8_t**)&data.constant_m128_u, 16);
            return data;
        }

        case DW_FORM_udata: {
            const ULEB128 value= raa_uleb128(start);
            return (FORM_DATA) {.constant_m64_u= value.v};
        }
        case DW_FORM_sdata: {
            const LEB128 value= raa_leb128(start);
            return (FORM_DATA) {.constant_m64_s= value.v};
        }

        case DW_FORM_flag: {
            const uint8_t flag= raa_uint(start, 1);
            return (FORM_DATA) {.flag= flag != 0};
        };
        case DW_FORM_flag_present: return (FORM_DATA) {.flag= 1};

        case DW_FORM_ref_addr: return (FORM_DATA) {.reference= raa_uint(start, offset_size)};

        case DW_FORM_ref1: return (FORM_DATA) {.reference= raa_uint(start, 1)};
        case DW_FORM_ref2: return (FORM_DATA) {.reference= raa_uint(start, 2)};
        case DW_FORM_ref4: return (FORM_DATA) {.reference= raa_uint(start, 4)};
        case DW_FORM_ref8: return (FORM_DATA) {.reference= raa_uint(start, 8)};

        case DW_FORM_ref_udata: return (FORM_DATA) {.reference= raa_uleb128(start).v};

        case DW_FORM_ref_sig8: return (FORM_DATA) {.typesig= raa_uint(start, 8)};
        case DW_FORM_ref_sup4: return (FORM_DATA) {.offset= raa_uint(start, 4)};
        case DW_FORM_ref_sup8: return (FORM_DATA) {.offset= raa_uint(start, 8)};

        case DW_FORM_string: return (FORM_DATA) {.string= (char*)raa_null_term_string(start)};

        case DW_FORM_strp:
        case DW_FORM_strp_sup:
        case DW_FORM_line_strp:
            return (FORM_DATA) {.offset= raa_uint(start, offset_size)};

        case DW_FORM_strx: return (FORM_DATA) {.offset= raa_uleb128(start).v};
        case DW_FORM_strx1: return (FORM_DATA) {.offset= raa_uint(start, 1)};
        case DW_FORM_strx2: return (FORM_DATA) {.offset= raa_uint(start, 2)};
        case DW_FORM_strx3: return (FORM_DATA) {.offset= raa_uint(start, 3)};
        case DW_FORM_strx4: return (FORM_DATA) {.offset= raa_uint(start, 4)};

        case DW_FORM_exprloc: return (FORM_DATA){ .exprloc= raa_expr(start)};

        case DW_FORM_indirect: {
            // This contains within it the form
            const DW_FORM encoded_form= raa_uleb128(start).v;
            return raa_form_data(start, encoded_form, addr_size, offset_size);
        }

        case DW_FORM_implicit_const: {
            // there is no value here
            return (FORM_DATA) {};
        }

        case DW_FORM_loclistx:
            //An index into the .debug_loclists section
            return (FORM_DATA) {.offset= raa_uleb128(start).v};

        case DW_FORM_rnglistx:
            //An index into the .debug_rnglists section
            return (FORM_DATA) {.offset= raa_uleb128(start).v};
        default: assert(false);
    }
}

void print_cu_header(const CU_HEADER* header) {
    printf("CU header v.%u, %#lx bytes (%s mode)\n", header->version, header->length, header->mode == MODE_32bit ? "32bit" : "64bit");
    if (header->version >= 5) {
        printf("\tType: %s", cu_type_str(header->type));
    }
    printf("\tAbbrev offset: %lu\n", header->abbrev_offset);
    printf("\tAddr. size: %u\n", header->address_size);
}

const char* cu_type_name(const CU_TYPE type) {
    switch (type) {
        case DW_UT_compile: return "DW_UT_compile";
        case DW_UT_type: return "DW_UT_type";
        case DW_UT_partial: return "DW_UT_partial";
        case DW_UT_skeleton: return "DW_UT_skeleton";
        case DW_UT_split_compile: return "DW_UT_split_compile";
        case DW_UT_split_type: return "DW_UT_split_type";
        case DW_UT_lo_user: return "DW_UT_lo_user";
        case DW_UT_hi_user: return "DW_UT_hi_user";
        default: return "Unknown unit type";
    }
}

void print_abbrev_tables(const TableArray* tables) {
    for (int i = 0; i < tables->pos; ++i) {
        printf("Abbrev table %u:\n", i);
        const Table* table= Table_arr_ptr(tables, i);

        for (int j = 0; j < table->abbrevs.pos; ++j) {
            const TagData* tag= TagData_arr_ptr(&table->abbrevs, j);

            printf("\t<%lu: %s %s>\n", tag->id, tag_string(tag->tag), (tag->has_children ? "No children" : "has children"));

            for (int k = 0; k < tag->attributes.pos; ++k) {
                const ATData* attr= ATData_arr_ptr(&tag->attributes, k);

                printf("\t\t\t%s %s", attribute_str(attr->attr), form_strs(attr->form));
                if (attr->form == DW_FORM_implicit_const) {
                    printf("const value: %ld", attr->impl_const);
                }
                newline();
            }
        }
    }
}

const char* tag_string(const DW_TAG tag) {
    switch (tag) {
        case DW_TAG_array_type: return "DW_TAG_array_type";
        case DW_TAG_class_type: return "DW_TAG_class_type";
        case DW_TAG_entry_point: return "DW_TAG_entry_point";
        case DW_TAG_enumeration_type: return "DW_TAG_enumeration_type";
        case DW_TAG_formal_parameter: return "DW_TAG_formal_parameter";
        case DW_TAG_imported_declaration: return "DW_TAG_imported_declaration";
        case DW_TAG_label: return "DW_TAG_label";
        case DW_TAG_lexical_block: return "DW_TAG_lexical_block";
        case DW_TAG_member: return "DW_TAG_member";
        case DW_TAG_pointer_type: return "DW_TAG_pointer_type";
        case DW_TAG_reference_type: return "DW_TAG_reference_type";
        case DW_TAG_compile_unit: return "DW_TAG_compile_unit";
        case DW_TAG_string_type: return "DW_TAG_string_type";
        case DW_TAG_structure_type: return "DW_TAG_structure_type";
        case DW_TAG_subroutine_type: return "DW_TAG_subroutine_type";
        case DW_TAG_typedef: return "DW_TAG_typedef";
        case DW_TAG_union_type: return "DW_TAG_union_type";
        case DW_TAG_unspecified_parameters: return "DW_TAG_unspecified_parameters";
        case DW_TAG_variant: return "DW_TAG_variant";
        case DW_TAG_common_block: return "DW_TAG_common_block";
        case DW_TAG_common_inclusion: return "DW_TAG_common_inclusion";
        case DW_TAG_inheritance: return "DW_TAG_inheritance";
        case DW_TAG_inlined_subroutine: return "DW_TAG_inlined_subroutine";
        case DW_TAG_module: return "DW_TAG_module";
        case DW_TAG_ptr_to_member_type: return "DW_TAG_ptr_to_member_type";
        case DW_TAG_set_type: return "DW_TAG_set_type";
        case DW_TAG_subrange_type: return "DW_TAG_subrange_type";
        case DW_TAG_with_stmt: return "DW_TAG_with_stmt";
        case DW_TAG_access_declaration: return "DW_TAG_access_declaration";
        case DW_TAG_base_type: return "DW_TAG_base_type";
        case DW_TAG_catch_block: return "DW_TAG_catch_block";
        case DW_TAG_const_type: return "DW_TAG_const_type";
        case DW_TAG_constant: return "DW_TAG_constant";
        case DW_TAG_enumerator: return "DW_TAG_enumerator";
        case DW_TAG_file_type: return "DW_TAG_file_type";
        case DW_TAG_friend: return "DW_TAG_friend";
        case DW_TAG_namelist: return "DW_TAG_namelist";
        case DW_TAG_namelist_item: return "DW_TAG_namelist_item";
        case DW_TAG_packed_type: return "DW_TAG_packed_type";
        case DW_TAG_subprogram: return "DW_TAG_subprogram";
        case DW_TAG_template_type_parameter: return "DW_TAG_template_type_parameter";
        case DW_TAG_template_value_parameter: return "DW_TAG_template_value_parameter";
        case DW_TAG_thrown_type: return "DW_TAG_thrown_type";
        case DW_TAG_try_block: return "DW_TAG_try_block";
        case DW_TAG_variant_part: return "DW_TAG_variant_part";
        case DW_TAG_variable: return "DW_TAG_variable";
        case DW_TAG_volatile_type: return "DW_TAG_volatile_type";
        case DW_TAG_dwarf_procedure: return "DW_TAG_dwarf_procedure";
        case DW_TAG_restrict_type: return "DW_TAG_restrict_type";
        case DW_TAG_interface_type: return "DW_TAG_interface_type";
        case DW_TAG_namespace: return "DW_TAG_namespace";
        case DW_TAG_imported_module: return "DW_TAG_imported_module";
        case DW_TAG_unspecified_type: return "DW_TAG_unspecified_type";
        case DW_TAG_partial_unit: return "DW_TAG_partial_unit";
        case DW_TAG_imported_unit: return "DW_TAG_imported_unit";
        case DW_TAG_condition: return "DW_TAG_condition";
        case DW_TAG_shared_type: return "DW_TAG_shared_type";
        case DW_TAG_type_unit: return "DW_TAG_type_unit";
        case DW_TAG_rvalue_reference_type: return "DW_TAG_rvalue_reference_type";
        case DW_TAG_template_alias: return "DW_TAG_template_alias";
        case DW_TAG_coarray_type: return "DW_TAG_coarray_type";
        case DW_TAG_generic_subrange: return "DW_TAG_generic_subrange";
        case DW_TAG_dynamic_type: return "DW_TAG_dynamic_type";
        case DW_TAG_atomic_type: return "DW_TAG_atomic_type";
        case DW_TAG_call_site: return "DW_TAG_call_site";
        case DW_TAG_call_site_parameter: return "DW_TAG_call_site_parameter";
        case DW_TAG_skeleton_unit: return "DW_TAG_skeleton_unit";
        case DW_TAG_immutable_type: return "DW_TAG_immutable_type";
        case DW_TAG_lo_user: return "DW_TAG_lo_user";
        case DW_TAG_hi_user: return "DW_TAG_hi_user";
        default: return "Unknown tag";
    }
}

const char* attribute_str(const DW_AT attr) {
    switch (attr) {
        case DW_AT_sibling: return "DW_AT_sibling";
        case DW_AT_location: return "DW_AT_location";
        case DW_AT_name: return "DW_AT_name";
        case DW_AT_ordering: return "DW_AT_ordering";
        case DW_AT_byte_size: return "DW_AT_byte_size";
        case DW_AT_bit_size: return "DW_AT_bit_size";
        case DW_AT_stmt_list: return "DW_AT_stmt_list";
        case DW_AT_low_pc: return "DW_AT_low_pc";
        case DW_AT_high_pc: return "DW_AT_high_pc";
        case DW_AT_language: return "DW_AT_language";
        case DW_AT_discr: return "DW_AT_discr";
        case DW_AT_discr_value: return "DW_AT_discr_value";
        case DW_AT_visibility: return "DW_AT_visibility";
        case DW_AT_import: return "DW_AT_import";
        case DW_AT_string_length: return "DW_AT_string_length";
        case DW_AT_common_reference: return "DW_AT_common_reference";
        case DW_AT_comp_dir: return "DW_AT_comp_dir";
        case DW_AT_const_value: return "DW_AT_const_value";
        case DW_AT_containing_type: return "DW_AT_containing_type";
        case DW_AT_default_value: return "DW_AT_default_value";
        case DW_AT_inline: return "DW_AT_inline";
        case DW_AT_is_optional: return "DW_AT_is_optional";
        case DW_AT_lower_bound: return "DW_AT_lower_bound";
        case DW_AT_producer: return "DW_AT_producer";
        case DW_AT_prototyped: return "DW_AT_prototyped";
        case DW_AT_return_addr: return "DW_AT_return_addr";
        case DW_AT_start_scope: return "DW_AT_start_scope";
        case DW_AT_bit_stride: return "DW_AT_bit_stride";
        case DW_AT_upper_bound: return "DW_AT_upper_bound";
        case DW_AT_abstract_origin: return "DW_AT_abstract_origin";
        case DW_AT_accessibility: return "DW_AT_accessibility";
        case DW_AT_address_class: return "DW_AT_address_class";
        case DW_AT_artificial: return "DW_AT_artificial";
        case DW_AT_base_types: return "DW_AT_base_types";
        case DW_AT_calling_convention: return "DW_AT_calling_convention";
        case DW_AT_count: return "DW_AT_count";
        case DW_AT_data_member_location: return "DW_AT_data_member_location";
        case DW_AT_decl_column: return "DW_AT_decl_column";
        case DW_AT_decl_file: return "DW_AT_decl_file";
        case DW_AT_decl_line: return "DW_AT_decl_line";
        case DW_AT_declaration: return "DW_AT_declaration";
        case DW_AT_discr_list: return "DW_AT_discr_list";
        case DW_AT_encoding: return "DW_AT_encoding";
        case DW_AT_external: return "DW_AT_external";
        case DW_AT_frame_base: return "DW_AT_frame_base";
        case DW_AT_friend: return "DW_AT_friend";
        case DW_AT_identifier_case: return "DW_AT_identifier_case";
        case DW_AT_namelist_item: return "DW_AT_namelist_item";
        case DW_AT_priority: return "DW_AT_priority";
        case DW_AT_segment: return "DW_AT_segment";
        case DW_AT_specification: return "DW_AT_specification";
        case DW_AT_static_link: return "DW_AT_static_link";
        case DW_AT_type: return "DW_AT_type";
        case DW_AT_use_location: return "DW_AT_use_location";
        case DW_AT_variable_parameter: return "DW_AT_variable_parameter";
        case DW_AT_virtuality: return "DW_AT_virtuality";
        case DW_AT_vtable_elem_location: return "DW_AT_vtable_elem_location";
        case DW_AT_allocated: return "DW_AT_allocated";
        case DW_AT_associated: return "DW_AT_associated";
        case DW_AT_data_location: return "DW_AT_data_location";
        case DW_AT_byte_stride: return "DW_AT_byte_stride";
        case DW_AT_entry_pc: return "DW_AT_entry_pc";
        case DW_AT_use_UTF8: return "DW_AT_use_UTF8";
        case DW_AT_extension: return "DW_AT_extension";
        case DW_AT_ranges: return "DW_AT_ranges";
        case DW_AT_trampoline: return "DW_AT_trampoline";
        case DW_AT_call_column: return "DW_AT_call_column";
        case DW_AT_call_file: return "DW_AT_call_file";
        case DW_AT_call_line: return "DW_AT_call_line";
        case DW_AT_description: return "DW_AT_description";
        case DW_AT_binary_scale: return "DW_AT_binary_scale";
        case DW_AT_decimal_scale: return "DW_AT_decimal_scale";
        case DW_AT_small: return "DW_AT_small";
        case DW_AT_decimal_sign: return "DW_AT_decimal_sign";
        case DW_AT_digit_count: return "DW_AT_digit_count";
        case DW_AT_picture_string: return "DW_AT_picture_string";
        case DW_AT_mutable: return "DW_AT_mutable";
        case DW_AT_threads_scaled: return "DW_AT_threads_scaled";
        case DW_AT_explicit: return "DW_AT_explicit";
        case DW_AT_object_pointer: return "DW_AT_object_pointer";
        case DW_AT_endianity: return "DW_AT_endianity";
        case DW_AT_elemental: return "DW_AT_elemental";
        case DW_AT_pure: return "DW_AT_pure";
        case DW_AT_recursive: return "DW_AT_recursive";
        case DW_AT_signature: return "DW_AT_signature";
        case DW_AT_main_subprogram: return "DW_AT_main_subprogram";
        case DW_AT_data_bit_offset: return "DW_AT_data_bit_offset";
        case DW_AT_const_expr: return "DW_AT_const_expr";
        case DW_AT_enum_class: return "DW_AT_enum_class";
        case DW_AT_linkage_name: return "DW_AT_linkage_name";
        case DW_AT_string_length_bit_size: return "DW_AT_string_length_bit_size";
        case DW_AT_string_length_byte_size: return "DW_AT_string_length_byte_size";
        case DW_AT_rank: return "DW_AT_rank";
        case DW_AT_str_offsets_base: return "DW_AT_str_offsets_base";
        case DW_AT_addr_base: return "DW_AT_addr_base";
        case DW_AT_rnglists_base: return "DW_AT_rnglists_base";
        case DW_AT_dwo_name: return "DW_AT_dwo_name";
        case DW_AT_reference: return "DW_AT_reference";
        case DW_AT_rvalue_reference: return "DW_AT_rvalue_reference";
        case DW_AT_macros: return "DW_AT_macros";
        case DW_AT_call_all_calls: return "DW_AT_call_all_calls";
        case DW_AT_call_all_source_calls: return "DW_AT_call_all_source_calls";
        case DW_AT_call_all_tail_calls: return "DW_AT_call_all_tail_calls";
        case DW_AT_call_return_pc: return "DW_AT_call_return_pc";
        case DW_AT_call_value: return "DW_AT_call_value";
        case DW_AT_call_origin: return "DW_AT_call_origin";
        case DW_AT_call_parameter: return "DW_AT_call_parameter";
        case DW_AT_call_pc: return "DW_AT_call_pc";
        case DW_AT_call_tail_call: return "DW_AT_call_tail_call";
        case DW_AT_call_target: return "DW_AT_call_target";
        case DW_AT_call_target_clobbered: return "DW_AT_call_target_clobbered";
        case DW_AT_call_data_location: return "DW_AT_call_data_location";
        case DW_AT_call_data_value: return "DW_AT_call_data_value";
        case DW_AT_noreturn: return "DW_AT_noreturn";
        case DW_AT_alignment: return "DW_AT_alignment";
        case DW_AT_export_symbols: return "DW_AT_export_symbols";
        case DW_AT_deleted: return "DW_AT_deleted";
        case DW_AT_defaulted: return "DW_AT_defaulted";
        case DW_AT_loclists_base: return "DW_AT_loclists_base";
        case DW_AT_lo_user: return "DW_AT_lo_user";
        case DW_AT_hi_user: return "DW_AT_hi_user";
        default: return "Unknown attribute";
    }
}

const char* cu_type_str(const CU_TYPE type) {
    switch (type) {
        case DW_UT_compile: return "DW_UT_compile";
        case DW_UT_type: return "DW_UT_type";
        case DW_UT_partial: return "DW_UT_partial";
        case DW_UT_skeleton: return "DW_UT_skeleton";
        case DW_UT_split_compile: return "DW_UT_split_compile";
        case DW_UT_split_type: return "DW_UT_split_type";
        case DW_UT_lo_user: return "DW_UT_lo_user";
        case DW_UT_hi_user: return "DW_UT_hi_user";
        default: return "Unknown cu type";
    }
}
