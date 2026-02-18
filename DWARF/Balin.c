//
// Created by james on 16/02/26.
//

#include "Balin.h"

#include "Errors.h"
#include "Saruman/DWARFParsing.h"
#include "Sauron.h"
#include "shared/Array.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct AttributeData {
    DW_AT attr;
    DW_FORM form;
    int64_t impl_const;
} ATData;

ARRAY_PROTO(ATData, ATData)
ARRAY_ADD(ATData, ATData)

typedef struct TagData {
    size_t id;
    DW_TAG tag;
    bool has_children;
    ATDataArray attributes;
} TagData;

ARRAY_PROTO(TagData, TagData)
ARRAY_ADD(TagData, TagData)

typedef enum ParseRes {
    PARSE_FAIL=-1,
    PARSE_SUCC_CONTINUE=0,
    PARSE_SUCC_END=1
} ParseRes;

TagDataArray create_table() {
    return TagData_arr_create();
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

ParseRes parse_tag(TagDataArray* table, uint8_t** base) {
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

    TagData_arr_add(table, data);
    return PARSE_SUCC_CONTINUE;
}

int parse_abbrev(uint8_t* data, uint8_t* section_end) {
    TagDataArray table= create_table();
    while (data < section_end) {
        const ParseRes res= parse_tag(&table, &data);
        switch (res) {
            case PARSE_SUCC_END: goto end;
            case PARSE_SUCC_CONTINUE: continue;
            case PARSE_FAIL: return FAIL;
        }
    }
end:
}