//
// Created by james on 16/02/26.
//

#ifndef BALIN_H
#define BALIN_H

#include "shared/Array.h"
#include "Saruman/DWARFParsing.h"
#include "Sauron.h"

typedef enum TAGS {
    DW_TAG_array_type= 0x01,
    DW_TAG_class_type= 0x02,
    DW_TAG_entry_point= 0x03,
    DW_TAG_enumeration_type= 0x04,
    DW_TAG_formal_parameter= 0x05,
    DW_TAG_imported_declaration= 0x08,
    DW_TAG_label= 0x0a,
    DW_TAG_lexical_block= 0x0b,
    DW_TAG_member= 0x0d,
    DW_TAG_pointer_type= 0x0f,
    DW_TAG_reference_type= 0x10,
    DW_TAG_compile_unit= 0x11,
    DW_TAG_string_type= 0x12,
    DW_TAG_structure_type= 0x13,
    DW_TAG_subroutine_type= 0x15,
    DW_TAG_typedef= 0x16,
    DW_TAG_union_type= 0x17,
    DW_TAG_unspecified_parameters= 0x18,
    DW_TAG_variant= 0x19,
    DW_TAG_common_block= 0x1a,
    DW_TAG_common_inclusion= 0x1b,
    DW_TAG_inheritance= 0x1c,
    DW_TAG_inlined_subroutine= 0x1d,
    DW_TAG_module= 0x1e,
    DW_TAG_ptr_to_member_type= 0x1f,
    DW_TAG_set_type= 0x20,
    DW_TAG_subrange_type= 0x21,
    DW_TAG_with_stmt= 0x22,
    DW_TAG_access_declaration= 0x23,
    DW_TAG_base_type= 0x24,
    DW_TAG_catch_block= 0x25,
    DW_TAG_const_type= 0x26,
    DW_TAG_constant= 0x27,
    DW_TAG_enumerator= 0x28,
    DW_TAG_file_type= 0x29,
    DW_TAG_friend= 0x2a,
    DW_TAG_namelist= 0x2b,
    DW_TAG_namelist_item= 0x2c,
    DW_TAG_packed_type= 0x2d,
    DW_TAG_subprogram= 0x2e,
    DW_TAG_template_type_parameter= 0x2f,
    DW_TAG_template_value_parameter= 0x30,
    DW_TAG_thrown_type= 0x31,
    DW_TAG_try_block= 0x32,
    DW_TAG_variant_part= 0x33,
    DW_TAG_variable= 0x34,
    DW_TAG_volatile_type= 0x35,
    DW_TAG_dwarf_procedure= 0x36,
    DW_TAG_restrict_type= 0x37,
    DW_TAG_interface_type= 0x38,
    DW_TAG_namespace= 0x39,
    DW_TAG_imported_module= 0x3a,
    DW_TAG_unspecified_type= 0x3b,
    DW_TAG_partial_unit= 0x3c,
    DW_TAG_imported_unit= 0x3d,
    DW_TAG_condition= 0x3f,
    DW_TAG_shared_type= 0x40,
    DW_TAG_type_unit= 0x41,
    DW_TAG_rvalue_reference_type= 0x42,
    DW_TAG_template_alias= 0x43,
    DW_TAG_coarray_type= 0x44,
    DW_TAG_generic_subrange=  0x45,
    DW_TAG_dynamic_type= 0x46,
    DW_TAG_atomic_type= 0x47,
    DW_TAG_call_site= 0x48,
    DW_TAG_call_site_parameter= 0x49,
    DW_TAG_skeleton_unit= 0x4a,
    DW_TAG_immutable_type= 0x4b,
    DW_TAG_lo_user= 0x4080,
    DW_TAG_hi_user= 0xffff,
} DW_TAG;

const char* tag_string(DW_TAG tag);

typedef enum ATTRIBUTES {
    DW_AT_sibling= 0x01,
    DW_AT_location= 0x02,
    DW_AT_name= 0x03,
    DW_AT_ordering= 0x09,
    DW_AT_byte_size= 0x0b,
    DW_AT_bit_size= 0x0d,
    DW_AT_stmt_list= 0x10,
    DW_AT_low_pc= 0x11,
    DW_AT_high_pc= 0x12,
    DW_AT_language= 0x13,
    DW_AT_discr= 0x15,
    DW_AT_discr_value= 0x16,
    DW_AT_visibility= 0x17,
    DW_AT_import= 0x18,
    DW_AT_string_length= 0x19,
    DW_AT_common_reference= 0x1a,
    DW_AT_comp_dir= 0x1b,
    DW_AT_const_value= 0x1c,
    DW_AT_containing_type= 0x1d,
    DW_AT_default_value= 0x1e,
    DW_AT_inline= 0x20,
    DW_AT_is_optional= 0x21,
    DW_AT_lower_bound= 0x22,
    DW_AT_producer= 0x25,
    DW_AT_prototyped= 0x27,
    DW_AT_return_addr= 0x2a,
    DW_AT_start_scope= 0x2c,
    DW_AT_bit_stride= 0x2e,
    DW_AT_upper_bound= 0x2f,
    DW_AT_abstract_origin= 0x31,
    DW_AT_accessibility= 0x32,
    DW_AT_address_class= 0x33,
    DW_AT_artificial= 0x34,
    DW_AT_base_types= 0x35,
    DW_AT_calling_convention= 0x36,
    DW_AT_count= 0x37,
    DW_AT_data_member_location= 0x38,
    DW_AT_decl_column= 0x39,
    DW_AT_decl_file= 0x3a,
    DW_AT_decl_line= 0x3b,
    DW_AT_declaration= 0x3c,
    DW_AT_discr_list= 0x3d,
    DW_AT_encoding= 0x3e,
    DW_AT_external= 0x3f,
    DW_AT_frame_base= 0x40,
    DW_AT_friend= 0x41,
    DW_AT_identifier_case= 0x42,
    DW_AT_namelist_item= 0x44,
    DW_AT_priority= 0x45,
    DW_AT_segment= 0x46,
    DW_AT_specification= 0x47,
    DW_AT_static_link= 0x48,
    DW_AT_type= 0x49,
    DW_AT_use_location= 0x4a,
    DW_AT_variable_parameter= 0x4b,
    DW_AT_virtuality= 0x4c,
    DW_AT_vtable_elem_location= 0x4d,
    DW_AT_allocated= 0x4e,
    DW_AT_associated= 0x4f,
    DW_AT_data_location= 0x50,
    DW_AT_byte_stride= 0x51,
    DW_AT_entry_pc= 0x52,
    DW_AT_use_UTF8= 0x53,
    DW_AT_extension= 0x54,
    DW_AT_ranges= 0x55,
    DW_AT_trampoline= 0x56,
    DW_AT_call_column= 0x57,
    DW_AT_call_file= 0x58,
    DW_AT_call_line= 0x59,
    DW_AT_description= 0x5a,
    DW_AT_binary_scale= 0x5b,
    DW_AT_decimal_scale= 0x5c,
    DW_AT_small= 0x5d,
    DW_AT_decimal_sign= 0x5e,
    DW_AT_digit_count= 0x5f,
    DW_AT_picture_string= 0x60,
    DW_AT_mutable= 0x61,
    DW_AT_threads_scaled= 0x62,
    DW_AT_explicit= 0x63,
    DW_AT_object_pointer= 0x64,
    DW_AT_endianity= 0x65,
    DW_AT_elemental= 0x66,
    DW_AT_pure= 0x67,
    DW_AT_recursive= 0x68,
    DW_AT_signature= 0x69,
    DW_AT_main_subprogram= 0x6a,
    DW_AT_data_bit_offset= 0x6b,
    DW_AT_const_expr= 0x6c,
    DW_AT_enum_class= 0x6d,
    DW_AT_linkage_name= 0x6e,
    DW_AT_string_length_bit_size= 0x6f,
    DW_AT_string_length_byte_size= 0x70,
    DW_AT_rank= 0x71,
    DW_AT_str_offsets_base= 0x72,
    DW_AT_addr_base= 0x73,
    DW_AT_rnglists_base= 0x74,
    DW_AT_dwo_name= 0x76,
    DW_AT_reference= 0x77,
    DW_AT_rvalue_reference= 0x78,
    DW_AT_macros= 0x79,
    DW_AT_call_all_calls= 0x7a,
    DW_AT_call_all_source_calls= 0x7b,
    DW_AT_call_all_tail_calls= 0x7c,
    DW_AT_call_return_pc= 0x7d,
    DW_AT_call_value= 0x7e,
    DW_AT_call_origin= 0x7f,
    DW_AT_call_parameter= 0x80,
    DW_AT_call_pc= 0x81,
    DW_AT_call_tail_call= 0x82,
    DW_AT_call_target= 0x83,
    DW_AT_call_target_clobbered= 0x84,
    DW_AT_call_data_location= 0x85,
    DW_AT_call_data_value= 0x86,
    DW_AT_noreturn= 0x87,
    DW_AT_alignment= 0x88,
    DW_AT_export_symbols= 0x89,
    DW_AT_deleted= 0x8a,
    DW_AT_defaulted= 0x8b,
    DW_AT_loclists_base= 0x8c,
    DW_AT_lo_user= 0x2000,
    DW_AT_hi_user= 0x3fff,
} DW_AT;

const char* attribute_str(DW_AT attr);

typedef struct AttributeData {
    DW_AT attr;
    DW_FORM form;
    int64_t impl_const;
} ATData;

ARRAY_PROTO(ATData, ATData)

typedef struct TagData {
    size_t id;
    DW_TAG tag;
    bool has_children;
    ATDataArray attributes;
} TagData;

ARRAY_PROTO(TagData, TagData)

typedef struct Table {
    TagDataArray abbrevs;
    uint64_t offset;
} Table;
ARRAY_PROTO(Table, Table)

void print_abbrev_tables(const TableArray* tables);
int parse_abbrev(Section* section, TableArray* tables);
int parse_info(Section* section, TableArray* abbrev_tables);

typedef enum CU_TYPE {
    DW_UT_compile= 0x01,
    DW_UT_type= 0x02,
    DW_UT_partial= 0x03,
    DW_UT_skeleton= 0x04,
    DW_UT_split_compile= 0x05,
    DW_UT_split_type= 0x06,
    DW_UT_lo_user= 0x80,
    DW_UT_hi_user= 0xff,
} CU_TYPE;

const char* cu_type_str(CU_TYPE type);

typedef struct CU_HEADER {
    uint64_t length;
    uint64_t abbrev_offset;
    uint16_t version;
    uint8_t address_size;
    CU_TYPE type;
    MODE mode;
} CU_HEADER;

typedef union DIE_DATA {
    uintptr_t address; // DW_FORM_ADDR,
    // these are indexes into debug_addr, with a base of DW_AT_addr_base in CU
    uint64_t address_x; // indirect address DW_FORM_ADDRX/1/2/3/4
    // uint64_t addrptr; // indirect address DW_FORM_sec_offset
    DW_BLOCK block;
    uint64_t constant_m64_u; // constant max 64 bits
    int64_t constant_m64_s;
    uint8_t constant_m128_u[16]; // constant max 128 bits
    int8_t constant_m128_s[16];
    DW_EXPR exprloc;
    uint8_t flag;
    uint64_t offset; // any form of DW_FORM_sec_offset
    // uint64_t lineptr; // offset into debug_line
    // uint64_t loclist; // offset into debug_loclists DW_FORM_loclistx/DW_FORM_sec_offset
    // uint64_t macptr; // offset into debug_macro DW_FORM_sec_offset
    // uint64_t rnglist; // offset into debug_rnglists DW_FORM_rnglistx/DW_FORM_sec_offset
    uint64_t reference; // references to locations of debugging information, many forms p.235
    uint64_t typesig; // Section 7.32 on page 245
    const char* string; // DW_FORM_string
    uint64_t strp; // string pointer DW_FORM_strp, DW_FORM_line_strp, DW_FORM_strp_sup
    uint64_t strx; // offset into debug_str_offsets DW_FORM_strx/1/2/3/4
    // uint64_t stroffsetsptr; // DW_FORM_sec_offset
} DIE_DATA;

ARRAY_PROTO(DIE_DATA, DIEDATA)

typedef struct DIE_TYPE {
    const Table* table;
    size_t abbrev;
} DIE_TYPE;

typedef struct DIE {
    DIE_TYPE type;
    uint8_t nesting;
    DIEDATAArray data;
} DIE;

ARRAY_PROTO(DIE, DIE)

DIE_DATA raa_die_data(uint8_t** start, DW_FORM form, const CU_HEADER* cu);

#endif //BALIN_H
