//
// Created by james on 26/12/25.
//

#ifndef SHUNTINGYARD_H
#define SHUNTINGYARD_H

#include "Parser.h"

typedef struct ShuntRet {
    bool succ;
    Node* node;
    size_t idx;
} ShuntRet;

ShuntRet shunt(TokenArray* tokens, size_t idx);

#endif //SHUNTINGYARD_H
