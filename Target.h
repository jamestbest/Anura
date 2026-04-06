//
// Created by james on 30/10/25.
//

#ifndef TARGET_H
#define TARGET_H

#include <stdint.h>
#include <stdbool.h>

#include "Buffer.h"
#include "main.h"
#include "Vector.h"

typedef struct LineAddrRes {
    bool succ;
    uintptr_t addr;
} LineAddrRes;

typedef struct AddrLineRes {
    bool succ;
    uint64_t line;
} AddrLineRes;

typedef uintptr_t runtime_addr;
typedef uintptr_t virtual_addr;

typedef struct Target {
    PROCESS_ID pid;

    int target_io_pipe[2];

    uintptr_t sw_bp_to_readd_addr;
    bool sw_bp_should_continue;

    long long (*target_place_bp_at_line)(uint32_t line);
    long long (*target_place_bp_at_addr)(uintptr_t addr, uint32_t line, BP_REASON reason);
    long long (*target_place_temp_bp)(uintptr_t addr, BP_REASON reason);
    long long (*target_place_bp_with_cfa)(uintptr_t addr, uintptr_t cfa, BP_REASON reason, void (*callback)(void* data), void* data);
    long long (*target_place_bp_defered)(uintptr_t addr, uintptr_t cfa, BP_REASON reason, void (*callback)(void* data), void* data);

    long long (*target_remove_bp_at_addr)(uintptr_t addr, BP_REASON reason);
    long long (*target_remove_bp_at_line)(uint32_t line, BP_REASON reason);
    long long (*target_remove_bp_at_addr_cfa)(uintptr_t addr, BP_REASON reason, uintptr_t cfa);

    void (*target_breakpoint_hit_cleanup)();

    LineAddrRes (*target_get_addr_of_line)(uint32_t line);

    void (*target_update_after_process_first_stopped)();

    PROCESS_ID (*target_launch_process)(const char* path, uint32_t argc, const char* argv[]);
    long long (*target_attach_process)(PROCESS_ID pid);
    int (*target_decode_file)(const char* filepath);

    long long (*target_unsafe_single_step)();
    long long (*target_unsafe_continue)();

    long long (*target_cf_single_step_assembly)();
    long long (*target_cf_continue)();
    long long (*target_cf_main)(bool is_continue);

    void (*target_interrupt)();
    void (*target_interrupt_handler_setup)();

    uintptr_t (*target_get_pc)();

    uintptr_t (*target_addr_runtime_to_virtual)(uintptr_t r_addr);
    uintptr_t (*target_addr_virtual_to_runtime)(uintptr_t v_addr);

    uintptr_t (*target_get_return_addr)();
    uint64_t (*target_get_cfa)(bool* succ);

    Reg (*target_get_reg)(uint16_t register_id);
    Reg (*target_get_reg_using)(uint16_t register_id, GeneralRegs* regs);
    uint64_t (*target_get_general_reg_at)(uintptr_t addr);
    bool (*target_set_reg_struct_value)(GeneralRegs* regs, uint16_t register_id, uint64_t value);
    GeneralRegs (*target_get_general_regs)(bool* succ);
    AllRegs (*target_get_all_regs)(bool* succ);
    VRegInstanceArray (*target_get_all_regs_instance)(AllRegs* regs);
    uint64_t (*target_get_general_reg_using)(uint16_t register_id, GeneralRegs* regs, bool* succ);

    long long (*target_aligned_write)(uintptr_t address, uint8_t value, uint8_t* existing_value);
    long long (*target_readd_sw_bp)(BPInfo* bp);

    LabelledRegs (*target_get_labelled_regs)();

    Data (*target_get_data_runtime)(runtime_addr runtime_addr, uint32_t bytes);
    Data (*target_get_data_virtual)(virtual_addr virtual_addr, uint32_t bytes);
    int64_t (*target_get_general_data_runtime)(runtime_addr runtime_addr, uint8_t bytes);

    const char* (*target_info_main_file_path)();

    void (*target_unwind_stack)();

    bool (*target_check_comparison)(COMPARISONS comparison);

    VSection (*target_get_text_section)();
    VSub (*target_get_next_sub)(SubIter* iter, bool* succ);
    const char* (*target_get_subroutine_name_at)(uintptr_t v_addr);
    VSub* (*target_get_vsub_at)(uintptr_t v_addr);

    LineAddrRes (*target_line_to_addr)(uint32_t line);
    AddrLineRes (*target_addr_to_line)(uintptr_t addr);

    SubIter (*target_get_selected_cu_sub_iter)();
    void (*target_load_cu)(const char* filepath);
    Vector (*target_get_all_cu_filenames)();
    void (*target_set_selected_cu)(void* cu);
    void* (*target_get_selected_cu)();
    void* (*target_get_main_cu)();
    const char* (*target_create_var_instance_string)(VVarInstance* inst);
} Target;

typedef enum TARGETS {
    TARGET_NONE= 0,
    TARGET_LINUX_X64= 1
} TARGETS;

extern Target target;

int init_target(TARGETS target);

#endif //TARGET_H
