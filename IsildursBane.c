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
            if (res) printf("Failed to continue process errno %d of %s\n", errno, strerror(errno));
            else printf("Continued process\n");

            return ACTION_HANDLE_PROC_WAIT;
        }

        case ACTION_BP_ADD: {
            printf("GETTING DAA IN BP ADD\n");
            void* addr= action->data.BP_ADD.addr;
            uint32_t line= action->data.BP_ADD.line;
            printf("GOT DATA\n");

            //action->data.BP_ADD.line
            long long res= target.target_place_bp_at_line(line);
            if (res) printf("Failed to place bp at %d with errno %lld of %s\n", line, res, strerror(res));
            else printf("Placed bp at line %d on addr 0x%p\n", line, addr);

            return ACTION_HANDLE_CONTINUE;
        }

        case ACTION_CF_SINGLE_STEP: {
            bool assembly_level= action->data.CF_SINGLE_STEP.assembly_level;
            printf("Control got single step at %s level\n", assembly_level ? "assembly" : "line");
            if (assembly_level) {
                long long res= target.target_single_step_assembly();

                if (res != 0) {
                    printf("Failed to single step target with err code %lld as %s\n", res, strerror(res));
                    return ACTION_HANDLE_CONTINUE;
                }

                return ACTION_HANDLE_PROC_WAIT;
            } else {
                printf("Got request to single step line; not impl\n");
            }
            break;
        }
        case ACTION_CF_EXIT:
            ptrace(PTRACE_KILL, t_pid, NULL, 0);

            return ACTION_HANDLE_EXIT;
    }
}

void* control_target(void* filepath_p) {
    long res;
    int ret;
    int status;
    char* filepath= filepath_p;

    breakpoint_program(filepath);

    ret = waitpid(t_pid, &status, 0);
    if (!WIFSTOPPED(status)) {
        fprintf(stderr, "Tracee did not stop\n");
    }

    if (status >> 16 == PTRACE_EVENT_EXEC) {
        printf("THIS WAS FROM EXEC\n");
    } else {
        printf("FIRST CALL WAS NOT EXEC\n");
    }

    ptrace(PTRACE_SETOPTIONS, t_pid, 0, PTRACE_O_TRACEEXEC);
    // the child is created but it's just a fork, it's about to run execv after CONT
    ptrace(PTRACE_CONT, t_pid, NULL, 0);

    ret = waitpid(t_pid, &status, 0);
    if (!WIFSTOPPED(status)) {
        fprintf(stderr, "Tracee did not stop\n");
    }

    if (status >> 16 == PTRACE_EVENT_EXEC) {
        printf("THIS WAS FROM EXEC\n");
    } else {
        perror("THIS WAS NOT FROM EXEC");
    }

    target.target_update_after_process_first_stopped();

    void* q_status;
    while (q_status= queueb_pop_blocking(&action_q), q_status) {
        Action* action= q_status;

        switch (handle_action(action)) {
            case ACTION_HANDLE_EXIT:
                goto end;
            case ACTION_HANDLE_PROC_WAIT:
                goto end_q_stat_loop1;
            case ACTION_HANDLE_CONTINUE:
                continue;
        }
    }
end_q_stat_loop1:;


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

        long long r6= ptrace(PTRACE_PEEKUSER, t_pid, offsetof(struct user, u_debugreg[6]));

        long long rip= ptrace(PTRACE_PEEKUSER, t_pid, offsetof(struct user, regs.rip));
        if (!(r6 & 0b1111)) {
            // if we're not hardware i.e. software then we are one ahead
            rip--;
        }

        BPAddressInfo* bp= BPAddressInfo_arr_search_ie(&bp_info, (void*)rip);
        if (!bp || bp->bps.pos == 0) {
            hlog("The tracee stopped via a non breakpoint event with rip= %p\n", (void*)rip);
        } else if (bp->bps.pos == 1) {
            BPInfo* info= BPInfo_arr_ptr(&bp->bps, 0);
            hlog("The tracee stopped via %s breakpoint at %p which is in line %u\n", BP_TYPE_STRS[info->type], rip, info->line);
        } else {
            hlog("The tracee stopped via an address with multiple breakpoints created at lines");
            for (int i= 0; i < bp->bps.pos; ++i) printf(" %u%c", BPInfo_arr_ptr(&bp->bps, i)->line, i == bp->bps.pos - 1 ? ' ' : ',');
            printf("\n");
        }

        target.target_breakpoint_hit_cleanup();

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
        end_q_stat_loop:;
        }
    }
end:

    return 0;
}
