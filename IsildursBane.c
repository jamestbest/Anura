//
// Created by james on 30/10/25.
//

#include "IsildursBane.h"

#include "main.h"
#include "Target.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>

#include "Saruman/Saruman.h"

uintptr_t base;

typedef enum ACTION_HANDLE_RES {
    ACTION_HANDLE_EXIT,
    ACTION_HANDLE_PROC_WAIT,
    ACTION_HANDLE_CONTINUE
} ACTION_HANDLE_RES;

typedef enum CONTROL_STATE {
    STATE_NORMAL,
    STATE_STEP_INTO, // we handle SIG TRAPS and send single step until
    STATE_STEP_OVER,
} CONTROL_STATE;

CONTROL_STATE state= STATE_NORMAL;
uint64_t step_into_line= 0;

typedef struct StepOverInfo {
    uint64_t line;
    uint64_t cfa_value;
    uintptr_t ip_value;
} StepOverInfo;
StepOverInfo step_over_info;

ACTION_HANDLE_RES handle_cf_single_step() {
    const long long res= target.target_single_step_assembly();

    if (res != 0) {
        printf("Failed to single step target with err code %lld as %s\n", res, strerror(res));
        return ACTION_HANDLE_CONTINUE;
    }

    return ACTION_HANDLE_PROC_WAIT;
}

ACTION_HANDLE_RES handle_cf_continue() {
    printf("Continuing process\n");
    const long res= target.target_cf_continue();
    if (res) printf("Failed to continue process errno %d of %s\n", errno, strerror(errno));
    else printf("Continued process\n");

    return ACTION_HANDLE_PROC_WAIT;
}

ACTION_HANDLE_RES handle_step_over() {
    /*  There is quite a lot to stepping over a line
     *   the basic idea is that we want to stop when we're on a new line of this function
     *   but it must be the same instance (i.e. on the same rbp (*))
     *   we might also leave to the caller (in which case we stop immediately)
     *   we might leave to another called function (in which case we want to skip all this)
     *   we might be in a tail call optimisation
     */

    // we have just executed a single step trap

    const uintptr_t c_ip= target.target_get_pc();
    const uint64_t c_cfa= target.target_get_cfa();

    const uintptr_t c_vaddr= target.target_addr_runtime_to_virtual(c_ip);
    const AddrLineRes line= addr2line(c_vaddr);

    if (!line.succ) {
        printf("Unable to get line information for ip address %#lx\n", c_ip);
        return ACTION_HANDLE_CONTINUE;
    }

    const bool on_same_line= line.line == step_over_info.line;
    const bool in_same_function_instance= c_cfa == step_over_info.cfa_value;

    if (on_same_line && in_same_function_instance) {
        // here we're still on the same line
        return handle_cf_single_step();
    }
    if (!on_same_line && in_same_function_instance) {
        // this is the easy condition for step-over because we've reached a new line in the same instance of a function
        //  so just inform the gui that we've stopped wait
        // todo inform gui
        printf("Reached newline from step over, original line: %lu, new line %lu\n", step_over_info.line, line.line);
        return ACTION_HANDLE_CONTINUE;
    }

    if (!in_same_function_instance) {
        // here there are some choices
        // the simple one is that we've called another function and so the stack has grown
        if (c_cfa < step_over_info.cfa_value) {
            uintptr_t return_address= target.target_get_return_addr();
            target.target_place_temp_bp(return_address, BP_REASON_STEP_OVER);
            return ACTION_HANDLE_PROC_WAIT;
        }
        if (c_cfa >= step_over_info.cfa_value) {
            // here there are two options
            //  either this is from a return (in which case we just stop)
            //  or it's from a tail call optimisation and we're in the next call, unfortunately we must also stop here
            //   as this may also be a tail call
            printf("Reached caller from step over\n");
            return ACTION_HANDLE_CONTINUE;
        }
    }
}

ACTION_HANDLE_RES handle_step_into_step() {
    printf("Handling step into");

    const uintptr_t c_addr= target.target_get_pc();
    const uintptr_t v_addr= (uintptr_t)target.target_addr_runtime_to_virtual((void*)c_addr);

    const AddrLineRes res= addr2line(v_addr);
    if (!res.succ) {
        printf("Failed to resolve line in step into\n");
        // here we have no line information so we assume that we've reached a new location
        state= STATE_NORMAL;
        return handle_cf_continue();
    }

    if (step_into_line != res.line) {
        state= STATE_NORMAL;
        printf("Reached new line %lu\n", res.line);
        return ACTION_HANDLE_CONTINUE;
    }

    return handle_cf_single_step();
}

ACTION_HANDLE_RES handle_action(Action* action) {
    switch (action->type) {
        case ACTION_CF_CONTINUE: {
            return handle_cf_continue();
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

        case ACTION_BP_REMOVE: {
            printf("Removing breakpoint\n");
            uint32_t line= action->data.BP_REMOVE.line;

            long long res= target.target_remove_bp_at_line(line);
            if (res) printf("Failed to place bp at %d with errno %lld of %s\n", line, res, strerror(res));
            else printf("Removed bp at line %d\n", line);

            return ACTION_HANDLE_CONTINUE;
        }

        case ACTION_CF_SINGLE_STEP: {
            bool assembly_level= action->data.CF_SINGLE_STEP.assembly_level;
            printf("Control got single step at %s level\n", assembly_level ? "assembly" : "line");
            if (assembly_level) {
                return handle_cf_single_step();
            } else {
                printf("Got request to single step line; not impl\n");
            }
            break;
        }
        case ACTION_CF_STEP_INTO: {
            const uintptr_t c_addr= target.target_get_pc();
            const uintptr_t v_addr= (uintptr_t)target.target_addr_runtime_to_virtual((void*)c_addr);
            printf("Current addr: %#lX\n", c_addr);
            printf("Current virtual addr: %#lX\n", v_addr);
            const AddrLineRes line= addr2line(v_addr);
            if (!line.succ) {
                perror("Unable to find line associated with current program counter to step into");
                return ACTION_HANDLE_CONTINUE;
            }
            step_into_line= line.line;
            state= STATE_STEP_INTO;
            target.target_single_step_assembly();

            return ACTION_HANDLE_PROC_WAIT;
        }
        case ACTION_CF_STEP_OVER: {
            const uintptr_t c_addr= target.target_get_pc();
            const uintptr_t v_addr= (uintptr_t)target.target_addr_runtime_to_virtual((void*)c_addr);
            printf("Current addr: %#lX\n", c_addr);
            printf("Current virtual addr: %#lX\n", v_addr);
            const AddrLineRes line= addr2line(v_addr);
            if (!line.succ) {
                perror("Unable to find line associated with current program counter to step over");
                return ACTION_HANDLE_PROC_WAIT;
            }
            step_over_info.line= line.line;
            step_over_info.ip_value= v_addr;
            step_over_info.cfa_value= target.target_get_cfa();
            state= STATE_STEP_OVER;
            target.target_single_step_assembly();

            return ACTION_HANDLE_PROC_WAIT;
        }
        case ACTION_CF_EXIT:
            ptrace(PTRACE_KILL, t_pid, NULL, 0);

            return ACTION_HANDLE_EXIT;
    }
}

// these don't seem to be exposed else where?
#define TRAP_BRKPT 1
#define TRAP_TRACE 2
#define TRAP_HWBKPT 3

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

        const BPAddressInfo* bp= BPAddressInfo_arr_search_ie(&bp_info, (void*)rip);
        if (!bp || bp->user_bp_count == 0) {
            hlog("The tracee stopped via a non breakpoint event with rip= %p\n", (void*)rip);
        } else {
            hlog("The tracee stopped via %s breakpoint at %p which is in line %u, there are %u breakpoints at this location\n",
                BP_TYPE_STRS[bp->canonical_bp.type],
                rip,
                bp->canonical_bp.line,
                bp->user_bp_count
            );
        }

        target.target_breakpoint_hit_cleanup();

        if (WIFSTOPPED(status)) {
            hlog("Target stopped by signal %d\n", WSTOPSIG(status));

            int signal= WSTOPSIG(status);
            if (signal == SIGTRAP) {
                siginfo_t siginfo;
                ptrace(PTRACE_GETSIGINFO, target.pid, 0, &siginfo);
                switch (siginfo.si_code) {
                    case TRAP_TRACE: {
                        switch (state) {
                            case STATE_STEP_INTO: {
                                ACTION_HANDLE_RES res= handle_step_into_step();
                                switch (res) {
                                    case ACTION_HANDLE_PROC_WAIT:
                                        goto end_q_stat_loop;
                                    case ACTION_HANDLE_CONTINUE:
                                        break;
                                }
                                break;
                            }
                            case STATE_STEP_OVER: {
                                const ACTION_HANDLE_RES res= handle_step_over();
                                switch (res) {
                                    case ACTION_HANDLE_PROC_WAIT:
                                        goto end_q_stat_loop;
                                    case ACTION_HANDLE_CONTINUE:
                                        break;
                                }
                                break;
                            }
                            case STATE_NORMAL: {
                                printf("Recieved trap in normal state\n");
                                break;
                            }
                        }

                    }
                }
            }

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
