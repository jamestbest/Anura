//
// Created by jamestbest on 3/10/26.
//

#include "break_on_cause.h"

#include <stdint.h>

#include "Vector.h"

typedef enum DEPTYPE {
    DEPTYPE_LINEAR,
    DEPTYPE_SPLIT,
    DEPTYPE_COND,
    DEPTYPE_SENTINEL
} DEPTYPE;

typedef union DEPDATA {

} DEPDATA;

typedef struct DEP_BASE {
    DEPTYPE type;
} DEP_BASE;

#define BASE DEP_BASE base;

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

#define MAKE_CONST(val, reg, addr_) {                      \
                                .addr= addr_,                           \
                                .register_id= reg,                      \
                                .expr= (Expr) {                         \
                                    .type= EXPRTYPE_CONSTANT,           \
                                    .data.constant= val                 \
                                },                                      \
                                .base= {.type= DEPTYPE_LINEAR},         \
                                .link= NULL                             \
                                };                                      \

DEP_BASE SENTINEL= (DEP_BASE) {.type= DEPTYPE_SENTINEL};

DEP_LINEAR const_0= MAKE_CONST(0, 0, 0x1136)
DEP_LINEAR const_1= MAKE_CONST(0, 1, 0x1145)

DEP_LINEAR const_1_1= MAKE_CONST(0, 1, 0x117e)

DEP_LINEAR const_0_1= MAKE_CONST(0, 0, 0x1177)
DEP_LINEAR const_1_2= MAKE_CONST(0, 1, 0x1162)
DEP_LINEAR lin_dep_t3= (DEP_LINEAR) {
    .addr= 0x1169,
    .register_id= 0,
    .base= {.type= DEPTYPE_LINEAR},
    .link= (DEP_BASE*)&const_0,
    .expr= {
        .type= EXPRTYPE_SINGLE,
        .data.single= "t3()"
    }
};

DEP_LINEAR lin_dep_t2= (DEP_LINEAR) {
    .addr= 0x1154,
    .register_id= 0,
    .base= {.type= DEPTYPE_LINEAR},
    .link= (DEP_BASE*)&const_1,
    .expr= {
        .type= EXPRTYPE_SINGLE,
        .data.single= "t2()"
    }
};

DEP_COND cond_t3= (DEP_COND) {
    .base= {.type= DEPTYPE_COND},
    .condition= (DEP_BASE*)&lin_dep_t3,
    .link= (DEP_BASE*)&const_0_1
};

DEP_COND cond_t2= (DEP_COND) {
    .base= {.type= DEPTYPE_COND},
    .condition= (DEP_BASE*)&lin_dep_t2,
    .link= (DEP_BASE*)&const_1_2
};

DEP_SPLIT t1_split= (DEP_SPLIT) {
    .base= {.type= DEPTYPE_SPLIT},
    .links= {
        (DEP_BASE*)&cond_t2,
        (DEP_BASE*)&cond_t3,
        (DEP_BASE*)&const_1_1,
        &SENTINEL
    }
};

DEP_LINEAR res_31= (DEP_LINEAR) {
    .base= {.type= DEPTYPE_LINEAR},
    .register_id= 0,
    .addr=  0x1193,
    .expr= {
        .type= EXPRTYPE_CONSTANT,
        .data.constant= 0
    },
    .link= NULL
};

DEP_LINEAR const_1_34= MAKE_CONST(0, 1, 0x119d)
DEP_LINEAR t1_call_34= (DEP_LINEAR) {
    .base= {.type= DEPTYPE_LINEAR},
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
    .base= {.type= DEPTYPE_LINEAR},
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

DEP_LINEAR const_1_34_2= MAKE_CONST(0, 1, 0x11a9)

DEP_COND res_34= (DEP_COND) {
    .base= {.type= DEPTYPE_COND},
    .condition= (DEP_BASE*)&res_34_cnd,
    .link= (DEP_BASE*)&const_1_34_2
};

int break_on_cause(const char* ident, uint32_t line) {
    // assume the ident and line are 'res' and 34 resp.

}

