//
// Created by james on 30/10/25.
//

#include "Linux-x64.h"

#include "../main.h"
#include "../TargetOS/Linux.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/user.h>

#define SW_INT_TYPE (unsigned char)
#define SW_INT_CODE 0xCC
_Static_assert(SW_INT_TYPE SW_INT_CODE == SW_INT_CODE);

_Static_assert(sizeof SW_INT_TYPE == sizeof ((BPInfo){0}.data.shadow), "The shadow element should encapsulate all data lost from the Software interrupt code");

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

        offset <<= 3; // * 8 to get the number of bits
        char shadow= ((data >> offset) & 0xFFUL);
        data= ((long long)SW_INT_CODE << offset) | (data & ~(0xFFL << offset));

        long long res= ptrace(PTRACE_POKETEXT, t_pid, a_addr, data);
        if (res == -1) return errno;

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

    return res;
}

int linux_x64_init_target(Target* t) {
    linux_init_target(t);

    t->place_bp= place_bp;

    return 0;
}
