//
// Created by jamestbest on 3/10/26.
//

#include "break_on_cause.h"

#include <stdint.h>

#include "Vector.h"
#include "Array.h"
#include "Errors.h"
#include "Target.h"

typedef enum DEPTYPE {
    DEPTYPE_LINEAR,
    DEPTYPE_SPLIT,
    DEPTYPE_COND,
    DEPTYPE_SENTINEL,
    DEPTYPE_COUNT
} DEPTYPE;

const char* DEPTYPE_STRS[DEPTYPE_COUNT]= {
    [DEPTYPE_LINEAR]= "DEPTYPE_LINEAR",
    [DEPTYPE_SPLIT]= "DEPTYPE_SPLIT",
    [DEPTYPE_COND]= "DEPTYPE_COND",
    [DEPTYPE_SENTINEL]= "DEPTYPE_SENTINEL"
};

typedef union DEPDATA {

} DEPDATA;

typedef struct DEP_BASE {
    DEPTYPE type;
    bool dead: 1;
    bool frame_header: 1; // the start of a new stack frame
} DEP_BASE;

#define BASE DEP_BASE base;
DEP_BASE SENTINEL= (DEP_BASE) {.type= DEPTYPE_SENTINEL, .dead=false};

typedef enum EXPRTYPE {
    EXPRTYPE_CONSTANT,  // e.g. 1, 0...
    EXPRTYPE_EXPR,      // e.g. x == y
    EXPRTYPE_SINGLE,    // e.g. return 0, t3() -- some single line
    EXPRTYPE_LINK       // link to another dependancy tree
} EXPRTYPE;

typedef enum OPERATOR {
    OP_EQ,
    OP_NE
} OPERATOR;

typedef enum TYPE {
    TYPE_LEFT,
    TYPE_RIGHT,
    TYPE_BOTH_R,
    TYPE_BOTH_L
} TYPE;

typedef union EXPRDATA {
    int64_t constant;
    struct {
        OPERATOR op;
        TYPE type;
        DEP_BASE* sup_link;
    } expr;
    const char* single;
} EXPRDATA;

typedef struct Expr {
    EXPRTYPE type;
    EXPRDATA data;
} Expr;

typedef struct DEP_LINEAR {
    BASE
    DEP_BASE* link;
    Expr expr;
    uintptr_t addr;
    uintptr_t value_addr_offset;
    uint16_t register_id;
} DEP_LINEAR;

typedef struct DEP_SPLIT {
    BASE
    DEP_BASE* links[4];
} DEP_SPLIT;

typedef struct DEP_COND {
    BASE
    DEP_BASE* condition;
    DEP_BASE* link;
} DEP_COND;

#define MAKE_CONST(val, reg, addr_, hdr) {                              \
    .addr= addr_,                                                       \
    .value_addr_offset= 0,                                              \
    .register_id= reg,                                                  \
    .expr= (Expr) {                                                     \
        .type= EXPRTYPE_CONSTANT,                                       \
        .data.constant= val                                             \
    },                                                                  \
    .base= {.type= DEPTYPE_LINEAR, .dead=false, .frame_header= hdr},    \
    .link= NULL                                                         \
};

typedef enum LocationType {
    LOCATION_REGISTER,
    LOCATION_REGISTER_OFFSET,
    LOCATION_FLAG,
} LocationType;

typedef enum Flag {
    FLAG_EQ,
    FLAG_NEQ,
    FLAG_LESS, // ... etc
} Flag;

typedef union LocationData {
    uint16_t register_id;
    struct {
        uint16_t register_id;
        int64_t offset;
    } reg_off;
    Flag flag;
} LocationData;

typedef struct Location {
    LocationType type;
    LocationData data;
} Location;

#define MAKE_SINGLE(addr_, addr_offset, reg_id, is_dead, is_frame_start, link_, single_str) (DEP_LINEAR) { \
    .addr= addr_,\
    .value_addr_offset= addr_offset,\
    .base= {.type= DEPTYPE_LINEAR, .dead= is_dead, .frame_header= is_frame_start},\
    .link= (DEP_BASE*)&link_,\
    .expr= {\
        .type= EXPRTYPE_SINGLE,\
        .data.single= single_str\
    }\
}

#define MAKE_SPLIT(is_dead, is_frame_start, ...) (DEP_SPLIT) {\
    .base= {.type= DEPTYPE_SPLIT, .dead=is_dead, .frame_header= is_frame_start},\
    .links= {__VA_ARGS__, &SENTINEL} \
}

#define MAKE_COND(is_dead, is_frame_start, condition_, link_) (DEP_COND) {  \
    .base= {.type= DEPTYPE_COND, .dead=is_dead, .frame_header= is_frame_start}, \
    .condition= (DEP_BASE*)&condition_, \
    .link= (DEP_BASE*)&link_    \
}

#define t3_output 0
#define t2_output 1

#define RAX_CODE 0

DEP_LINEAR dep_t3= MAKE_CONST(t3_output, RAX_CODE, 0x1136, true)
DEP_LINEAR dep_t2= MAKE_CONST(t2_output, RAX_CODE, 0x1145, true)

DEP_LINEAR dep_t2_call_in_t1= MAKE_SINGLE(
    0x1154, 0x5,
    RAX_CODE,
    false, false,
    dep_t2, "t2()"
);

DEP_LINEAR dep_t1_return_fail= MAKE_CONST(FAIL, RAX_CODE, 0x1162, false);
DEP_COND dep_t1_cond_on_t2= MAKE_COND(false, false, dep_t2_call_in_t1, dep_t1_return_fail);

DEP_LINEAR dep_t3_call_in_t1= MAKE_SINGLE(
    0x1169, 0x5,
    RAX_CODE,
    false, false,
    dep_t3, "t3()"
);
DEP_LINEAR dep_t1_return_success= MAKE_CONST(SUCCESS, RAX_CODE, 0x1177, false);
DEP_COND dep_t1_cond_on_t3= MAKE_COND(false, false, dep_t3_call_in_t1, dep_t1_return_success);

DEP_LINEAR dep_t1_linear_on_fail= MAKE_CONST(FAIL, RAX_CODE, 0x117e, false);

DEP_SPLIT dep_t1= MAKE_SPLIT(false, true,
    (DEP_BASE*)&dep_t1_cond_on_t2, \
    (DEP_BASE*)&dep_t1_cond_on_t3, \
    (DEP_BASE*)&dep_t1_linear_on_fail \
);




// todo the main::res::34








DEP_LINEAR const_0= MAKE_CONST(0, 0, 0x1136, true)
DEP_LINEAR const_1= MAKE_CONST(0, 1, 0x1145, true)

DEP_LINEAR const_1_1= MAKE_CONST(0, 1, 0x117e, false)

DEP_LINEAR const_0_1= MAKE_CONST(0, 0, 0x1177, false)
DEP_LINEAR const_1_2= MAKE_CONST(0, 1, 0x1162, false)
DEP_LINEAR lin_dep_t3= (DEP_LINEAR) {
    .addr= 0x1169,
    .register_id= 0,
    .base= {.type= DEPTYPE_LINEAR, .dead=false},
    .link= (DEP_BASE*)&const_0,
    .expr= {
        .type= EXPRTYPE_SINGLE,
        .data.single= "t3()"
    }
};

DEP_LINEAR lin_dep_t2= (DEP_LINEAR) {
    .addr= 0x1154,
    .register_id= 0,
    .base= {.type= DEPTYPE_LINEAR, .dead=false},
    .link= (DEP_BASE*)&const_1,
    .expr= {
        .type= EXPRTYPE_SINGLE,
        .data.single= "t2()"
    }
};

DEP_COND cond_t3= (DEP_COND) {
    .base= {.type= DEPTYPE_COND, .dead=false},
    .condition= (DEP_BASE*)&lin_dep_t3,
    .link= (DEP_BASE*)&const_0_1
};

DEP_COND cond_t2= (DEP_COND) {
    .base= {.type= DEPTYPE_COND, .dead=false},
    .condition= (DEP_BASE*)&lin_dep_t2,
    .link= (DEP_BASE*)&const_1_2
};

DEP_SPLIT t1_split= (DEP_SPLIT) {
    .base= {.type= DEPTYPE_SPLIT, .dead=false, .frame_header= true},
    .links= {
        (DEP_BASE*)&cond_t2,
        (DEP_BASE*)&cond_t3,
        (DEP_BASE*)&const_1_1,
        &SENTINEL
    }
};

DEP_LINEAR res_31= (DEP_LINEAR) {
    .base= {.type= DEPTYPE_LINEAR, .dead=false},
    .register_id= 0,
    .addr=  0x1193,
    .expr= {
        .type= EXPRTYPE_CONSTANT,
        .data.constant= 0
    },
    .link= NULL
};

DEP_LINEAR const_1_34= MAKE_CONST(0, 1, 0x119d, false)
DEP_LINEAR t1_call_34= (DEP_LINEAR) {
    .base= {.type= DEPTYPE_LINEAR, .dead=false},
    .register_id= 0,
    .addr= 0x1198,
    .expr= {
        .type= EXPRTYPE_SINGLE,
        .data.single= "t1()"
    },
    .link= (DEP_BASE*)&t1_split
};

DEP_LINEAR res_34_cnd= (DEP_LINEAR) {
    .addr= 0x11a0,
    .register_id= 0,
    .base= {.type= DEPTYPE_LINEAR, .dead=false},
    .expr= {
        .type= EXPRTYPE_EXPR,
        .data.expr= {
            .op= OP_EQ,
            .type= TYPE_LEFT,
            .sup_link= (DEP_BASE*)&const_1_34
        }
    },
    .link= (DEP_BASE*)&t1_call_34
};

DEP_LINEAR const_1_34_2= MAKE_CONST(0, 1, 0x11a9, false)

DEP_COND res_34= (DEP_COND) {
    .base= {.type= DEPTYPE_COND, .dead=false},
    .condition= (DEP_BASE*)&res_34_cnd,
    .link= (DEP_BASE*)&const_1_34_2
};

ARRAY_PROTO(uintptr_t, Addr)
ARRAY_ADD(uintptr_t, Addr)

typedef struct SubInfo {
    uintptr_t func_start; // id
    uintptr_t func_ends[3];
} SubInfo;

#define FUNC_ENDS_END 0x0

const SubInfo main_info= (SubInfo) {
    .func_start= 0x1180,
    .func_ends= {
        0x11ac,
        FUNC_ENDS_END
    }
};

const SubInfo t1_info= (SubInfo) {
    .func_start= 0x1147,
    .func_ends= {
        0x117e,
        FUNC_ENDS_END
    }
};

const SubInfo t2_info= (SubInfo) {
    .func_start= 0x1138,
    .func_ends= {
        0x1145,
        FUNC_ENDS_END
    }
};

const SubInfo t3_info= (SubInfo) {
    .func_start= 0x1129,
    .func_ends= {
        0x1136,
        FUNC_ENDS_END
    }
};

// this can either be a normal value, or it's logical for conditionals in which case 0 or not 0
typedef struct TargetValue {
    uint64_t value;
    bool logical; // 0 or not 0
    bool is_logical;
} TargetValue;

struct TreeNode;

typedef struct Marker {
    DEP_BASE* pos;
    uintptr_t pc;
    struct TreeNode* node;
    TargetValue target_value;
    bool dead;
} Marker;

VECTOR_PROTO(struct TreeNode, Node)
typedef struct TreeNode {
    uintptr_t cfa;
    DEP_BASE* base;

    StackFrame frame;
    NodeVector links;
} TreeNode;
VECTOR_ADD(struct TreeNode, Node)

TreeNode* alloc_node(uint64_t cfa, DEP_BASE* base) {
    TreeNode* node= malloc(sizeof(TreeNode));

    *node= (TreeNode) {
        .cfa= cfa,
        .links= Node_vec_create(),
        .base= base,
        .frame= (StackFrame){0}
    };

    return node;
}

bool target_to_logical(const TargetValue target) {
    if (target.is_logical) return target.logical;

    return target.value != 0;
}

#define TARGET_LOGICAL(val) (TargetValue) {.is_logical= true, .logical= val}
#define TARGET_CONST(val) (TargetValue) {.is_logical= false, .value= val}

uintptr_t marker_cmp(uintptr_t a, uintptr_t b) {
    return a - b;
}

ARRAY_PROTO_CMP(Marker, Marker, marker_cmp, pc)
ARRAY_ADD_CMP(Marker, Marker, marker_cmp, pc)

MarkerArray markers= (MarkerArray){0};

TreeNode* root;

void place_marker_on(DEP_BASE* base, const Marker* existing_marker, const TargetValue target_value) {
    Marker m;

    if (!markers.arr) markers= Marker_arr_create();

    m.dead= base->dead || (existing_marker && existing_marker->dead);
    m.pos= base;

    if (base->frame_header || !existing_marker) {
        bool succ;
        const uint64_t cfa= target.target_get_cfa(&succ);
        TreeNode* new_node= alloc_node(cfa, base);

        if (!existing_marker) {
            root= new_node;
        } else {
            Node_vec_add(&existing_marker->node->links, new_node);

            m.node= new_node;
        }
    } else {
        m.node= existing_marker->node;
    }

    switch (base->type) {
        case DEPTYPE_LINEAR: {
            const DEP_LINEAR* lin= (DEP_LINEAR*)base;
            m.pc= lin->addr;
            m.target_value= target_value;
            break;
        }

        case DEPTYPE_SPLIT: {
            DEP_SPLIT* split= (DEP_SPLIT*)base;
            size_t i= 0;
            while (split->links[i]->type != DEPTYPE_SENTINEL) {
                place_marker_on(split->links[i], existing_marker, target_value);
                i++;
            }
            break;
        }

        case DEPTYPE_COND: {
            const DEP_COND* cond= (DEP_COND*)base;

            place_marker_on(cond->condition, existing_marker, TARGET_LOGICAL(true));
            place_marker_on(cond->link, existing_marker, target_value);
            break;
        }
        case DEPTYPE_SENTINEL:
            assert(false);
    }

    if (base->type == DEPTYPE_LINEAR) {
        const DEP_LINEAR* lin= (DEP_LINEAR*)base;

        // todo add const check

        target.target_place_temp_bp(lin->addr, BP_REASON_BREAK_CAUSE);
    }

    Marker_arr_add(&markers, m);
}

void place_initial_marker(DEP_BASE* base, uint64_t target_value) {
    place_marker_on(base, NULL, TARGET_CONST(target_value));
}

int break_on_cause(const char* ident, uint32_t line) {
    // assume the ident and line are 'res' and 34 resp.
    // this is creating the initial setup
    DEP_BASE* first= (DEP_BASE*)&res_34_cnd;

    bool succ;
    const uint64_t cfa= target.target_get_cfa(&succ);
    if (!succ) return FAIL;

    root= alloc_node(cfa, first);

    place_initial_marker(first, FAIL);

    return SUCCESS;
}

void handle_marker_hit(Marker* marker, size_t idx) {
    if (marker->pos->type != DEPTYPE_LINEAR) {
        show_err("Hit non-linear marker (%s), should not occur\n", DEPTYPE_STRS[marker->pos->type]);
        return;
    }

    const DEP_LINEAR* lin= (DEP_LINEAR*)marker;
    DEP_BASE* next= lin->link;

    place_marker_on(next, marker, marker->target_value);
}

int handle_break_on_cause() {
    const uintptr_t pc= target.target_get_pc();
    const uintptr_t v_pc= target.target_addr_runtime_to_virtual(pc);

    Marker_arr_sort_i(&markers);

    bool in_line= false;
    for (size_t i = 0; i < markers.pos; ++i) {
        Marker* marker= Marker_arr_ptr(&markers, i);

        if (marker->pc == v_pc) {
            in_line= true;

            handle_marker_hit(marker, i);
        } else if (in_line) {
            break;
        }
    }

    return SUCCESS;
}

