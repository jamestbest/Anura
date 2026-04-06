#include "main.h"

#include "Array.h"
#include "IsildursBane.h"
#include "MtDoom/MtDoom.h"
#include "Palantir.h"
#include "QueueB.h"
#include "Saruman/Saruman.h"
#include "Sauron.h"
#include "Target.h"
#include "Palantir/Palantir.h"

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <gtk/gtk.h>

#define PCOND_X(cond) (cond ? 'X' : ' ')

const char* const RW_MAP[4]= {
    [0b00]= "BRK INST",
    [0b01]= "BRK WRITES",
    [0b10]= "BRK IO",
    [0b11]= "BRK RW NO INST FETCH"
};

const unsigned int LEN_MAP[4]= {
    [0b00]= 1,
    [0b01]= 2,
    [0b10]= 8,
    [0b11]= 4
};

const char* BP_REASON_STRS[BP_REASON_COUNT]= {
    [BP_REASON_STEP_OUT]= "STEP OUT",
    [BP_REASON_STEP_OVER]= "STEP OVER",
    [BP_REASON_BREAK_CAUSE]= "BREAK CAUSE",
    [BP_REASON_USER]= "USER"
};

void hlog(const char* message, ...);

void print_bp_status(long long r7, unsigned int reg, const char* prefix) {
    unsigned int offset= reg << 1;
    bool l_active= (r7 >> offset) & 1;
    bool g_active= (r7 >> (offset + 1)) & 1;

    offset <<= 1;
    unsigned int rw = ((r7 >> 16) >> offset) & 0b11;
    unsigned int len= ((r7 >> 18) >> offset) & 0b11;

    printf(
        "%sDR%u: Local --%c-- Global --%c-- %s %u byte(s)\n",
        prefix,
        reg,
        l_active ? 'X' : ' ',
        g_active ? 'X' : ' ',
        RW_MAP[rw],
        LEN_MAP[len]
    );
}

void print_dr_status(long long r7) {
    hlog(
        "Breakpoint register status' (%llu) %s%s%s%s\n",
        r7,
        (r7 >>  8) & 1 ? "LE " : "",
        (r7 >>  9) & 1 ? "GE " : "",
        (r7 >> 11) & 1 ? "RTM " : "",
        (r7 >> 13) & 1 ? "GD " : ""
    );
    for (int j = 0; j < 4; ++j) {
        print_bp_status(r7, j, "  ");
    }
}

void print_dr7(long long reg) {
    print_dr_status(reg);
}

void print_dr6(long long reg) {
    hlog("DR6 (Status Register)\n"
           "  - Triggers\n"
           "    - B0: %c\n"
           "    - B1: %c\n"
           "    - B2: %c\n"
           "    - B3: %c\n"
           "  - Trigger flags\n"
           "    - BLD (bus-lock): %c\n"
           "    - BD (debug reg access): %c\n"
           "    - BS (single step): %c\n"
           "    - BT (task switch): %c\n"
           "    - RTM (restricted transactional memory): %c\n\n",
           PCOND_X((reg >> 0) & 1),
           PCOND_X((reg >> 1) & 1),
           PCOND_X((reg >> 2) & 1),
           PCOND_X((reg >> 3) & 1),
           PCOND_X((reg >> 11) & 0), // BLD is on CLEAR
           PCOND_X((reg >> 13) & 1),
           PCOND_X((reg >> 14) & 1),
           PCOND_X((reg >> 15) & 1),
           PCOND_X((reg >> 16) & 0)  // RTM is on CLEAR
    );
}

const char* const BP_TYPE_STRS[BP_TYPE_COUNT]= {
    [BP_HARDWARE]= "HARDWARE",
    [BP_SOFTWARE]= "SOFTWARE",
    [BP_SOURCE_SINGLE_STEP_TRAP]= "SINGLE STEP TRAP"
};

int BP_type_is_user(BP_TYPE type) {
    return type == BP_HARDWARE || type == BP_SOFTWARE;
}

int compare_bp_addr_info(uintptr_t bpa, uintptr_t bpb) {
    return bpa - bpb;
}

int vsub_cmp(const uintptr_t a, const uintptr_t b) {
    return a - b;
}

int64_t vtype_cmp(const int64_t a, const int64_t b) {
    return a - b;
}

ARRAY_ADD(BPInfo, BPInfo)
ARRAY_ADD_CMP(BPAddressInfo, BPAddressInfo, compare_bp_addr_info, address)
ARRAY_ADD(StackFrame, StackFrame)
ARRAY_ADD(BP, BP)
ARRAY_ADD_CMP(VSub, VSub, vsub_cmp, vaddr_start)
ARRAY_ADD(VTypeStructElement, StructElem)
ARRAY_ADD_CMP(VVar, VVar, strcmp, name);
ARRAY_ADD_CMP(VType, VType, vtype_cmp, ref);
ARRAY_ADD(EnumElement, EnumElement);
ARRAY_ADD(VVarInstance, VVarInstance)
ARRAY_ADD(VRegInstance, VRegInstance)

BPAddressInfoArray bp_info;

int tui_pipe[2];

void increment_by_reason(BPAddressInfo* info, BP_REASON reason) {
    switch (reason) {
        case BP_REASON_STEP_OVER:
            info->temp_bp_count++;
            break;
        case BP_REASON_USER:
            info->user_bp_count++;
            break;
    }

    info->bp_count++;
}

void decrement_by_reason(BPAddressInfo* info, BP_REASON reason) {
    switch (reason) {
        case BP_REASON_STEP_OVER:
            info->temp_bp_count--;
            break;
        case BP_REASON_USER:
            info->user_bp_count--;
            break;
    }

    info->bp_count--;
}

BPAddressInfo* get_or_add_bp_address_info(uintptr_t address, BPInfo info_if_none, BP_REASON reason) {
    BPAddressInfo* info= BPAddressInfo_arr_search_ie(&bp_info, address);
    if (info) {
        increment_by_reason(info, reason);
        return info;
    }

    BPAddressInfo* addr_info= BPAddressInfo_arr_add_i(&bp_info);
    *addr_info= (BPAddressInfo){0};
    addr_info->address= address;
    addr_info->canonical_bp= info_if_none;
    addr_info->bps= BP_arr_construct(0);
    increment_by_reason(addr_info, reason);

    return addr_info;
}

void vlog(bool is_t, const char* message, va_list args) {
    printf("LOG(%s): ", is_t ? "TARGET" : " HOST ");
    vprintf(message, args);

    fflush(stdout);
}

void tlog(const char* message, ...) {
    va_list args;
    va_start(args, message);
    vlog(true, message, args);
    va_end(args);
}

void hlog(const char* message, ...) {
    va_list args;
    va_start(args, message);
    vshow_log(message, args, "LOG(HOST): ");
    va_end(args);
}

#include <pthread.h>

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

int breakpoint_program(const char* program) {
    bp_info= BPAddressInfo_arr_construct(4);

    const char* args[]= {
        program,
        "test",
        NULL
    };

    const PROCESS_ID pid= target.target_launch_process(program, sizeof(args), args);
    target.pid= pid;

    const long long res= target.target_attach_process(target.pid);
    hlog("The attach result is %lld errno is %d with error %s\n", res, errno, strerror(errno));
    printf("Set the t_pid to %lu\n", pid);

    return 0;
}

void print_breakpoints() {
    show_log("-----BREAKPOINTS-----\n");
    show_log("There are %zu locations with breakpoints\n", bp_info.pos);

    for (int j = 0; j < bp_info.pos; ++j) {
        const BPAddressInfo* addr_info= BPAddressInfo_arr_ptr(&bp_info, j);

        show_log("  - Addr: %#lx contains %u breakpoint(s) (%u USER and %u TEMP): \n", addr_info->address, addr_info->bp_count, addr_info->user_bp_count, addr_info->temp_bp_count);

        const BPInfo* bp= &addr_info->canonical_bp;

        show_log("    + Canonical BP of Type %s with data ", BP_TYPE_STRS[bp->type]);

        switch (bp->type) {
            case BP_HARDWARE:
                show_log("BP No. %u", bp->data.bp);
                break;
            case BP_SOFTWARE:
                show_log("SHADOW 0x%x", bp->data.shadow);
                break;
            case BP_SOURCE_SINGLE_STEP_TRAP:
                show_log("No data");
                break;
        }
        show_log(" on line %u", bp->line);

        if (addr_info->bps.pos != 0) {
            for (int i = 0; i < addr_info->bps.pos; ++i) {
                BP* bp= BP_arr_ptr(&addr_info->bps, i);
                show_log("      * BP for %s CFA: %#lx Callback: %p Data: %p Defered: %s\n",
                    BP_REASON_STRS[bp->reason],
                    bp->cfa,
                    bp->callback,
                    bp->data,
                    bp->defer ? "True" : "False"
                );
            }
        } else {
            show_log("\n");
        }
    }
}

// int read_test(void* loc) {
//     char buff[40];
//     struct iovec l;
//     l.iov_base= buff;
//     l.iov_len= sizeof (buff);
//     struct iovec r;
//     r.iov_base= (void*)loc;
//     r.iov_len= sizeof (buff);
//     ssize_t read= process_vm_readv(t_pid, &l, 1, &r, 1, 0);
//     if (read != sizeof (buff)) {
//         perror("Cannot read vm readv\n");
//     }
//     hlog("Read %zu byte from target\n", read);
//     for (int j = 0; j < sizeof (buff); ++j) {
//         printf("%02hhX ", buff[j]);
//     }
//     putchar('\n');
//
//     return 0;
// }

QueueB action_q;
void* bp_pos= 0;
int bp_line= 0;

Action* create_action(ACTION_TYPE type, ACTION_DATA data) {
    Action* action= malloc(sizeof (Action));

    action->type= type;
    action->data= data;

    return action;
}

void open_program(const char* filepath) {
    target.target_decode_file(filepath);

    queueb_push_blocking(&action_q, create_action(ACTION_AT_ATTACH, (ACTION_DATA) {
        .AT_ATTACH.filepath= filepath
    }));

    g_idle_add(guiup_main_file, (gpointer)target.target_info_main_file_path());
    g_idle_add(update_target_data, NULL);
}

char* va_to_string(const char* message, va_list args) {
    va_list arg_copy;
    va_copy(arg_copy, args);

    int size= vsnprintf(NULL, 0, message, arg_copy);
    va_end(arg_copy);

    size++;
    char* buffer= malloc(size);
    if (!buffer) return NULL;

    vsnprintf(buffer, size, message, args);

    return buffer;
}

void show_err(const char* message, ...) {
    va_list args;
    va_start(args, message);
    char* buffer= va_to_string(message, args);
    va_end(args);

    fprintf(stderr, "%s", buffer);

    g_idle_add(terminal_err, buffer);
}

void show_log(const char* message, ...) {
    va_list args;
    va_start(args, message);
    char* buffer= va_to_string(message, args);
    va_end(args);

    printf("%s", buffer);

    g_idle_add(terminal_log, buffer);
}

void vshow_log(const char* message, va_list args, const char* prefix) {
    char* buffer= va_to_string(message, args);
    const ssize_t len= strlen(buffer) + strlen(prefix) + 1;
    char* big_buff= malloc(len);
    snprintf(big_buff, len, "%s%s", prefix, buffer);
    free(buffer);

    printf("%s", big_buff);

    g_idle_add(terminal_log, big_buff);
}

void show_newline() {
    g_idle_add(terminal_newline, NULL);
}

void* tui_thread_create(void* data) {
    tui_setup();
    tui_loop();

    return NULL;
}

// talking about step over
// talking about things that arent' supported but there is information that could be in place by the compiler
// break point on an if that is
int main(int argc, char* argv[]) {
    if (argc <= 1) {
        perror("Usage; expected at least one argument for the program path\n");
        return 1;
    }
    const char* program= argv[1];

    init_target(ANURA_TARGET);

    action_q= queueb_create();

    pipe(target.target_io_pipe);
    pipe(tui_pipe);

    pthread_t cmd_thread;
    pthread_create(&cmd_thread, NULL, control_thread_create, (void*)program);

    pthread_t tui_thread;
    printf("The stdio pipe before is %d\n", target.target_io_pipe[1]);
    pthread_create(&tui_thread, NULL, tui_thread_create, NULL);

    create_gui(target.target_io_pipe);

    queueb_push_blocking(&action_q, create_action(ACTION_AT_QUIT, (ACTION_DATA) {.NO_DATA = 0}));
    pthread_kill(tui_thread, SIGKILL);

    printf("Waiting for command thread to join\n");

    // here we can assume that the process has died
    pthread_join(cmd_thread, NULL);
    pthread_join(tui_thread, NULL);

    return 0;
}
