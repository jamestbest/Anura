//
// Created by jamestbest on 1/23/26.
//

#ifndef ANURA_SHUNTINGYARDINTERNAL_H
#define ANURA_SHUNTINGYARDINTERNAL_H

#include "lexer/Lexer.h"

typedef enum ASSOC_TYPE {
    ASSOC_LEFT,
    ASSOC_RIGHT
} ASSOC_TYPE;

const uint8_t ASSOC[BINARY_OP_COUNT]= {
    [EQUALITY]= ASSOC_LEFT,
    [NEQUALITY]= ASSOC_LEFT,
    [DOT]= ASSOC_LEFT,
    [STAR]= ASSOC_LEFT,
    [POW]= ASSOC_RIGHT,
    [AND]= ASSOC_LEFT,
    [PIPE]= ASSOC_LEFT
};

const uint8_t ASSOC_UN[UNARY_OP_COUNT]= {
    [NOT]= ASSOC_RIGHT,
    [EXISTS]= ASSOC_RIGHT
};

const uint8_t PRECEDENCE[BINARY_OP_COUNT]= {
    [EQUALITY]= 2,
    [NEQUALITY]= 2,
    [DOT]= 3,
    [STAR]= 4,
    [POW]= 5,
    [AND]= 1,
    [PIPE]= 0
};

const uint8_t PRECEDENCE_UN[UNARY_OP_COUNT]= {
    [NOT]= 6,
    [EXISTS]= 6
};

#define SHUNT_RET_FAIL (ShuntRet){.succ= false}

#endif //ANURA_SHUNTINGYARDINTERNAL_H