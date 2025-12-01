//
// Created by jamestbest on 10/20/25.
//

#include "Linux.h"

#include "Array.h"
#include "Saruman.h"
#include "Sauron.h"

#include <elf.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <unistd.h>

static void* virtual_to_runtime_addr(void* v_addr);

static ino_t file_inode;

PROCESS_ID launch_process(const char* path, uint32_t argc, const char* argv[]) {
    struct stat file_stats;
    int res= stat(path, &file_stats);

    if (res != 0) {
        perror("Unable to read file stats on process launch\n");
        return -1;
    }

    printf("The file inode is for %s\n", path);
    file_inode= file_stats.st_ino;

    pid_t pid= fork();

    if (pid == 0) {
        ptrace(PTRACE_TRACEME, getpid(), NULL, 0);
        printf("Im the sub proc with pid %d about to become %s\n", getpid(), path);
        // we're the sub proc
        int ret= execvp(path, (char* const*)argv);
        perror("Sub process failed to execv\n");
        exit(127);
    }

    return pid;
}

long long attach_process(PROCESS_ID pid) {
    return ptrace(PTRACE_ATTACH, pid, NULL);
}

LineAddrRes get_addr_at_line(uint32_t line) {
    return line2startaddr(line);
}

long long place_bp_at_line(uint32_t line) {
    LineAddrRes res= get_addr_at_line(line);

    if (!res.succ) return -1;

    void* runtime= virtual_to_runtime_addr(res.addr);

    printf("Runtime addr is %p\n", runtime);
    printf("Addr used is %p\n", res.addr + 0x555555554000);

    return target.target_place_bp_at_addr(runtime, line);
}

int decode_file(const char* filepath) {
    FILE* elf= fopen(filepath, "r");

    if (!elf) {
        printf("Cannot open filepath %s with errno %d %s\n", filepath, errno, strerror(errno));
        return -1;
    }

    return decode(elf);
}

typedef Elf64_Phdr ProgSeg;

typedef struct ProcMap {
    void* base;
    void* end;
    uint8_t perms;
    uint64_t offset;
    char* device;
    uint64_t inode;
    const char* path;
} ProcMap;

ARRAY_PROTO(ProcMap, ProcMap)
ARRAY_ADD(ProcMap, ProcMap)

ProcMapArray proc_maps= ProcMapARRAY_EMPTY;
ProcMapArray pt_load_maps= ProcMapARRAY_EMPTY;

void load_proc_maps() {
    // open /proc/pid/maps
    char buff[100];
    sprintf(buff, "/proc/%lu/maps", target.pid);
    FILE* f= fopen(buff, "r");

    if (!f) {
        perror("Unable to open /proc/<pid>/maps to load runtime address info");
        return;
    }

    char* line_buff;
    size_t line_buff_size= 0;

    proc_maps= ProcMap_arr_create();
    pt_load_maps= ProcMap_arr_create();

    while (getline(&line_buff, &line_buff_size, f) != -1) {
        ProcMap map;

        long long unsigned start, end, offset, inode;
        char perms_str[5];
        char device_str[32];

        int chars_read;
        int read= sscanf(
            line_buff,
            "%llx-%llx %4s %llx %31s %llu %n",
            &start,
            &end,
            perms_str,
            &offset,
            device_str,
            &inode,
            &chars_read
        );

        char c= line_buff[chars_read];
        while (c == '\t' || c == ' ') c= line_buff[++chars_read];

        char* filename;
        if (c != '\n') {
            size_t filename_size= line_buff_size - chars_read + 1;
            filename= malloc(sizeof(char) * filename_size);
            memcpy(filename, &line_buff[chars_read], filename_size);
            filename[filename_size - 1]= '\0';
        } else {
            filename= NULL;
        }

        uint8_t perms= 0;
        size_t i= 0;
        while (perms_str[i] != '\0') {
            switch (perms_str[i]) {
                case 'r': perms |= PF_R; break;
                case 'w': perms |= PF_W; break;
                case 'x': perms |= PF_X; break;
            }
            i++;
        }

        map= (ProcMap) {
            .offset= offset,
            .path= filename,
            .end=(void*)end,
            .base=(void*)start,
            .device= device_str,
            .inode= inode,
            .perms= perms
        };

        if (map.inode == file_inode) {
            printf("GOt matching indoe\n");
            ProcMap_arr_add(&pt_load_maps, map);
        } else printf("Mismatch inode map %ld file %ld\n", map.inode, file_inode);

        ProcMap_arr_add(&proc_maps, map);
    }

    proc_maps.flags.sorted= true;
    pt_load_maps.flags.sorted= true;
}

void first_stopped() {
    load_proc_maps();
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

int vaddr_in_pt_procmap_range(const void* addrp, const void* procmapp) {
    const uintptr_t addr= *(const uintptr_t*)addrp;
    const ProcMap* procMap= (const ProcMap*)procmapp;

    size_t map_size= procMap->end - procMap->base;
    printf("Proc map %p %p\n", procMap->base, procMap->end);
    printf("Got addr %lx checking against %lx-%lx\n", addr, procMap->offset, procMap->offset + map_size);

    if (procMap->offset > addr) {
        return -1;
    }
    if (procMap->offset + map_size < addr) {
        return 1;
    }

    return 0;
}

ProcMap* get_procmap_at_vaddr(void* vaddr) {
    printf("There are %zu entries\n", pt_load_maps.pos);
    uint pos= ProcMap_arr_search(&pt_load_maps, &vaddr, vaddr_in_pt_procmap_range);

    if (pos == (uint)-1) return NULL;

    return ProcMap_arr_ptr(&pt_load_maps, pos);
}

void* virtual_to_runtime_addr(void* v_addr) {
    ProgSeg* segment= get_segment_enclosing_vaddr(v_addr);

    void* s_vaddr= (void*)segment->p_vaddr;

    ProcMap* proc_map= get_procmap_at_vaddr(s_vaddr);

    if (proc_map == NULL) {
        perror("TEST");
        return NULL;
    }

    void* s_paddr= proc_map->base;

    return s_paddr + (v_addr - s_vaddr);
}

long long single_step_assembly() {
    return ptrace(PTRACE_SINGLESTEP, target.pid, 0, NULL);
}

void linux_init_target(Target* target) {
    target->target_update_after_process_first_stopped= first_stopped;

    target->target_launch_process= launch_process;
    target->target_attach_process= attach_process;

    target->target_decode_file= decode_file;

    target->target_get_addr_of_line= get_addr_at_line;
    target->target_place_bp_at_line= place_bp_at_line;

    target->target_single_step_assembly= single_step_assembly;
}