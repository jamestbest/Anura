#include "main.h"

#include "Array.h"
#include "IsildursBane.h"
#include "Palantir.h"
#include "QueueB.h"
#include "Saruman.h"
#include "Sauron.h"
#include "Target.h"

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int i=0;

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

int cmp_addr(void* bpa, void* bpb) {
    return bpa - bpb;
}

ARRAY_ADD(BPInfo, BPInfo)
ARRAY_ADD_CMP(BPAddressInfo, BPAddressInfo, cmp_addr, address)

BPAddressInfoArray bp_info;

BPAddressInfo* get_or_add_bp_address_info(void* address) {
    BPAddressInfo* info= BPAddressInfo_arr_search_ie(&bp_info, address);
    if (info) return info;

    BPAddressInfo* addr_info= BPAddressInfo_arr_add_i(&bp_info);
    addr_info->address= address;
    addr_info->bps= BPInfo_arr_construct(1);

    return addr_info;
}


PROCESS_ID t_pid;

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
    vlog(false, message, args);
    va_end(args);
}

#define INT3 (0xCC)
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
    hlog("Hello, World!\n");

    bp_info= BPAddressInfo_arr_construct(4);

    const char* args[]= {
        program,
        "test",
        NULL
    };

    PROCESS_ID pid= target.target_launch_process(program, sizeof(args), args);
    t_pid= pid;

    long long res= target.target_attach_process(t_pid);
    hlog("The attach result is %ld errno is %d with error %s\n", res, errno, strerror(errno));
    printf("Set the t_pid to %llu\n", pid);

    return 0;
}

void print_breakpoints() {
    printf("-----BREAKPOINTS-----\n");
    printf("There are %zu locations with breakpoints\n", bp_info.pos);

    for (int j = 0; j < bp_info.pos; ++j) {
        BPAddressInfo* addr_info= BPAddressInfo_arr_ptr(&bp_info, j);

        printf("  - Addr: %p contains %zu breakpoint(s): \n", addr_info->address, addr_info->bps.pos);

        for (int k = 0; k < addr_info->bps.pos; ++k) {
            BPInfo* bp= BPInfo_arr_ptr(&addr_info->bps, k);

            printf("    + BP of Type %s with data ", BP_TYPE_STRS[bp->type]);

            switch (bp->type) {
                case BP_HARDWARE:
                    printf("BP No. %u", bp->data.bp);
                    break;
                case BP_SOFTWARE:
                    printf("SHADOW 0x%x", bp->data.shadow);
                    break;
                case BP_SOURCE_SINGLE_STEP_TRAP:
                    printf("No data");
                    break;
            }

            printf("\n");
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

int main(int argc, char* argv[]) {
    if (argc <= 1) {
        perror("Usage; expected at least one argument for the program path\n");
        return 1;
    }
    const char* program= argv[1];

    init_target(TARGET_LINUX_X64);

    action_q= queueb_create();

    FILE* elf= fopen(program, "r");
    decode(elf);

    pthread_t cmd_thread;
    pthread_create(&cmd_thread, NULL, control_target, NULL);

    tui();

    printf("Waiting for command thread to join\n");
    // here we can assume that the process has died
    pthread_join(cmd_thread, NULL);

    print_breakpoints();

    return 0;
}
