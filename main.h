//
// Created by james on 30/10/25.
//

#ifndef MAIN_H
#define MAIN_H

#include "Array.h"
#include "QueueB.h"

#include <stdint.h>
#include <xmmintrin.h>

#include "Buffer.h"

typedef uint64_t PROCESS_ID;

extern QueueB action_q;

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

    uintptr_t addr;
    uint32_t line;

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
    uintptr_t address;
    BPInfo canonical_bp;
    uint16_t user_bp_count;
    uint16_t temp_bp_count;
    uint32_t bp_count;
} BPAddressInfo;

int compare_bp_addr_info(uintptr_t bpa, uintptr_t bpb);
ARRAY_PROTO_CMP(BPAddressInfo, BPAddressInfo, compare_bp_addr_info, address)

extern BPAddressInfoArray bp_info;
typedef enum BP_REASON {
    BP_REASON_STEP_OVER,
    BP_REASON_STEP_OUT,
    BP_REASON_USER,
} BP_REASON;

BPAddressInfo* get_or_add_bp_address_info(uintptr_t address, BPInfo info_if_none, BP_REASON reason);
void increment_by_reason(BPAddressInfo* info, BP_REASON reason);
void decrement_by_reason(BPAddressInfo* info, BP_REASON reason);

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
    ACTION_CF_STEP_INTO,
    ACTION_CF_STEP_OUT,
    ACTION_CF_CONTINUE,

    ACTION_AT_ATTACH, // ACTION_AT is ATTACH requests (attaching, detaching, running)
    ACTION_AT_QUIT,

    ACTION_DS_REGS, // ACTION_DS is DISPLAY requests
} ACTION_TYPE;

typedef union ACTION_DATA {
    int NO_DATA;

    struct {
        uintptr_t addr;
        uint32_t line;
    } BP_ADD;

    struct {
        uint32_t line;
        uintptr_t addr;
    } BP_REMOVE;

    struct {
        bool assembly_level;
    } CF_SINGLE_STEP;

    struct {
        const char* filepath;
    } AT_ATTACH;
} ACTION_DATA;

typedef struct Action {
    ACTION_TYPE type;
    ACTION_DATA data;
} Action;

Action* create_action(ACTION_TYPE type, ACTION_DATA data);

typedef union RegValue {
    uint64_t general;
    __m128 vector[4];
} RegValue;

typedef enum RegValueType {
    REGVAL_GENERAL,
    REGVAL_VECTOR,
    REGVAL_ERROR
} RegValueType;

typedef struct Reg {
    RegValueType type;
    RegValue value;
} Reg;

typedef struct LabelledReg {
    const char* name;
    uint16_t reg_num;
    Reg reg;
} LabelledReg;

ARRAY_PROTO(LabelledReg, LabelledReg)
typedef struct LabelledRegs {
    Buffer string_buff;
    LabelledRegArray regs;
} LabelledRegs;

void vlog(bool is_t, const char* message, va_list args);
void tlog(const char* message, ...);
void hlog(const char* message, ...);
int breakpoint_program(const char* program);
void print_breakpoints();
void display_labelled_regs(LabelledRegs lregs);

void show_log(const char* message, ...);
void show_err(const char* message, ...);

void open_program(const char* filepath);

#endif //MAIN_H
