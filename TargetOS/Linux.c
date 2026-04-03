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

    show_log("The file inode is for %s\n", path);
    file_inode= file_stats.st_ino;

    const pid_t pid= fork();

    if (pid == 0) {
        ptrace(PTRACE_TRACEME, getpid(), NULL, 0);

        printf("Making the stdio pipes!\n");
        fflush(stdout);
        close(target.target_io_pipe[0]);
        dup2(target.target_io_pipe[1], STDOUT_FILENO);
        // dup2(target.stdio_pipe[1], STDERR_FILENO);
        close(target.target_io_pipe[1]);

        printf("Im the sub proc with pid %d about to become %s\n", getpid(), path);

        // we're the sub proc
        int ret= execvp(path, (char* const*)argv);
        show_err("Sub process failed to execv\n");
        exit(127);
    } else {
        close(target.target_io_pipe[1]);

        const int flags= fcntl(target.target_io_pipe[0], F_GETFL, 0);
        fcntl(target.target_io_pipe[0], F_SETFL, flags | O_NONBLOCK);
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

    const uintptr_t r_addr= target.target_addr_virtual_to_runtime(res.addr);
    return target.target_remove_bp_at_addr(r_addr, reason);
}

long long place_bp_at_line(uint32_t line) {
    const LineAddrRes res= get_addr_at_line(line);

    if (!res.succ) return -1;

    const uintptr_t runtime= virtual_to_runtime_addr(res.addr);

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
            size_t filename_size= line_buff_size - chars_read;
            filename= malloc(sizeof(char) * filename_size + 1);
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

    // show_log("Proc map %#lx %#lx\n", procMap->base, procMap->end);

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
    // show_log("Proc map %p %p\n", procMap->base, procMap->end);
    // show_log("Got addr %lx checking against %lx-%lx\n", addr, procMap->offset, procMap->offset + map_size);

    if (procMap->offset > addr) {
        return -1;
    }
    if (procMap->offset + map_size < addr) {
        return 1;
    }

    return 0;
}

ProcMap* get_procmap_at_vaddr(uintptr_t vaddr) {
    // show_log("There are %zu entries\n", pt_load_maps.pos);
    const uint pos= ProcMap_arr_search(&pt_load_maps, &vaddr, vaddr_in_pt_procmap_range);

    if (pos == (uint)-1) return NULL;

    return ProcMap_arr_ptr(&pt_load_maps, pos);
}

ProcMap* get_procmap_at_raddr(uintptr_t raddr) {
    // show_log("There are %zu entries\n", pt_load_maps.pos);
    const uint pos= ProcMap_arr_search(&pt_load_maps, &raddr, raddr_in_pt_procmap);
    // show_log("POSITION RESULT: %u\n", pos);

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
    // show_log("Segment is %p\n",seg);
    if (seg == NULL) return 0;

    uintptr_t s_paddr= seg->p_vaddr;
    // show_log("segment pvaddr: %#lx\n", s_paddr);

    uintptr_t res= s_paddr + (r_addr - proc_map->base);
    // show_log("Res; %#lx\n", res);
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

    const FDE_Entry* fde= get_fde_for_virtual_pc(vaddr);
    const CIE_Entry* cie= fde->cie_entry;

    bool succ;
    return restore_reg_value_at(vaddr, &succ, cie->return_address_register);
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
        show_err("Cannot read vm readv\n");
    }

    return (Data) {
        .raw_data= l.iov_base,
        .data_size= bytes,
    };
}

Data get_data_virtual(virtual_addr addr, uint32_t bytes) {
    const runtime_addr raddr= target.target_addr_virtual_to_runtime(addr);

    return get_data_runtime(raddr, bytes);
}

const char* info_main_file_path() {
    const DIE* main_cu= get_main_cu();
    show_log("Got the main cu as %p\n", main_cu);

    if (main_cu == NULL) return NULL;

    return cu_get_filename(main_cu);
}

StackFrame create_stack_frame(const uintptr_t pc, const GeneralRegs regs, bool pc_is_return_addr) {
    const uintptr_t v_addr= target.target_addr_runtime_to_virtual(pc);
    const char* sub_name= get_subprog_name_at(v_addr);

    AddrLineRes res;
    if (pc_is_return_addr)
        res= addr2line(v_addr - 1);
    else res= addr2line(v_addr);

    return (StackFrame) {
        .pc= pc,
        .v_pc= v_addr,
        .line= res.line,
        .subprog_name= sub_name,
        .regs= regs,
    };
}

void show_stack(const Stack* stack) {
    show_log("Stack trace\n");
    for (int i = 0; i < stack->pos; ++i) {
        const StackFrame* frame= StackFrame_arr_ptr(stack, i);

        show_log("\tFrame @ %#lx (v: %#lx)\n", frame->pc, frame->v_pc);
        show_log("\t\tSubroutine: %s:%u\n", frame->subprog_name, frame->line);
        show_log("\t\tRegs: \n");
    }
}

void unwind_stack() {
    uintptr_t start= target.target_get_pc();
    uintptr_t v_addr= target.target_addr_runtime_to_virtual(start);
    FDE_Entry* s_fde= get_fde_for_virtual_pc(v_addr);

    if (!s_fde) {
        show_err("Unable to unwind stack, no fde for pc %#lx (v: %#lx) found\n", start, v_addr);
        return;
    }

    show_log("The stack is currently at addr %#lx (v: %#lx) in function %s\n", start, v_addr, get_subprog_name_at(v_addr));

    bool succ;
    FrameRow row= get_frame_row_at(v_addr, &succ);
    if (!succ) {
        show_err("Unable to get frame row to unwind stack for location %#lx (v: %#lx)\n", start, v_addr);
        return;
    }


    Stack stack= StackFrame_arr_create();

    GeneralRegs regs= target.target_get_general_regs(&succ);
    if (!succ) {
        show_err("Unable to get base regs when unwinding the stack\n");
        return;
    }

    StackFrame_arr_add(&stack, create_stack_frame(start, regs, false));

    RegisterRule rule;
    while (rule= get_rule_for(&row, s_fde->cie_entry->return_address_register, &succ),
        succ && rule.op != DW_CFA_undefined
    ) {
        const uint64_t cfa= eval_cfa_rule_using(row.cfa_rule, &regs);
        const uintptr_t return_addr= eval_register_rule_using(&rule, s_fde->cie_entry->return_address_register, cfa, &regs);
        const uintptr_t v_addr= target.target_addr_runtime_to_virtual(return_addr);

        show_log("The stack is currently at addr %#lx (v: %#lx) in function %s\n", return_addr, v_addr, get_subprog_name_at(v_addr));

        s_fde= get_fde_for_virtual_pc(v_addr);
        if (!s_fde) break; // here we're probably in something like _entry

        row= get_frame_row_at(v_addr, &succ);

        if (!succ) {
            show_err("Unable to unwind the stack further\n");
            break;
        }

        GeneralRegs* old_regs= &StackFrame_arr_peek(&stack)->regs;
        regs= restore_regs(&row, old_regs, cfa, s_fde->cie_entry->return_address_register);
        StackFrame_arr_add(&stack, create_stack_frame(return_addr, regs, true));
    }

    show_stack(&stack);
}

VSection get_text_section() {
    const uintptr_t start= ELF.section_map.text.header->sh_addr;
    const uintptr_t size= ELF.section_map.text.header->sh_size;
    const uintptr_t end= start + size;

    return (VSection) {
        .data= ELF.section_map.text.data,
        .vaddr_start= start,
        .vaddr_end= end,
        .size= size
    };
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

    target->target_unwind_stack= unwind_stack;

    target->target_get_text_section= get_text_section;
    target->target_get_next_sub= next_sub;
    target->target_get_subroutine_name_at= get_subprog_name_at;

    target->target_line_to_addr= line2startaddr;
    target->target_addr_to_line= addr2line;
}
