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

typedef enum BP_REASON {
    BP_REASON_STEP_OVER,
    BP_REASON_STEP_OUT,
    BP_REASON_BREAK_CAUSE,
    BP_REASON_USER,
    BP_REASON_COUNT
} BP_REASON;

extern const char* BP_REASON_STRS[BP_REASON_COUNT];

typedef struct BP {
    BP_REASON reason;
    uintptr_t cfa;
    void (*callback)(void* data);
    void* data;
    bool defer;
} BP;

ARRAY_PROTO(BP, BP)

typedef struct BPAddressInfo {
    uintptr_t address;
    BPInfo canonical_bp;
    uint16_t user_bp_count;
    uint16_t temp_bp_count;
    uint32_t bp_count;
    BPArray bps;
} BPAddressInfo;

int compare_bp_addr_info(uintptr_t bpa, uintptr_t bpb);
ARRAY_PROTO_CMP(BPAddressInfo, BPAddressInfo, compare_bp_addr_info, address)

extern BPAddressInfoArray bp_info;

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
    ACTION_BP_CAUSE,

    ACTION_CF_EXIT,
    ACTION_CF_SINGLE_STEP,
    ACTION_CF_STEP_OVER,
    ACTION_CF_STEP_INTO,
    ACTION_CF_STEP_OUT,
    ACTION_CF_CONTINUE,

    ACTION_AT_ATTACH, // ACTION_AT is ATTACH requests (attaching, detaching, running)
    ACTION_AT_OPEN,
    ACTION_AT_QUIT,

    ACTION_DS_REGS, // ACTION_DS is DISPLAY requests
    ACTION_DS_STACK_UNWIND,
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

    struct {
        bool is_simple;
    } BP_CAUSE;

    struct {
        const char* path;
    } AT_OPEN;
} ACTION_DATA;

typedef struct Action {
    ACTION_TYPE type;
    ACTION_DATA data;
} Action;

Action* create_action(ACTION_TYPE type, ACTION_DATA data);

typedef union RegValue {
    int64_t general;
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

#define ANURA_TARGET TARGET_LINUX_X64

// this is to be filled in by the defining target
// includes only the registers that fit in the word size of the machine
//  i.e. for this case 64 bits
#if ANURA_TARGET == TARGET_LINUX_X64
#include <sys/user.h>
typedef struct user_regs_struct GeneralRegs;
typedef struct CombinedRegs AllRegs;
typedef enum FLAGS {
    FLAG_ZERO= 0x40
} FLAGS;
typedef enum COMPARISONS {
    COMPARE_EQ,
    COMPARE_NEQ,
    COMPARE_GT,
    COMPARE_GTE,
    COMPARE_LESS,
    COMPARE_LESSEQ
} COMPARISONS;
#endif

ARRAY_PROTO(GeneralRegs, GeneralRegs)

typedef struct Data {
    uint8_t* raw_data;
    uint32_t data_size;
} Data;

typedef struct VRegInstance {
    const char* name;
    char* value;
} VRegInstance;
ARRAY_PROTO(VRegInstance, VRegInstance)

typedef struct VSection {
    uintptr_t vaddr_start;
    uintptr_t vaddr_end;
    size_t size;
    uint8_t* data;
} VSection;

typedef enum VTypeType {
    VTYPE_BASE,
    VTYPE_STRUCTURE,
    VTYPE_POINTER,
    VTYPE_ENUM,
    VTYPE_CONST,
    VTYPE_TYPEDEF
} VTypeType;

typedef struct VTypeBaseData {
    uint8_t encoding;
    const char* name;
} VTypeBaseData;

typedef struct VTypeStructElement {
    const char* name;
    struct VType* type;
    uint64_t type_ref;
    uint32_t offset;
} VTypeStructElement;
ARRAY_PROTO(VTypeStructElement, StructElem)

typedef struct VTypeStructData {
    const char* name;
    StructElemArray elements;
} VTypeStructData;

typedef struct VTypePointerData {
    struct VType* type;
    uint64_t type_ref;
} VTypePointerData;

typedef struct EnumElement {
    const char* name;
    int64_t value;
} EnumElement;
ARRAY_PROTO(EnumElement, EnumElement);

typedef struct VTypeEnumData {
    const char* name;
    uint8_t encoding;
    struct VType* base_type;
    uint64_t type_ref;
    EnumElementArray elements;
} VTypeEnumData;

typedef struct VTypeConstData {
    uint64_t type_ref;
    struct VType* type;
} VTypeConstData;

typedef struct VTypeTypedef {
    uint64_t type_ref;
    struct VType* type;
} VTypeTypedef;

typedef union VTypeData {
    VTypeBaseData base;
    VTypeStructData structure;
    VTypePointerData pointer;
    VTypeEnumData enumerator;
    VTypeConstData constant_mod;
    VTypeTypedef type_def;
} VTypeData;

typedef struct VType {
    uint64_t ref;
    VTypeType type;
    VTypeData data;
    uint16_t byte_size;
} VType;
int64_t vtype_cmp(const int64_t a, const int64_t b);
ARRAY_PROTO_CMP(VType, VType, vtype_cmp, ref);

typedef enum VLocationType {
    VLOCATION_REGISTER,
    VLOCATION_REG_OFF,
    VLOCATION_CONST,
    VLOCATION_ADDR,
    VLOCATION_NO_ACCESS,
    VLOCATION_EXPR
} VLocationType;

typedef union VLocationData {
    uint16_t register_id;
    struct {
        uint16_t register_id;
        int64_t offset;
    } reg_off;
    int64_t constant;
    uintptr_t vaddr;
    void* expr;
} VLocationData;

typedef struct VLocation {
    VLocationType type;
    VLocationData data;
} VLocation;

typedef struct VVar {
    const char* name;
    uint32_t line;
    uint32_t col;
    VType* type;
    uint64_t type_ref;
    VLocation loc;
} VVar;
ARRAY_PROTO_CMP(VVar, VVar, strcmp, name);

typedef enum ValueType {
    VALUE_NONE,
    VALUE_GENERAL_VALUE,
    VALUE_DATA
} ValueType;

typedef union ValueData {
    int64_t general;
    Data data;
} ValueData;

typedef struct Value {
    ValueType type;
    ValueData data;
} Value;

typedef struct VVarInstance {
    VVar* var;
    Value value;
} VVarInstance;
ARRAY_PROTO(VVarInstance, VVarInstance)

typedef struct VSub {
    uintptr_t vaddr_start;
    uintptr_t vaddr_end;
    const char* subprog_name;
    VVarArray vars;
    VVarArray params;
} VSub;

typedef struct StackFrame {
    uintptr_t pc;
    uintptr_t v_pc;
    uint32_t line;
    VSub* sub;
    GeneralRegs regs;
    uintptr_t cfa;
    uintptr_t end_stack_pointer;
    uint8_t* data;
    VVarInstanceArray vars;
    VVarInstanceArray args;
} StackFrame;

ARRAY_PROTO(StackFrame, StackFrame)
typedef StackFrameArray Stack;

int vsub_cmp(const uintptr_t a, const uintptr_t b);
ARRAY_PROTO_CMP(VSub, VSub, vsub_cmp, vaddr_start);

typedef struct SubIter {
    struct CU* cu;
    size_t idx;
} SubIter;

extern int tui_pipe[2];

#ifdef __GNUC__
#define PRINTF_LIKE(func) func __attribute__((format(printf, 1, 2)));
#else
#define PRINTF_LIKE(f) f;
#endif

void vlog(bool is_t, const char* message, va_list args);
PRINTF_LIKE(void tlog(const char* message, ...))
PRINTF_LIKE(void hlog(const char* message, ...))
int breakpoint_program(const char* program);
void print_breakpoints();
void display_labelled_regs(LabelledRegs lregs);

PRINTF_LIKE(void show_log(const char* message, ...))
void vshow_log(const char* message, va_list args, const char* prefix);
void show_newline();
PRINTF_LIKE(void show_err(const char* message, ...))

void open_program(const char* filepath);

#endif //MAIN_H
