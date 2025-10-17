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

void hlog(const char* message, ...);

void print_bp_status(long long r7, unsigned int reg, const char* prefix) {
    unsigned int offset= reg << 1;
    bool l_active= (r7 >> offset) & 1;
    bool g_active= (r7 >> (offset + 1)) & 1;

    offset <<= 1;
    unsigned int rw = ((r7 >> 16) >> offset) & 0b11;
    unsigned int len= ((r7 >> 18) >> offset) & 0b11;

    const char* rw_map[4]= {
            [0b00]= "BRK INST",
            [0b01]= "BRK WRITES",
            [0b10]= "BRK IO",
            [0b11]= "BRK RW NO INST FETCH"
    };
    const unsigned int len_map[4]= {
            [0b00]= 1,
            [0b01]= 2,
            [0b10]= 8,
            [0b11]= 4
    };

    printf(
        "%sDR%u: Local --%c-- Global --%c-- %s %u byte(s)\n",
        prefix,
        reg,
        l_active ? 'X' : ' ',
        g_active ? 'X' : ' ',
        rw_map[rw],
        len_map[len]
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
    BP_SOFTWARE
} BP_TYPE;

#define SW_INT_TYPE (unsigned char)
#define SW_INT_CODE 0xCC
_Static_assert(SW_INT_TYPE SW_INT_CODE == SW_INT_CODE);

typedef struct BPInfo {
    BP_TYPE type;
    void* address;

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
ARRAY_ADD_CMP(BPInfo, BPInfo, cmp_addr, address)

BPInfoArray bps;

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

        BPInfo_arr_add(
            &bps,
            (BPInfo){
                .type= BP_SOFTWARE,
                .address= address,
                .data.shadow= shadow
            }
        );

        return 0;
    }

    r7 |= 1 << (free_bp << 1); // enable LOCAL BREAKPOINT free_bp
    r7 &= (~(0b11 << (16 + (free_bp << 2)))); // set R/Wx to 00 I.e. BRK INST
    r7 &= (~(0b11 << (18 + (free_bp << 2)))); // set LENx to 00 FOLLOWING R/W0 being 00 (Vol. 3B 19-5)

    long long res= 0;
    res= ptrace(PTRACE_POKEUSER, t_pid, offsetof(struct user, u_debugreg[7]), r7);
    if (res != 0) return res;

    res= ptrace(PTRACE_POKEUSER, t_pid, offsetof(struct user, u_debugreg[free_bp]), (long long)address);

    BPInfo_arr_add(
        &bps,
        (BPInfo) {
            .type= BP_HARDWARE,
            .address= address,
            .data.bp= free_bp
        }
    );

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

    bps= BPInfo_arr_construct(5);

    pid_t pid= fork();

    if (pid == 0) {
        target(false);
        return 0;
    }

    t_pid= pid;

    long res= ptrace(PTRACE_ATTACH, pid, 0, 0);
    hlog("The result is %ld errno is %d with error %s\n", res, errno, strerror(errno));

    int status;
    int ret = waitpid(pid, &status, 0);
    hlog("waitpid returned %d, status: 0x%x\n", ret, status);

    if (!WIFSTOPPED(status)) {
        perror("target did not stop as expected\n");
    }

    res= ptrace(PTRACE_PEEKUSER, pid, offsetof(struct user, u_debugreg[7]));
    print_dr7(res);
    print_dr_status(res);
    res= ptrace(PTRACE_PEEKUSER, pid, offsetof(struct user, u_debugreg[6]));
    print_dr6(res);

    struct user regs;
    res= ptrace(PTRACE_GETREGS, pid, &regs, &regs);
    hlog("Result of getting registers is %d\n", res);

//    place_bp(t5);
//    place_bp(t4);
//    place_bp(t3);
//    place_bp(t2);
//    place_bp(target);

    char buff[40];
    struct iovec l;
    l.iov_base= buff;
    l.iov_len= sizeof (buff);
    struct iovec r;
    r.iov_base= (void*)t2;
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


    res= ptrace(PTRACE_PEEKUSER, pid, offsetof(struct user, u_debugreg[7]));
    print_dr7(res);
    res= ptrace(PTRACE_PEEKUSER, pid, offsetof(struct user, u_debugreg[6]));
    print_dr6(res);
    res= ptrace(PTRACE_PEEKUSER, pid, offsetof(struct user, u_debugreg[0]));
    hlog("DR0 is : %lld\n", res);

    res= ptrace(PTRACE_PEEKDATA, pid, &i, 0);
    int value = (int)(res & 0xffffffffUL);

    hlog("i is : %d\n", value);

//    res= ptrace(PTRACE_CONT, pid, NULL, 0);
//    hlog("The result is %ld errno is %d with error %s\n", res, errno, strerror(errno));

    return 0;
}

int main(void) {
    breakpoint_program();

    FILE* elf= fopen("Anura", "r");
    decode(elf);

    // have to find the runtime address; for now just a quick fetch from the /proc/<pid>/maps file
    char buff[100];
    sprintf(buff, "/proc/%d/maps", t_pid);
    FILE* f= fopen(buff, "r");

    if (!f) perror("Unable to open /proc/<pid>/maps");

    char fbuff[500];
    fread(fbuff, sizeof(char), sizeof(fbuff), f);
    sscanf(fbuff, "%lx", &base);

    printf("The base is %lx\n", base);

//    ptrace(PTRACE_CONT, t_pid, NULL, 0);

    while (true) {
        printf("cmd: ");
        char buff[100];
        int line= -1;

        if (!fgets(buff, sizeof(buff), stdin)) break;

        sscanf(buff, "set %d", &line);
        if (line == -1) {
            printf("Ending loop\n");
            break;
        }
        uint64_t addr= line2startaddr(line);
        if (addr == -1) {
            printf("There is no code on line %d\n", line);
            continue;
        }
        long long res= place_bp((void*)addr + base);

        if (res) printf("Failed to place bp at %d\n", line);
        else printf("Placed bp at line %d on addr 0x%p\n", line, (void*)addr);
    }



    long res;
    int ret;
    int status;

    ptrace(PTRACE_CONT, t_pid, NULL, 0);

    while (true) {
        ret = waitpid(t_pid, &status, 0);
        if (ret == -1) {
            perror("waitpid");
            break;
        }
        if (WIFSTOPPED(status)) {
            hlog("Target stopped by signal %d\n", WSTOPSIG(status));

            res= ptrace(PTRACE_PEEKDATA, t_pid, &i, 0);
            long value = (int)(res & 0xffffffffUL);
            hlog("i is : %d\n", value);

            long long r6= ptrace(PTRACE_PEEKUSER, t_pid, offsetof(struct user, u_debugreg[6]));

            long long rip= ptrace(PTRACE_PEEKUSER, t_pid, offsetof(struct user, regs.rip));
            if (!(r6 & 0b1111)) {
                // if we're not hardware i.e. software then we are one ahead
                rip--;
            }
            hlog("The tracee stopped via breakpoint at %p which is in line %u\n", rip, addr2line(rip - base).line);

            char buff[40];
            struct iovec l;
            l.iov_base= buff;
            l.iov_len= sizeof (buff);
            struct iovec r;
            r.iov_base= (void*)rip;
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

            res= ptrace(PTRACE_PEEKUSER, t_pid, offsetof(struct user, u_debugreg[7]));
            print_dr7(res);
            res= ptrace(PTRACE_PEEKUSER, t_pid, offsetof(struct user, u_debugreg[6]));
            print_dr6(res);
//            res= ptrace(PTRACE_PEEKUSER, t_pid, offsetof(struct user, u_debugreg[0]));
//            hlog("DR0 is : %lld\n", res);
//            res= ptrace(PTRACE_PEEKUSER, t_pid, offsetof(struct user, u_debugreg[1]));
//            hlog("DR1 is : %lld\n", res);
//            res= ptrace(PTRACE_PEEKUSER, t_pid, offsetof(struct user, u_debugreg[2]));
//            hlog("DR2 is : %lld\n", res);
//            res= ptrace(PTRACE_PEEKUSER, t_pid, offsetof(struct user, u_debugreg[3]));
//            hlog("DR3 is : %lld\n", res);

            BPInfo* bp= BPInfo_arr_search_ie(&bps, (void*)rip);
            hlog("Bp triggered is a %s breakpoint at address %p (%u)\n", bp->type == BP_SOFTWARE ? "SOFTWARE" : "HARDWARE", (void*)rip, addr2line(rip - base).line);

            // clear R6 for the next
            r6= 1 << 16 | 1 << 11; // Enable RTM & BLD (19-4 Vol. 3B)
            ptrace(PTRACE_POKEUSER, t_pid, offsetof(struct user, u_debugreg[6]), r6);

            ptrace(PTRACE_CONT, t_pid, NULL, 0);
        } else if (WIFEXITED(status)) {
            hlog("Target exited with %d\n", WEXITSTATUS(status));
            break;
        }
    }


//    return 0;


    return 0;
}
