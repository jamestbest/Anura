//
// Created by James Coward on 10/20/25.
//

#include "Linux.h"

#include "../Saruman/Saruman.h"
#include "Array.h"
#include "Sauron.h"

#include <elf.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "Errors.h"
#include "DWARF/Balin.h"
#include "Saruman/FrameInfo.h"

static uintptr_t virtual_to_runtime_addr(uintptr_t v_addr);
static uintptr_t runtime_to_virtual_addr(uintptr_t r_addr);

static ino_t file_inode;

PROCESS_ID launch_process(const char* path, uint32_t argc, const char* argv[]) {
    struct stat file_stats;
    const int res= stat(path, &file_stats);

    if (res != 0) {
        show_err("Unable to read file stats on process launch\n");
        return -1;
    }

    if (pipe(target.stdio_pipe) == -1) {
        show_err("pipe");
        return -1;
    }

    show_log("The file inode is for %s\n", path);
    file_inode= file_stats.st_ino;

    const pid_t pid= fork();

    if (pid == 0) {
        ptrace(PTRACE_TRACEME, getpid(), NULL, 0);

        printf("Making the stdio pipes\n");
        // close(target.stdio_pipe[0]);
        // dup2(target.stdio_pipe[1], STDOUT_FILENO);
        // dup2(target.stdio_pipe[1], STDERR_FILENO);
        // close(target.stdio_pipe[1]);
        printf("They are now %d\n", target.stdio_pipe[1]);
        fflush(stdout);

        show_log("Im the sub proc with pid %d about to become %s\n", getpid(), path);
        // we're the sub proc
        int ret= execvp(path, (char* const*)argv);
        show_err("Sub process failed to execv\n");
        exit(127);
    } else {
        // close(target.stdio_pipe[1]);

        const int flags= fcntl(target.stdio_pipe[0], F_GETFL, 0);
        fcntl(target.stdio_pipe[0], F_SETFL, flags | O_NONBLOCK);
    }

    return pid;
}

long long attach_process(PROCESS_ID pid) {
    return ptrace(PTRACE_ATTACH, pid, NULL);
}

LineAddrRes get_addr_at_line(uint32_t line) {
    return line2startaddr(line);
}

long long remove_bp_at_line(uint32_t line, BP_REASON reason) {
    const LineAddrRes res= line2startaddr(line);
    if (!res.succ) return -1;

    return target.target_remove_bp_at_addr(res.addr, reason);
}

long long place_bp_at_line(uint32_t line) {
    const LineAddrRes res= get_addr_at_line(line);

    if (!res.succ) return -1;

    uintptr_t runtime= virtual_to_runtime_addr(res.addr);

    show_log("Runtime addr is %#lx\n", runtime);
    show_log("Addr used is %#lx\n", res.addr + 0x555555554000);

    return target.target_place_bp_at_addr(runtime, line, BP_REASON_USER);
}

int decode_file(const char* filepath) {
    FILE* elf= fopen(filepath, "r");

    if (!elf) {
        show_log("Cannot open filepath %s with errno %d %s\n", filepath, errno, strerror(errno));
        return -1;
    }

    return decode(elf);
}

typedef Elf64_Phdr ProgSeg;

typedef struct ProcMap {
    uintptr_t base;
    uintptr_t end;
    uint8_t perms;
    uint64_t offset;
    char* device;
    uint64_t inode;
    const char* path;
    ProgSeg* seg;
} ProcMap;

ARRAY_PROTO(ProcMap, ProcMap)
ARRAY_ADD(ProcMap, ProcMap)

ProcMapArray proc_maps= ProcMapARRAY_EMPTY;
ProcMapArray pt_load_maps= ProcMapARRAY_EMPTY;

ProgSeg* get_segment_at_offset(uintptr_t offset) {
    for (int i = 0; i < ELF.ProgHeader.header_count; ++i) {
        ProgSeg* seg= &ELF.ProgHeader.program_headers[i];

        if (seg->p_offset == offset) return seg;
    }

    return NULL;
}

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
            .end=end,
            .base=start,
            .device= device_str,
            .inode= inode,
            .perms= perms,
            .seg= get_segment_at_offset(offset)
        };

        if (map.inode == file_inode) {
            show_log("GOt matching indoe\n");
            ProcMap_arr_add(&pt_load_maps, map);
        } else show_log("Mismatch inode map %ld file %ld\n", map.inode, file_inode);

        ProcMap_arr_add(&proc_maps, map);
    }

    proc_maps.flags.sorted= true;
    pt_load_maps.flags.sorted= true;
}

void first_stopped() {
    load_proc_maps();
}

int vaddr_in_segment_range(const void* addrp, const void* segmentp) {
    const uintptr_t addr= *(const uintptr_t*)addrp;
    const ProgSeg* seg= segmentp;

    if (seg->p_vaddr > addr) {
        return -1;
    }

    if (seg->p_vaddr + seg->p_memsz < addr) {
        return 1;
    }

    return 0;
}

ProgSeg* get_segment_enclosing_vaddr(uintptr_t v_addr) {
    ProgSeg* seg= bsearch(
        &v_addr,
        ELF.ProgHeader.program_headers,
        ELF.ProgHeader.header_count,
        ELF.ProgHeader.header_size,
        vaddr_in_segment_range
    );

    return seg;
}

int raddr_in_pt_procmap(const void* r_addrp, const void* procmapp) {
    const uintptr_t addr= *(const uintptr_t*)r_addrp;
    const ProcMap* procMap= procmapp;

    show_log("Proc map %#lx %#lx\n", procMap->base, procMap->end);

    if ((uintptr_t)procMap->base > addr) {
        return -1;
    }
    if ((uintptr_t)procMap->end < addr) {
        return 1;
    }

    return 0;
}

int vaddr_in_pt_procmap_range(const void* addrp, const void* procmapp) {
    const uintptr_t addr= *(const uintptr_t*)addrp;
    const ProcMap* procMap= procmapp;

    const size_t map_size= procMap->end - procMap->base;
    show_log("Proc map %p %p\n", procMap->base, procMap->end);
    show_log("Got addr %lx checking against %lx-%lx\n", addr, procMap->offset, procMap->offset + map_size);

    if (procMap->offset > addr) {
        return -1;
    }
    if (procMap->offset + map_size < addr) {
        return 1;
    }

    return 0;
}

ProcMap* get_procmap_at_vaddr(uintptr_t vaddr) {
    show_log("There are %zu entries\n", pt_load_maps.pos);
    const uint pos= ProcMap_arr_search(&pt_load_maps, &vaddr, vaddr_in_pt_procmap_range);

    if (pos == (uint)-1) return NULL;

    return ProcMap_arr_ptr(&pt_load_maps, pos);
}

ProcMap* get_procmap_at_raddr(uintptr_t raddr) {
    show_log("There are %zu entries\n", pt_load_maps.pos);
    const uint pos= ProcMap_arr_search(&pt_load_maps, &raddr, raddr_in_pt_procmap);
    show_log("POSITION RESULT: %u\n", pos);

    if (pos == (uint)-1) return NULL;

    return ProcMap_arr_ptr(&pt_load_maps, pos);
}

uintptr_t virtual_to_runtime_addr(uintptr_t v_addr) {
    const ProgSeg* segment= get_segment_enclosing_vaddr(v_addr);

    uintptr_t s_vaddr= segment->p_vaddr;

    const ProcMap* proc_map= get_procmap_at_vaddr(s_vaddr);

    if (proc_map == NULL) {
        return 0;
    }

    uintptr_t s_paddr= proc_map->base;

    return s_paddr + (v_addr - s_vaddr);
}

uintptr_t runtime_to_virtual_addr(uintptr_t r_addr) {
    const ProcMap* proc_map= get_procmap_at_raddr(r_addr);
    if (proc_map == NULL) return 0;

    ProgSeg* seg= proc_map->seg;
    show_log("Segment is %p\n",seg);
    if (seg == NULL) return 0;

    uintptr_t s_paddr= seg->p_vaddr;
    show_log("segment pvaddr: %#lx\n", s_paddr);

    uintptr_t res= s_paddr + (r_addr - proc_map->base);
    show_log("Res; %#lx\n", res);
    return res;
}

long long unsafe_single_step() {
    return ptrace(PTRACE_SINGLESTEP, target.pid, 0, NULL);
}

long long cf_single_step_assembly() {
    return target.target_cf_main(false);
}

uint64_t get_cfa(bool* succ) {
    const uintptr_t pc= target.target_get_pc();
    const uintptr_t vaddr= target.target_addr_runtime_to_virtual(pc);

    return cfa_value_at(vaddr, succ);
}

uintptr_t get_return_addr() {
    const uintptr_t pc= target.target_get_pc();
    const uintptr_t vaddr= target.target_addr_runtime_to_virtual(pc);

    const FDE_Entry* fde= get_fde_for_pc(vaddr);
    const CIE_Entry* cie= fde->cie_entry;

    bool succ;
    return reg_value_at(vaddr, &succ, cie->return_address_register);
}

#include <sys/uio.h>
ssize_t process_vm_readv(pid_t pid,
                         const struct iovec *local_iov,
                         unsigned long liovcnt,
                         const struct iovec *remote_iov,
                         unsigned long riovcnt,
                         unsigned long flags);

ssize_t process_vm_writev(pid_t pid,
                          const struct iovec *local_iov,
                          unsigned long liovcnt,
                          const struct iovec *remote_iov,
                          unsigned long riovcnt,
                          unsigned long flags);

Data get_data_runtime(runtime_addr addr, uint32_t bytes) {
    char* buff= malloc(bytes);
    struct iovec l;
    l.iov_base= buff;
    l.iov_len= bytes;
    struct iovec r;
    r.iov_base= (void*)addr;
    r.iov_len= bytes;

    const ssize_t read= process_vm_readv(target.pid, &l, 1, &r, 1, 0);
    if (read != bytes) {
        perror("Cannot read vm readv\n");
    }
    hlog("Read %zu byte from target\n", read);
    for (int j = 0; j < bytes; ++j) {
        show_log("%02hhX ", buff[j]);
    }
    putchar('\n');

    return (Data) {
        .raw_data= l.iov_base,
        .data_size= bytes,
    };
}

Data get_data_virtual(virtual_addr addr, uint32_t bytes) {
    const runtime_addr raddr= target.target_addr_runtime_to_virtual(addr);

    return get_data_runtime(raddr, bytes);
}

const char* info_main_file_path() {
    const DIE* main_cu= get_main_cu();
    show_log("Got the main cu as %p\n", main_cu);

    if (main_cu == NULL) return NULL;

    return cu_get_filename(main_cu);
}

void linux_init_target(Target* target) {
    target->target_update_after_process_first_stopped= first_stopped;

    target->target_launch_process= launch_process;
    target->target_attach_process= attach_process;

    target->target_decode_file= decode_file;

    target->target_get_addr_of_line= get_addr_at_line;
    target->target_place_bp_at_line= place_bp_at_line;
    target->target_remove_bp_at_line= remove_bp_at_line;

    target->target_cf_single_step_assembly= cf_single_step_assembly;
    target->target_unsafe_single_step= unsafe_single_step;

    target->target_addr_runtime_to_virtual= runtime_to_virtual_addr;
    target->target_addr_virtual_to_runtime= virtual_to_runtime_addr;
    target->target_get_return_addr= get_return_addr;
    target->target_get_cfa= get_cfa;

    target->target_get_data_runtime= get_data_runtime;
    target->target_get_data_virtual= get_data_virtual;

    target->target_info_main_file_path= info_main_file_path;
}
