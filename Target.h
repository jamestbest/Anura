//
// Created by james on 30/10/25.
//

#ifndef TARGET_H
#define TARGET_H

long long place_bp(void* address);

typedef struct Target {
    long long (*place_bp)(void* addr);
} Target;

typedef enum TARGETS {
    TARGET_LINUX_X64
} TARGETS;

extern Target target;

int init_target(TARGETS target);

#endif //TARGET_H
