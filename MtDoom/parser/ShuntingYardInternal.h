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
    [POW]= ASSOC_LEFT,
    [PIPE]= ASSOC_LEFT
};

const uint8_t ASSOC_UN[UNARY_OP_COUNT]= {
    [NOT]= ASSOC_LEFT,
    [EXISTS]= ASSOC_LEFT
};

const uint8_t PRECEDENCE[BINARY_OP_COUNT]= {
    [EQUALITY]= 1,
    [NEQUALITY]= 1,
    [DOT]= 1,
    [STAR]= 1,
    [POW]= 1,
    [PIPE]= 1
};

const uint8_t PRECEDENCE_UN[UNARY_OP_COUNT]= {
    [NOT]= 1,
    [EXISTS]= 1
};

#endif //ANURA_SHUNTINGYARDINTERNAL_H