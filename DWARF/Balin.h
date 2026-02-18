//
// Created by james on 16/02/26.
//

#ifndef BALIN_H
#define BALIN_H

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


#endif //BALIN_H
