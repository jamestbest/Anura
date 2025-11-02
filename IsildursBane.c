//
// Created by james on 30/10/25.
//

#include "IsildursBane.h"

#include "main.h"
#include "Saruman.h"
#include "Target.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>

uintptr_t base;

typedef enum ACTION_HANDLE_RES {
    ACTION_HANDLE_EXIT,
    ACTION_HANDLE_PROC_WAIT,
    ACTION_HANDLE_CONTINUE
} ACTION_HANDLE_RES;

ACTION_HANDLE_RES handle_action(Action* action) {
    switch (action->type) {
        case ACTION_CF_CONTINUE: {
            printf("Continuing process\n");
            long long res= ptrace(PTRACE_CONT, t_pid, NULL, 0);
            if (res) printf("Failed to continue process errno %lld of %s\n", res, strerror(res));
            else printf("Continued process\n");

            return ACTION_HANDLE_PROC_WAIT;
        }

        case ACTION_BP_ADD: {
            printf("GETTING DAA IN BP ADD\n");
            void* addr= action->data.BP_ADD.addr;
            uint32_t line= action->data.BP_ADD.line;
            printf("GOT DATA\n");

            long long res= target.place_bp(addr + base);
            if (res) printf("Failed to place bp at %d with errno %lld of %s\n", line, res, strerror(res));
            else printf("Placed bp at line %d on addr 0x%p\n", line, addr);

            return ACTION_HANDLE_CONTINUE;
        }
        case ACTION_CF_EXIT:
            ptrace(PTRACE_KILL, t_pid, NULL, 0);

            return ACTION_HANDLE_EXIT;
    }
}

void* control_target(void* a) {
    long res;
    int ret;
    int status;

    breakpoint_program("/home/james/UoNDocs/Anura/cmake-build-debug/test");

    ret = waitpid(t_pid, &status, 0);
    if (!WIFSTOPPED(status)) {
        fprintf(stderr, "Tracee did not stop\n");
    }

    if (status >> 16 == PTRACE_EVENT_EXEC) {
        printf("THIS WAS FROM EXEC\n");
    }


    ptrace(PTRACE_CONT, t_pid, NULL, 0);

    // have to find the runtime address; for now just a quick fetch from the /proc/<pid>/maps file
    char buff[100];
    sprintf(buff, "/proc/%lu/maps", t_pid);
    FILE* f= fopen(buff, "r");

    if (!f) perror("Unable to open /proc/<pid>/maps");

    char fbuff[500];
    fread(fbuff, sizeof(char), sizeof(fbuff), f);
    printf("The buffer:\n%s\n", fbuff);
    sscanf(fbuff, "%lx", &base);

    printf("The base is %lx and the tpid is %lu\n", base, t_pid);

    while (true) {
        printf("Starting to wait\n");
        ret = waitpid(t_pid, &status, __WALL);
        if (ret == -1) {
            perror("waitpid");
            break;
        }
        printf("!!Stopped having to wait\n");

        if (WIFEXITED(status)) {
            hlog("Target exited with %d\n", WEXITSTATUS(status));
            break;
        }

        // res= ptrace(PTRACE_PEEKDATA, t_pid, &i, 0);
        // long value = (int)(res & 0xffffffffUL);
        // hlog("i is : %d\n", value);
        // printf("err: %ld as %s with t_pid %lu\n", res, strerror(errno), t_pid);

        long long r6= ptrace(PTRACE_PEEKUSER, t_pid, offsetof(struct user, u_debugreg[6]));

        long long rip= ptrace(PTRACE_PEEKUSER, t_pid, offsetof(struct user, regs.rip));
        if (!(r6 & 0b1111)) {
            // if we're not hardware i.e. software then we are one ahead
            rip--;
        }
        hlog("The tracee stopped via breakpoint at %p which is in line %u\n", rip, addr2line(rip - base).line);

        res= ptrace(PTRACE_PEEKUSER, t_pid, offsetof(struct user, u_debugreg[7]));
        // print_dr7(res);
        res= ptrace(PTRACE_PEEKUSER, t_pid, offsetof(struct user, u_debugreg[6]));
        // print_dr6(res);

        // BPAddressInfo* addr_info= BPAddressInfo_arr_search_ie(&bp_info, (void*)rip);
        // if (!addr_info) {
            // printf("The target was stopped at a non-breakpoint location %p\n", (void*)rip);
        // } else {
            // BPInfo* bp= BPInfo_arr_ptr(&addr_info->bps, 0);
            // hlog("Bp triggered is a %s breakpoint at address %p (%u)\n", bp->type == BP_SOFTWARE ? "SOFTWARE" : "HARDWARE", (void*)rip, addr2line(rip - base).line);
        // }

        // clear R6 for the next
        r6= 1 << 16 | 1 << 11; // Enable RTM & BLD (19-4 Vol. 3B)
        ptrace(PTRACE_POKEUSER, t_pid, offsetof(struct user, u_debugreg[6]), r6);


        if (WIFSTOPPED(status)) {
            hlog("Target stopped by signal %d\n", WSTOPSIG(status));

            void* q_status;
            while (q_status= queueb_pop_blocking(&action_q), q_status) {
                Action* action= q_status;

                switch (handle_action(action)) {
                    case ACTION_HANDLE_EXIT:
                        goto end;
                    case ACTION_HANDLE_PROC_WAIT:
                        goto end_q_stat_loop;
                    case ACTION_HANDLE_CONTINUE:
                        continue;
                }
            }
        end_q_stat_loop:
        }
    }
end:

    return 0;
}
