//
// Created by jamestbest on 4/7/26.
//

#ifndef ANURA_BREAK_SAVE_H
#define ANURA_BREAK_SAVE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "Errors.h"
#include "main.h"
#include "Target.h"

int break_save();

#define BASE POINT base;

typedef enum CF_REASON {
    CF_REASON_FRAME_START,
    CF_REASON_FRAME_END,
    CF_REASON_PRE_FUNC_CALL,
    CF_REASON_POST_FUNC_CALL,
    CF_REASON_PRE_CONDITIONAL,
    CF_REASON_CONDITIONAL_RESOLUTION,
    CF_REASON_CUSTOM
} CF_REASON;

typedef struct CF_POINT {
    BASE
    CF_REASON reason;
    const char* custom_reason;
} CF_POINT;

typedef enum DF_REASON {
    DF_REASON_VAR_ASSIGN,   // if the value of a variable has been assigned
    DF_REASON_RETURN_ASSIGN,// if the return value of a function has been assigned
    DF_REASON_MEMBER_ASSIGN,// if a member of a struct/union is updated
    DF_REASON_ARG_ASSIGN    // if the value of a variable has been assigned by the function it was passed to
} DF_REASON;

typedef struct DF_POINT {
    BASE
    DF_REASON reason;
    const char* custom_reason;
    uintptr_t die_offset;
    VLocation loc;
} DF_POINT;

extern const char* POINT_TYPE_STRS[];
extern const char* DF_POINT_REASON_STRS[];
extern const char* CF_POINT_REASON_STRS[];

#include <gtk/gtk.h>
gboolean display_break_save_tree(gpointer root);

#endif //ANURA_BREAK_SAVE_H
