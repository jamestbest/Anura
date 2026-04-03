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
    OP_GT
} OPERATOR;

typedef enum TYPE {
    TYPE_LEFT,
    TYPE_RIGHT,
    TYPE_BOTH,
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
    DEP_BASE* links[4];
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

#define t3_output 0
#define t2_output 0

#define RAX_CODE 0
#define RBP_CODE 6
#define RSP_CODE 7
#define RIP_CODE 16
#define LOC_RAX (Location){.type= LOCATION_REGISTER, .data.register_id=RAX_CODE}

#define LOC_CONST(val) (Location){.type= LOCATION_CONST, .data.const_val= val}
#define LOC_REG_OFF(reg_id, off) (Location){.type= LOCATION_REGISTER_OFFSET, .data.reg_off= {.register_id= reg_id, .offset= off}}
#define LOC_COMPARE(comp){.type= LOCATION_COMPARE, .data.comparison= comp}

ARRAY_PROTO(uintptr_t, Addr)

typedef struct SubInfo {
    uintptr_t func_start; // id
    uintptr_t func_ends[3];
} SubInfo;
#define FUNC_ENDS_END 0x0

// this can either be a normal value, or it's logical for conditionals in which case 0 or not 0
typedef struct TargetValue {
    uint64_t value;
    bool logical; // 0 or not 0
    bool is_logical;
} TargetValue;

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

VECTOR_PROTO(struct TreeNode, Node)
typedef struct TreeNode {
    uintptr_t cfa;
    DEP_BASE* base;

    StackFrame frame;
    NodeVector links;
    MarkerVector markers;
} TreeNode;

int break_on_cause(const char* ident, uint32_t line);

#endif //ANURA_BREAK_ON_CAUSE_H