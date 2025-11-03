//
// Created by jamestbest on 10/20/25.
//

#include "Linux.h"

#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <unistd.h>

#include <elf.h>

PROCESS_ID launch_process(const char* path, uint32_t argc, const char* argv[]) {
    pid_t pid= fork();

    if (pid == 0) {
        ptrace(PTRACE_TRACEME, getpid(), NULL, 0);
        printf("Im the sub proc with pid %d\n", getpid());
        // we're the sub proc
        int ret= execvp(path, (char* const*)argv);

        exit(0);
    }

    return pid;
}

long long attach_process(PROCESS_ID pid) {
    return ptrace(PTRACE_ATTACH, pid, 0, 0);
}



void load_proc_segs() {

}

void load_proc_maps() {

}

Elf64_Phdr* get_segment_enclosing_vaddr(void* v_addr) {
    return NULL;
}

typedef Elf64_Phdr ProcSeg;

typedef struct ProcMap {
    void* base;
    void* end;
    uint8_t perms;
    uint64_t offset;
    struct {
        uint8_t major;
        uint8_t minor;
    } dev;
    uint64_t inode;
    const char* path;
} ProcMap;

void* v_to_p_addr(void* v_addr) {
//    ProcSeg* segment= get_segment_enclosing_vaddr(v_addr);
//
//    void* s_vaddr= (void*)segment->p_vaddr;
//
//    ProcMap* proc_map= get_procmap_at_vaddr(s_vaddr);
//    void* s_paddr= proc_map->base;
//
//    return s_paddr + (v_addr - s_vaddr);
}

void linux_init_target(Target* target) {
    target->target_launch_process= launch_process;
    target->target_attach_process= attach_process;
}