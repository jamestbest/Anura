//
// Created by jamestbest on 10/20/25.
//

#include "Linux.h"
#include "Sauron.h"
#include "Saruman.h"

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

LineAddrRes get_addr_at_line(uint32_t line) {
    return line2startaddr(line);
}

long long place_bp_at_line(uint32_t line) {
    LineAddrRes res= line2startaddr(line);

    if (!res.succ) return -1;

    return target.target_place_bp_at_addr(res.addr);
}

int decode_file(const char* filepath) {
    FILE* elf= fopen(filepath, "r");
    return decode(elf);
}

typedef Elf64_Phdr ProgSeg;

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

void load_proc_maps() {

}

int vaddr_in_segment_range(const void* addrp, const void* segmentp) {
    uintptr_t addr= *(const uintptr_t*)addrp;
    const ProgSeg* seg= (const ProgSeg*)segmentp;

    if (seg->p_vaddr > addr) {
        return -1;
    }

    if (seg->p_vaddr + seg->p_memsz < addr) {
        return 1;
    }

    return 0;
}

ProgSeg* get_segment_enclosing_vaddr(void* v_addr) {
    ProgSeg* seg= bsearch(
        &v_addr,
        ELF.ProgHeader.program_headers,
        ELF.ProgHeader.header_count,
        ELF.ProgHeader.header_size,
        vaddr_in_segment_range
    );

    return seg;
}

void* v_to_p_addr(void* v_addr) {
    ProgSeg* segment= get_segment_enclosing_vaddr(v_addr);

    void* s_vaddr= (void*)segment->p_vaddr;

//    ProcMap* proc_map= get_procmap_at_vaddr(s_vaddr);
//    void* s_paddr= proc_map->base;

//    return s_paddr + (v_addr - s_vaddr);
}

void linux_init_target(Target* target) {
    target->target_launch_process= launch_process;
    target->target_attach_process= attach_process;

    target->target_decode_file= decode_file;

    target->target_get_addr_of_line= get_addr_at_line;
    target->target_place_bp_at_line= place_bp_at_line;
}