//
// Created by james on 30/10/25.
//

#ifndef TARGET_H
#define TARGET_H

#include <stdint.h>

typedef uint64_t PROCESS_ID;

typedef struct Target {
    long long (*place_bp)(void* addr);
    PROCESS_ID (*target_launch_process)(const char* path, uint32_t argc, const char* argv[]);
} Target;

typedef enum TARGETS {
    TARGET_LINUX_X64
} TARGETS;

extern Target target;

int init_target(TARGETS target);

#endif //TARGET_H
