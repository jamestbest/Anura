//
// Created by james on 30/10/25.
//

#ifndef MAIN_H
#define MAIN_H

#include "QueueB.h"

#include <stdint.h>
#include "Array.h"

extern QueueB action_q;
extern pid_t t_pid;

typedef enum BP_TYPE {
    BP_HARDWARE,
    BP_SOFTWARE,

    BP_SOURCE_SINGLE_STEP_TRAP,

    BP_TYPE_COUNT
} BP_TYPE;

extern const char* const BP_TYPE_STRS[BP_TYPE_COUNT];

int BP_type_is_user(BP_TYPE type);

typedef struct BPInfo {
    BP_TYPE type;

    union {
        // we could just save the contents of the word, but there may be another bp or code having been changed in between,
        //  so we'll just store the single byte. This won't help if something else overwrites the underlying value
        //  (this requires the bp to be placed, value overwritten and then bp removed)
        unsigned char shadow;
        unsigned char bp;
    } data;
} BPInfo;

ARRAY_PROTO(BPInfo, BPInfo)

typedef struct BPAddressInfo {
    void* address;

    BPInfoArray bps;
} BPAddressInfo;

ARRAY_PROTO(BPAddressInfo, BPAddressInfo)

extern BPAddressInfoArray bp_info;

BPAddressInfo* get_or_add_bp_address_info(void* address);

typedef enum ACTION_GTYPE {
    ACTION_GTYPE_BREAK_POINT,
    ACTION_GTYPE_CONTROL_FLOW,
} ACTION_GTYPE;

typedef enum ACTION_TYPE {
    ACTION_BP_ADD,
    ACTION_BP_REMOVE,
    ACTION_BP_LIST,

    ACTION_CF_EXIT,
    ACTION_CF_SINGLE_STEP,
    ACTION_CF_STEP_OVER,
    ACTION_CF_STEP_INFO,
    ACTION_CF_CONTINUE
} ACTION_TYPE;

typedef union ACTION_DATA {
    int NO_DATA;

    struct {
        uint32_t line;
        void* addr;
    } BP_ADD;

    struct {
        uint32_t line;
        void* addr;
    } BP_REMOVE;
} ACTION_DATA;

typedef struct Action {
    ACTION_TYPE type;
    ACTION_DATA data;
} Action;

Action* create_action(ACTION_TYPE type, ACTION_DATA data);

void vlog(bool is_t, const char* message, va_list args);
void log(bool is_t, const char* message, ...);
void tlog(const char* message, ...);
void hlog(const char* message, ...);
int breakpoint_program(const char* program);
#endif //MAIN_H
