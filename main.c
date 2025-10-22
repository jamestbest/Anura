#include <stdio.h>
#include <sys/ptrace.h>
#include <errno.h>
#include <string.h>

#include <sys/user.h>
#include <elf.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdarg.h>
#include <stddef.h>
#include <assert.h>
#include "Array.h"
#include "Sauron.h"
#include "Saruman.h"

int i=0;
uintptr_t base;

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

typedef enum BP_TYPE {
    BP_HARDWARE,
    BP_SOFTWARE,

    BP_SOURCE_SINGLE_STEP_TRAP,

    BP_TYPE_COUNT
} BP_TYPE;

const char* const BP_TYPE_STRS[BP_TYPE_COUNT]= {
    [BP_HARDWARE]= "HARDWARE",
    [BP_SOFTWARE]= "SOFTWARE",
    [BP_SOURCE_SINGLE_STEP_TRAP]= "SINGLE STEP TRAP"
};

int BP_type_is_user(BP_TYPE type) {
    return type == BP_HARDWARE || type == BP_SOFTWARE;
}

#define SW_INT_TYPE (unsigned char)
#define SW_INT_CODE 0xCC
_Static_assert(SW_INT_TYPE SW_INT_CODE == SW_INT_CODE);

typedef struct BPInfo {
    BP_TYPE type;

    union {
        // we could just save the contents of the word, but there may be another bp or code having been changed in between,
        //  so we'll just store the single byte. This won't help if something else overwrites the underlying value
        //  (this requires the bp to be placed, value overwritten and then bp removed)
        unsigned char shadow;
        unsigned char bp;
    } data;
} BPInfo;
_Static_assert(sizeof SW_INT_TYPE == sizeof ((BPInfo){0}.data.shadow), "The shadow element should encapsulate all data lost from the Software interrupt code");

int cmp_addr(void* bpa, void* bpb) {
    return bpa - bpb;
}

ARRAY_PROTO(BPInfo, BPInfo)
ARRAY_ADD(BPInfo, BPInfo)

//BPInfoArray bps;

typedef struct BPAddressInfo {
    void* address;

    BPInfoArray bps;
} BPAddressInfo;

ARRAY_PROTO(BPAddressInfo, BPAddressInfo)
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

pid_t t_pid;
long long place_bp(void* address) {
    long long r7= ptrace(PTRACE_PEEKUSER, t_pid, offsetof(struct user, u_debugreg[7]));
    
    bool bp_used[4]= {0};
    unsigned int free_bp= -1;
    for (int j = 0; j < 4; ++j) {
        bool used= ((r7 >> (j << 1)) & 0b11);
        bp_used[j]= used;
        if (!used && (free_bp == (unsigned int)-1)) free_bp= j;
    }
    // if there is a local or global breakpoint enabled for all bps then we cannot create a hardware breakpoint
    if (free_bp == -1) {
        // we want to read just 1 byte of data for the shadow, but this might not be an aligned read
        // so we'll find the nearest 8 aligned boundry which includes the address and then shift out the rest
        //  save this alignment offset for later writing the shadow back
        void* a_addr= (void*)((long long)address & ~0b111);
        unsigned int offset= address - a_addr;

        errno= 0;
        long long data= ptrace(PTRACE_PEEKDATA, t_pid, a_addr, a_addr);

        if (data == -1 && errno) {
            return errno;
        }

        hlog("The values at TARGET FUNCTION is: ");
        unsigned char* v= (unsigned char*)&data;
        for (int j = 0; j < sizeof (data); ++j) {
            unsigned char va= v[j];
            printf("%02X", va);
        }
        printf("\n");
        fflush(stdout);

        offset <<= 3; // * 8 to get the number of bits
        char shadow= ((data >> offset) & 0xFFUL);
        data= ((long long)SW_INT_CODE << offset) | (data & ~(0xFFL << offset));

        long long res= ptrace(PTRACE_POKETEXT, t_pid, a_addr, data);
        if (res == -1) return errno;

        hlog("The values at TARGET FUNCTION is: ");
        v= (unsigned char*)&data;
        for (int j = 0; j < sizeof (data); ++j) {
            unsigned char va= v[j];
            printf("%02X", va);
        }
        printf("\n");
        fflush(stdout);

        hlog(
            "Created SOFTWARE bp at %p (aligned: %p) with interrupt %#x and shadow %#x\n",
            address,
            a_addr,
            (int)SW_INT_CODE,
            (int)shadow
        );

        BPAddressInfo* addr_info= get_or_add_bp_address_info(address);
        BPInfo_arr_add(&addr_info->bps, (BPInfo){
            .type= BP_SOFTWARE,
            .data.shadow= shadow
        });

        return 0;
    }

    r7 |= 1 << (free_bp << 1); // enable LOCAL BREAKPOINT free_bp
    r7 &= (~(0b11 << (16 + (free_bp << 2)))); // set R/Wx to 00 I.e. BRK INST
    r7 &= (~(0b11 << (18 + (free_bp << 2)))); // set LENx to 00 FOLLOWING R/W0 being 00 (Vol. 3B 19-5)

    long long res= 0;
    res= ptrace(PTRACE_POKEUSER, t_pid, offsetof(struct user, u_debugreg[7]), r7);
    if (res != 0) return res;

    res= ptrace(PTRACE_POKEUSER, t_pid, offsetof(struct user, u_debugreg[free_bp]), (long long)address);

    BPAddressInfo* addr_info= get_or_add_bp_address_info(address);
    BPInfo_arr_add(&addr_info->bps, (BPInfo) {
        .type= BP_HARDWARE,
        .data.bp= free_bp
    });

    hlog("Created HARDWARE bp %d at address %p\n", free_bp, address);

    return res;
}

void vlog(bool is_t, const char* message, va_list args) {
    printf("LOG(%s): ", is_t ? "TARGET" : " HOST ");
    vprintf(message, args);

    fflush(stdout);
}

void log(bool is_t, const char* message, ...) {
    va_list args;
    va_start(args, message);
    vlog(is_t, message, args);
    va_end(args);
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

void t2() {
    tlog("I am in T2\n");
}

void t3() {
    tlog("I am in T3\n");
    t2();
}

void t4() {
    tlog("I am in T4\n");
    t3();
    t2();
}

void t5() {
    tlog("I am in T5\n");
    t4();
    t3();
    t2();
}

int target(bool second) {
    tlog("PID: %d\n", getpid());
    tlog("I is at %p\n", &i);

    i=0;

    while (i < 3) {
        i++;
        tlog("I is: %d\n", i);
        sleep(1);
    }

//    asm (
//            "int3"
//    );

//    tlog("Target starting again after int3\n");

    t5();

    if (second) while (true);
    else target(true);

    return 0;
}

#define INT3 (0xCC)
#include <sys/uio.h>
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

int breakpoint_program() {
    hlog("Hello, World!\n");

    bp_info= BPAddressInfo_arr_construct(4);

    pid_t pid= fork();
//    execv()

    if (pid == 0) {
        raise(SIGSTOP);
        target(false);
        return 0;
    }

    t_pid= pid;

    printf("Set the t_pid to %d\n", pid);

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

int read_test(void* loc) {
    char buff[40];
    struct iovec l;
    l.iov_base= buff;
    l.iov_len= sizeof (buff);
    struct iovec r;
    r.iov_base= (void*)loc;
    r.iov_len= sizeof (buff);
    ssize_t read= process_vm_readv(t_pid, &l, 1, &r, 1, 0);
    if (read != sizeof (buff)) {
        perror("Cannot read vm readv\n");
    }
    hlog("Read %zu byte from target\n", read);
    for (int j = 0; j < sizeof (buff); ++j) {
        printf("%02hhX ", buff[j]);
    }
    putchar('\n');

    return 0;
}

#define ACTION_CONTINUE 1
#define ACTION_BREAKPOINT_SET 2
int action= 0;
void* bp_pos= 0;
int bp_line= 0;

void* control_target(void* a) {
    long res;
    int ret;
    int status;

    // have to find the runtime address; for now just a quick fetch from the /proc/<pid>/maps file
    char buff[100];
    sprintf(buff, "/proc/%d/maps", t_pid);
    FILE* f= fopen(buff, "r");

    if (!f) perror("Unable to open /proc/<pid>/maps");

    char fbuff[500];
    fread(fbuff, sizeof(char), sizeof(fbuff), f);
    sscanf(fbuff, "%lx", &base);

    printf("The base is %lx\n", base);

    res= ptrace(PTRACE_ATTACH, t_pid, 0, 0);
    hlog("The result is %ld errno is %d with error %s\n", res, errno, strerror(errno));

    ret = waitpid(t_pid, &status, 0);
    if (!WIFSTOPPED(status)) {
        fprintf(stderr, "Tracee did not stop\n");
    }

    ptrace(PTRACE_CONT, t_pid, NULL, 0);

    while (true) {
        ret = waitpid(t_pid, &status, __WALL);
        if (ret == -1) {
            perror("waitpid");
            break;
        }

        res= ptrace(PTRACE_PEEKDATA, t_pid, &i, 0);
        long value = (int)(res & 0xffffffffUL);
        hlog("i is : %d\n", value);
        printf("err: %ld as %s with t_pid %d\n", res, strerror(errno), t_pid);

        long long r6= ptrace(PTRACE_PEEKUSER, t_pid, offsetof(struct user, u_debugreg[6]));

        long long rip= ptrace(PTRACE_PEEKUSER, t_pid, offsetof(struct user, regs.rip));
        if (!(r6 & 0b1111)) {
            // if we're not hardware i.e. software then we are one ahead
            rip--;
        }
        hlog("The tracee stopped via breakpoint at %p which is in line %u\n", rip, addr2line(rip - base).line);

        res= ptrace(PTRACE_PEEKUSER, t_pid, offsetof(struct user, u_debugreg[7]));
        print_dr7(res);
        res= ptrace(PTRACE_PEEKUSER, t_pid, offsetof(struct user, u_debugreg[6]));
        print_dr6(res);

        BPAddressInfo* addr_info= BPAddressInfo_arr_search_ie(&bp_info, (void*)rip);
        if (!addr_info) {
            printf("The target was stopped at a non-breakpoint location %p\n", (void*)rip);
        } else {
            BPInfo* bp= BPInfo_arr_ptr(&addr_info->bps, 0);
            hlog("Bp triggered is a %s breakpoint at address %p (%u)\n", bp->type == BP_SOFTWARE ? "SOFTWARE" : "HARDWARE", (void*)rip, addr2line(rip - base).line);
        }

        // clear R6 for the next
        r6= 1 << 16 | 1 << 11; // Enable RTM & BLD (19-4 Vol. 3B)
        ptrace(PTRACE_POKEUSER, t_pid, offsetof(struct user, u_debugreg[6]), r6);


        if (WIFSTOPPED(status)) {
            hlog("Target stopped by signal %d\n", WSTOPSIG(status));

            int c=0;
            while (true) {
                if (c++ == 0) {
                    printf("Still waiting\n");
                }
                if (action == ACTION_CONTINUE) {
                    ptrace(PTRACE_CONT, t_pid, NULL, 0);
                    action= 0;
                    break;
                }
                if (action == ACTION_BREAKPOINT_SET) {
                    long long res= place_bp((void*)bp_pos + base);
                    if (res) printf("Failed to place bp at %d with errno %lld of %s\n", bp_line, res, strerror(res));
                    else printf("Placed bp at line %d on addr 0x%p\n", bp_line, (void*)bp_pos);
                    action= 0;
                    continue;
                }
            }


//            if (WSTOPSIG(status) == SIGSTOP) {
//                ptrace(PTRACE_CONT, t_pid, NULL, 0);
//                continue;
//            }

        } else if (WIFEXITED(status)) {
            hlog("Target exited with %d\n", WEXITSTATUS(status));
            break;
        }
    }

    return 0;
}

/*                 UI HANDLING
 * There are ui actions that we want to allow
 *    E.g. set breakpoint, read value, show stack trace etc.
 * These require in linux the process being in a PTRACE_stopped mode
 *
 * So
 *  UI trigger_bp_set -> trigger_stop -> CTRL CATCH SIGSTOP -> CTRL bp_set -> CTRL RESUME
 * The UI thread may need to trigger some kind of stop for the OS target,
 *  and so there is a trigger_bp_set which does this
 *  it also queues an action of bp_set
 * The CTRL thread catches this SIGSTOP, it will check that it was internal (storing generative SIGSTOPs)
 * The CTRL thread can then do any/all the actions in the queue that are available, one will be the bp_set
 * The CTRL thread can then CONT the execution
 *
 * The action queue should be multithreaded safe
 * Same for the SIGSTOP generative queue
 * Just pop the top SIGSTOP generator off when verifying, we can't know which of the signals is external, just that
 *  ones at the end are extra
 * INTERNAL_SIGSTOP -> INTERNAL_SIGSTOP -> EXTERNAL_SIGSTOP -> INTERNAL_SIGSTOP
 *  queue: SIGSTOP, SIGSTOP, SIGSTOP
 * handling:
 *  receive stop, pop queue -> receive stop, pop queue -> receive stop, pop queue -> receive stop, NOTHING TO POP
 * any actions are all delt with at each SIGSTOP, even if bp_add, bp_add placed two SIGSTOPs it shouldn't matter
 * as they'll both be in the generator list.
 */

int main(void) {
    FILE* elf= fopen("Anura", "r");
    decode(elf);

    breakpoint_program();

    pthread_t tui_thread;
    pthread_create(&tui_thread, NULL, control_target, NULL);

    while (true) {
        printf("cmd: ");
        char buff[100];
        int line= -1;

        if (!fgets(buff, sizeof(buff), stdin)) break;

        if (strncmp(buff, "set", sizeof("set") - 1) == 0) {
            sscanf(buff, "set %d", &line);
            uint64_t addr= line2startaddr(line);
            if (addr == -1) {
                printf("There is no code on line %d\n", line);
                continue;
            }
            action= ACTION_BREAKPOINT_SET;
            bp_line= line;
            bp_pos= (void*)addr;
        } else if (strncmp(buff, "cont", sizeof("cont") - 1) == 0) {
            printf("Continuing process\n");
            errno=0;
            action= ACTION_CONTINUE;
        } else {
            printf("Unable to match command `%s`\n", buff);
        }
    }

    print_breakpoints();

    return 0;
}
