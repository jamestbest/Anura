//
// Created by jamestbest on 4/5/26.
//

#ifndef ANURA_EXPR_EVAL_H
#define ANURA_EXPR_EVAL_H

#include "main.h"
#include "Target.h"
#include "Saruman/DWARFParsing.h"
#include "shared/Array.h"

VLocation eval_dw_expr_to_location(DW_EXPR* expr, uintptr_t fbreg);

#endif //ANURA_EXPR_EVAL_H
