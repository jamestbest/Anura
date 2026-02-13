//
// Created by james on 30/10/25.
//

#ifndef TARGET_H
#define TARGET_H

#include <stdint.h>
#include <stdbool.h>

typedef uint64_t PROCESS_ID;

typedef struct LineAddrRes {
    bool succ;
    void* addr;
} LineAddrRes;

typedef struct AddrLineRes {
    bool succ;
    uint64_t line;
} AddrLineRes;

typedef struct Target {
    PROCESS_ID pid;

    long long (*target_place_bp_at_line)(uint32_t line);
    long long (*target_place_bp_at_addr)(void* addr, uint32_t line);

    void (*target_breakpoint_hit_cleanup)();

    LineAddrRes (*target_get_addr_of_line)(uint32_t line);

    void (*target_update_after_process_first_stopped)();

    PROCESS_ID (*target_launch_process)(const char* path, uint32_t argc, const char* argv[]);
    long long (*target_attach_process)(PROCESS_ID pid);
    int (*target_decode_file)(const char* filepath);

    long long (*target_single_step_assembly)();

    void (*target_interrupt)();
    void (*target_interrupt_handler_setup)();

    uintptr_t (*target_get_pc)();
    long (*target_cf_continue)();

    void* (*target_addr_runtime_to_virtual)(void* r_addr);
} Target;

typedef enum TARGETS {
    TARGET_LINUX_X64
} TARGETS;

extern Target target;

int init_target(TARGETS target);

#endif //TARGET_H
