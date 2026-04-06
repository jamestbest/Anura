//
// Created by jamestbest on 3/10/26.
//

#ifndef ANURA_BREAK_ON_CAUSE_H
#define ANURA_BREAK_ON_CAUSE_H

#include <stdint.h>
#include <stdbool.h>

#include "shared/Array.h"
#include "shared/Vector.h"
#include "main.h"

typedef enum DEPTYPE {
    DEPTYPE_LINEAR,
    DEPTYPE_SPLIT,
    DEPTYPE_COND,
    DEPTYPE_HEADER,
    DEPTYPE_COLLECT,
    DEPTYPE_EXPR,

    DEPTYPE_SENTINEL,
    DEPTYPE_COUNT
} DEPTYPE;

typedef struct DEP_BASE {
    DEPTYPE type;
    bool dead: 1;
    bool pass: 1;
    uint8_t id;
} DEP_BASE;

#define BASE DEP_BASE base;
extern DEP_BASE SENTINEL;

typedef enum OPERATOR {
    OP_EQ,
    OP_NE,
    OP_GT,
    OP_COUNT
} OPERATOR;

extern const char* OPERATOR_STRS[OP_COUNT];

typedef enum TYPE {
    TYPE_LEFT,
    TYPE_RIGHT,
    TYPE_BOTH,
    TYPE_NONE
} TYPE;

typedef enum LocationType {
    LOCATION_REGISTER,
    LOCATION_REGISTER_OFFSET,
    LOCATION_COMPARE,
    LOCATION_CONST,
} LocationType;

typedef union LocationData {
    uint16_t register_id;
    struct {
        uint16_t register_id;
        int64_t offset;
    } reg_off;
    COMPARISONS comparison;
    int64_t const_val;
} LocationData;

typedef struct Location {
    LocationType type;
    LocationData data;
    uint8_t byte_size;
} Location;

#define HITTABLE_BASE BASE; \
    uintptr_t addr; \
    uintptr_t value_addr_offset; \
    Location result_loc;

typedef struct DEP_HITTABLE {
    HITTABLE_BASE
} DEP_HITTABLE;

typedef struct DEP_LINEAR {
    HITTABLE_BASE
    DEP_BASE* link;
} DEP_LINEAR;

typedef struct DEP_EXPR {
    HITTABLE_BASE
    TYPE type;

    DEP_BASE* left;
    DEP_BASE* right;
    OPERATOR op;
} DEP_EXPR;

typedef struct DEP_SPLIT {
    BASE
    DEP_BASE* links[10];
} DEP_SPLIT;

typedef struct DEP_COND {
    BASE
    DEP_BASE* condition;
    DEP_BASE* link;
    bool left;
} DEP_COND;

typedef struct DEP_HEADER {
    BASE
    uintptr_t addr;
    DEP_BASE* link;
} DEP_HEADER;

typedef struct DEP_COLLECT {
    BASE
    size_t conns;
    DEP_BASE* link;
} DEP_COLLECT;

#define MAKE_CONST(id_, val, addr_) {                                        \
    .addr= addr_,                                                       \
    .value_addr_offset= 0,                                              \
    .result_loc= LOC_CONST(val),                                          \
    .base= {.type= DEPTYPE_LINEAR, .dead=false, .id=id_, .pass=false},                        \
    .link= NULL                                                         \
};

#define MAKE_DATA(id_, loc, addr_) {\
    .addr= addr_,                                                       \
    .value_addr_offset= 0,                                              \
    .result_loc= loc,                                                     \
    .base= {.type= DEPTYPE_LINEAR, .dead=false, .id=id_, .pass=false},                        \
    .link= NULL                                                         \
};

#define MAKE_SINGLE(id_, addr_, addr_offset, loc, is_dead, pass_, link_, single_str) { \
    .addr= addr_,\
    .value_addr_offset= addr_offset,\
    .result_loc= loc,\
    .base= {.type= DEPTYPE_LINEAR, .dead= is_dead, .id=id_, .pass=pass_},\
    .link= (DEP_BASE*)&link_\
}

#define MAKE_SPLIT(id_, is_dead, ...) (DEP_SPLIT) {\
    .base= {.type= DEPTYPE_SPLIT, .dead=is_dead, .id=id_, .pass=PASS},\
    .links= {__VA_ARGS__, &SENTINEL} \
}

#define MAKE_COND(id_, is_dead, is_left, condition_, link_) (DEP_COND) {  \
    .base= {.type= DEPTYPE_COND, .dead=is_dead, .id=id_, .pass=PASS}, \
    .condition= (DEP_BASE*)&condition_, \
    .link= (DEP_BASE*)&link_,    \
    .left= is_left\
}

#define MAKE_EXPR(id_, is_dead, pass_, op_, left_, right_, type_, addr_, addr_offset, loc_) {\
    .base= {.type= DEPTYPE_EXPR, .dead= is_dead, .id=id_, .pass=pass_}, \
\
.type= type_,\
\
    .left= (DEP_BASE*)&left_,\
    .right= (DEP_BASE*)&right_,\
    .op= op_,\
\
    .addr= addr_,\
    .value_addr_offset= addr_offset,\
    .result_loc= loc_\
}

#define MAKE_HEADER(id_, addr_, link_, is_dead, pass_) (DEP_HEADER) {\
    .base= {.type= DEPTYPE_HEADER, .dead= is_dead, .id=id_, .pass=pass_},\
    .addr= addr_,\
    .link= (DEP_BASE*)&link_\
}

#define MAKE_COLLECT(id_, conns_, dead_, pass_, link_) (DEP_COLLECT) {\
    .base= {.type= DEPTYPE_COLLECT, .dead= dead_, .id=id_, .pass=pass_},\
    .conns= conns_,\
    .link= (DEP_BASE*)&link_\
}

#define CONNECTIONS(num) num

#define t3_output 0
#define t2_output 0

#define RAX_CODE 0
#define RBP_CODE 6
#define RSP_CODE 7
#define RIP_CODE 16
#define LOC_RAX (Location){.type= LOCATION_REGISTER, .data.register_id=RAX_CODE, .byte_size=8}
#define LOC_EAX (Location){.type= LOCATION_REGISTER, .data.register_id=RAX_CODE, .byte_size=4}

#define LOC_CONST(val) (Location){.type= LOCATION_CONST, .data.const_val= val, .byte_size=8}
#define LOC_REG_OFF(reg_id, off, size) (Location){.type= LOCATION_REGISTER_OFFSET, .data.reg_off= {.register_id= reg_id, .offset= off}, .byte_size= size}
#define LOC_COMPARE(comp){.type= LOCATION_COMPARE, .data.comparison= comp}

typedef struct SubInfo {
    uintptr_t func_start; // id
    uintptr_t func_ends[3];
} SubInfo;
#define FUNC_ENDS_END 0x0

typedef struct TargetRule {
    OPERATOR op;
    int64_t value;
} TargetRule;
ARRAY_PROTO(TargetRule, TargetRule)

typedef union TargetValueData {
    int64_t value;
    TargetRuleArray rules;
    struct {
        int64_t start;
        int64_t end;
    } range;
} TargetValueData;

typedef enum TargetValueType {
    TARGET_VALUE_LOGICAL,
    TARGET_VALUE_VALUE,
    TARGET_VALUE_RULES,
    TARGET_VALUE_RANGE,
    TARGET_VALUE_ANY,
    TARGET_VALUE_COUNT
} TargetValueType;

extern const char* TARGET_VALUE_TYPE_STRS[TARGET_VALUE_COUNT];

typedef struct TargetValue {
    TargetValueType type;
    TargetValueData data;
    bool inverted;
} TargetValue;

extern const TargetValue TARGET_VALUE_BASE;

struct TreeNode;

typedef struct Marker {
    struct Marker* prev;

    DEP_BASE* pos;
    uintptr_t pc;
    struct TreeNode* node;
    TargetValue target_value;
    bool dead;
    bool going_upstream;
    bool placed_right_expr_marker;
    uint8_t split_hits;
} Marker;

VECTOR_PROTO(Marker, Marker)

typedef struct CollectInfo {
    DEP_COLLECT* collect;
    TargetValue target_value;
    size_t current_hits;
} CollectInfo;
ARRAY_PROTO_CMP(CollectInfo, CollectInfo, collect_info_cmp, collect)

VECTOR_PROTO(struct TreeNode, Node)
typedef struct TreeNode {
    uintptr_t cfa;
    DEP_BASE* base;

    StackFrame frame;
    NodeVector links;
    MarkerVector markers;
    CollectInfoArray collections;
} TreeNode;

int break_on_cause(bool is_simple);

#endif //ANURA_BREAK_ON_CAUSE_H