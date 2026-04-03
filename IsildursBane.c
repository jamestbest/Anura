//
// Created by james on 30/10/25.
//

// this is needed to include the TRAP_* enums
#define _GNU_SOURCE
#include <signal.h>

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

#include "break_on_cause.h"
#include "Palantir/Palantir.h"
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
    STATE_STEP_OUT,
} CONTROL_STATE;

CONTROL_STATE state= STATE_NORMAL;
uint64_t step_into_line= 0;

typedef struct StepOverInfo {
    uint64_t line;
    uint64_t cfa_value;
    uintptr_t ip_value;
    uintptr_t temp_bp_addr;
} StepOverInfo;
StepOverInfo step_over_info;

uintptr_t step_out_addr= -1;

ACTION_HANDLE_RES handle_cf_single_step() {
    const long long res= target.target_cf_single_step_assembly();

    if (res != 0) {
        show_log("Failed to single step target with err code %lld as %s\n", res, strerror(res));
        return ACTION_HANDLE_CONTINUE;
    }

    return ACTION_HANDLE_PROC_WAIT;
}

ACTION_HANDLE_RES handle_cf_continue() {
    show_log("Continuing process\n");
    const long res= target.target_cf_continue();
    if (res) show_log("Failed to continue process errno %d of %s\n", errno, strerror(errno));
    else show_log("Continued process\n");

    return ACTION_HANDLE_PROC_WAIT;
}

void step_over_cleanup() {
    // here we've done or failed to do the actual step over
    //  this could be because we hit a breakpoint midway through that was a user one
    //  or we're on a newline in the function instance
    //  or we failed - usually from being in an extern function w/ no line info

    // cleanup means removing the temp breakpoint & changing the state back
    state= STATE_NORMAL;
    if (step_over_info.temp_bp_addr != -1)
        target.target_remove_bp_at_addr(step_over_info.temp_bp_addr, BP_REASON_STEP_OVER);
    step_over_info.temp_bp_addr= -1;
}

void step_out_cleanup() {
    state= STATE_NORMAL;

    if (step_out_addr == -1) {
        show_log("Attempted to cleanup step out when address is not set\n");
        return;
    }

    target.target_remove_bp_at_addr(step_out_addr, BP_REASON_STEP_OUT);
    step_out_addr= -1;
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

    bool succ;
    const uintptr_t c_ip= target.target_get_pc();
    const uint64_t c_cfa= target.target_get_cfa(&succ);

    const uintptr_t c_vaddr= target.target_addr_runtime_to_virtual(c_ip);
    const AddrLineRes line= addr2line(c_vaddr);

    if (step_over_info.temp_bp_addr != -1)
        target.target_remove_bp_at_addr(step_over_info.temp_bp_addr, BP_REASON_STEP_OVER);
    step_over_info.temp_bp_addr= -1;

    if (!line.succ) {
        show_log("Unable to get line information for ip address %#lx and so cannot step-over source line\n", c_ip);
        step_over_cleanup();
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
        show_log("Reached newline from step over, original line: %lu, new line %lu\n", step_over_info.line, line.line);
        step_over_cleanup();

        g_idle_add(hit_addr, GUINT_TO_POINTER(c_vaddr));
        return ACTION_HANDLE_CONTINUE;
    }

    if (!in_same_function_instance) {
        // here there are some choices
        // the simple one is that we've called another function and so the stack has grown
        if (c_cfa < step_over_info.cfa_value) {
            const uintptr_t return_address= target.target_get_return_addr();
            show_log("The return address gotten is %#lx\n", return_address);
            show_log("The virtual address is %#lx\n", target.target_addr_runtime_to_virtual(return_address));
            target.target_place_temp_bp(return_address, BP_REASON_STEP_OVER);
            step_over_info.temp_bp_addr= return_address;
            target.target_cf_continue();
            return ACTION_HANDLE_PROC_WAIT;
        }
        if (c_cfa >= step_over_info.cfa_value) {
            // here there are two options
            //  either this is from a return (in which case we just stop)
            //  or it's from a tail call optimisation and we're in the next call, unfortunately we must also stop here
            //   as this may also be a tail call
            show_log("Reached caller from step over\n");
            step_over_cleanup();

            g_idle_add(hit_addr, GUINT_TO_POINTER(c_vaddr));
            return ACTION_HANDLE_CONTINUE;
        }
    }

    return ACTION_HANDLE_CONTINUE;
}

ACTION_HANDLE_RES handle_step_into_step() {
    show_log("Handling step into");

    const uintptr_t c_addr= target.target_get_pc();
    const uintptr_t v_addr= target.target_addr_runtime_to_virtual(c_addr);

    const AddrLineRes res= addr2line(v_addr);
    if (!res.succ) {
        show_log("Failed to resolve line in step into\n");
        // here we have no line information so we assume that we've reached a new location
        state= STATE_NORMAL;
        return handle_cf_continue();
    }

    if (step_into_line != res.line) {
        state= STATE_NORMAL;
        show_log("Reached new line %lu\n", res.line);

        g_idle_add(hit_addr, GUINT_TO_POINTER(v_addr));
        return ACTION_HANDLE_CONTINUE;
    }

    return handle_cf_single_step();
}

ACTION_HANDLE_RES handle_step_out() {
    show_log("Handling step out\n");

    // the only option here is that we've reached the return address
    step_out_cleanup();

    const uintptr_t pc= target.target_get_pc();
    const uintptr_t v_pc= target.target_addr_runtime_to_virtual(pc);
    g_idle_add(hit_addr, GUINT_TO_POINTER(v_pc));

    return ACTION_HANDLE_CONTINUE;
}

ACTION_HANDLE_RES handle_action(Action* action) {
    switch (action->type) {
        case ACTION_CF_CONTINUE: {
            return handle_cf_continue();
        }

        case ACTION_BP_ADD: {
            const uintptr_t addr= action->data.BP_ADD.addr;
            const uint32_t line= action->data.BP_ADD.line;
            long long res;

            if (line == -1) {
                res= target.target_place_bp_at_addr(addr, -1, BP_REASON_USER);
            } else {
                res= target.target_place_bp_at_line(line);
            }

            if (res) show_log("Failed to place bp at %d with errno %lld of %s\n", line, res, strerror(res));
            else show_log("Placed bp at line %d on addr 0x%#lx\n", line, addr);

            return ACTION_HANDLE_CONTINUE;
        }

        case ACTION_BP_REMOVE: {
            show_log("Removing breakpoint\n");
            const uint32_t line= action->data.BP_REMOVE.line;
            long long res;
            if (line == -1) {
                res= target.target_remove_bp_at_addr(action->data.BP_REMOVE.addr, BP_REASON_USER);
            } else {
                res= target.target_remove_bp_at_line(line, BP_REASON_USER);
            }

            if (res) show_log("Failed to place bp at %d with errno %lld of %s\n", line, res, strerror(res));
            else show_log("Removed bp at line %d\n", line);

            return ACTION_HANDLE_CONTINUE;
        }

        case ACTION_BP_CAUSE: {
            break_on_cause("res", 34);

            return ACTION_HANDLE_CONTINUE;
        }

        case ACTION_CF_SINGLE_STEP: {
            bool assembly_level= action->data.CF_SINGLE_STEP.assembly_level;
            show_log("Control got single step at %s level\n", assembly_level ? "assembly" : "line");
            if (assembly_level) {
                return handle_cf_single_step();
            } else {
                show_log("Got request to single step line; not impl\n");
            }
            break;
        }
        case ACTION_CF_STEP_INTO: {
            const uintptr_t c_addr= target.target_get_pc();
            const uintptr_t v_addr= (uintptr_t)target.target_addr_runtime_to_virtual(c_addr);
            show_log("Current addr: %#lX\n", c_addr);
            show_log("Current virtual addr: %#lX\n", v_addr);
            const AddrLineRes line= addr2line(v_addr);
            if (!line.succ) {
                perror("Unable to find line associated with current program counter to step into");
                return ACTION_HANDLE_CONTINUE;
            }
            step_into_line= line.line;
            state= STATE_STEP_INTO;
            target.target_cf_single_step_assembly();

            return ACTION_HANDLE_PROC_WAIT;
        }
        case ACTION_CF_STEP_OVER: {
            const uintptr_t c_addr= target.target_get_pc();
            const uintptr_t v_addr= target.target_addr_runtime_to_virtual(c_addr);
            show_log("Current addr: %#lX\n", c_addr);
            show_log("Current virtual addr: %#lX\n", v_addr);
            const AddrLineRes line= addr2line(v_addr);
            if (!line.succ) {
                perror("Unable to find line associated with current program counter to step over");
                return ACTION_HANDLE_CONTINUE;
            }
            bool succ;
            step_over_info.line= line.line;
            step_over_info.ip_value= v_addr;
            step_over_info.cfa_value= target.target_get_cfa(&succ);
            step_over_info.temp_bp_addr= -1;
            show_log("CFA value of %#lx\n", step_over_info.cfa_value);
            state= STATE_STEP_OVER;
            target.target_cf_single_step_assembly();

            return ACTION_HANDLE_PROC_WAIT;
        }
        case ACTION_CF_STEP_OUT: {
            const uintptr_t return_address= target.target_get_return_addr();
            show_log("The return address gotten is %#lx\n", return_address);
            show_log("The virtual address is %#lx\n", target.target_addr_runtime_to_virtual(return_address));
            target.target_place_temp_bp(return_address, BP_REASON_STEP_OVER);
            step_out_addr= return_address;
            state= STATE_STEP_OUT;
            target.target_cf_continue();
            return ACTION_HANDLE_PROC_WAIT;
        }
        case ACTION_DS_REGS: {
            LabelledRegs lregs= target.target_get_labelled_regs();
            display_labelled_regs(lregs);
            return ACTION_HANDLE_CONTINUE;
        }
        case ACTION_DS_STACK_UNWIND: {
            target.target_unwind_stack();
            return ACTION_HANDLE_CONTINUE;
        }
        case ACTION_AT_QUIT:
        case ACTION_CF_EXIT:
            ptrace(PTRACE_KILL, target.pid, NULL, 0);

            return ACTION_HANDLE_EXIT;
        default: assert(false);
    }
}

ACTION_HANDLE_RES handle_signal_trap(const BPAddressInfo* bp) {
    siginfo_t siginfo;
    ptrace(PTRACE_GETSIGINFO, target.pid, 0, &siginfo);

    switch (siginfo.si_code) {
        case TRAP_BRKPT:
        case TRAP_HWBKPT:
        case SI_KERNEL: {
            switch (state) {
                case STATE_NORMAL:
                case STATE_STEP_INTO: return ACTION_HANDLE_CONTINUE;

                case STATE_STEP_OUT: {
                    break;
                }

                case STATE_STEP_OVER: {
                    if (bp->temp_bp_count > 0) {
                        return handle_step_over();
                    }
                    return ACTION_HANDLE_CONTINUE;
                }
            }
        }
        case TRAP_TRACE: {
            if (target.sw_bp_to_readd_addr != -1) {
                // there was a sw bp that we stepped over to re-add the shadow
                show_log("About to re-add sw bp\n");
                BPAddressInfo* info= BPAddressInfo_arr_search_ie(&bp_info, target.sw_bp_to_readd_addr);
                if (info && info->canonical_bp.type == BP_SOFTWARE) {
                    target.target_readd_sw_bp(&info->canonical_bp);
                    target.sw_bp_to_readd_addr= -1;
                    if (target.sw_bp_should_continue) return handle_cf_continue();
                }
            }
            switch (state) {
                case STATE_STEP_INTO: return handle_step_into_step();
                case STATE_STEP_OVER: return handle_step_over();
                case STATE_STEP_OUT: return handle_step_out();
                case STATE_NORMAL: {
                    show_log("Recieved trap in normal state\n");
                    break;
                }
                default: assert(false);
            }
        }
        default: return ACTION_HANDLE_CONTINUE;
    }

    return ACTION_HANDLE_CONTINUE;
}

void control_target(const char* filepath_p);
void* control_thread_create(void* data) {
    // at first the control thread just exists, it doesn't need to attach or anything
    // just waits on the action queue waiting for the run

    while (true) {
        const Action* action= queueb_pop_blocking(&action_q);

        if (action->type == ACTION_AT_ATTACH) {
            control_target(action->data.AT_ATTACH.filepath);
        }

        if (action->type == ACTION_AT_QUIT) return NULL;
    }
}

void control_target(const char* filepath_p) {
    long res;
    int ret;
    int status;
    char* filepath= filepath_p;

    breakpoint_program(filepath);

    ret = waitpid(target.pid, &status, 0);
    if (!WIFSTOPPED(status)) {
        show_err("Tracee did not stop\n");
    }

    if (status >> 16 == PTRACE_EVENT_EXEC) {
        show_log("THIS WAS FROM EXEC\n");
    } else {
        show_log("FIRST CALL WAS NOT EXEC\n");
    }

    ptrace(PTRACE_SETOPTIONS, target.pid, 0, PTRACE_O_TRACEEXEC);
    // the child is created but it's just a fork, it's about to run execv after CONT
    ptrace(PTRACE_CONT, target.pid, NULL, 0);

    ret = waitpid(target.pid, &status, 0);
    if (!WIFSTOPPED(status)) {
        show_err("Tracee did not stop\n");
    }

    if (status >> 16 == PTRACE_EVENT_EXEC) {
        show_log("THIS WAS FROM EXEC\n");
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
        show_log("Starting to wait\n");
        ret = waitpid(target.pid, &status, __WALL);
        if (ret == -1) {
            perror("waitpid");
            break;
        }
        show_log("!!Stopped having to wait\n");

        if (WIFEXITED(status)) {
            hlog("Target exited with %d\n", WEXITSTATUS(status));
            break;
        }

        g_idle_add(update_breakpoint_memory, NULL);

        const long long r6= ptrace(PTRACE_PEEKUSER, target.pid, offsetof(struct user, u_debugreg[6]));

        long long rip= ptrace(PTRACE_PEEKUSER, target.pid, offsetof(struct user, regs.rip));

        BPAddressInfo* bp= NULL;
        const int signal= WSTOPSIG(status);
        bool is_trace= false;
        if (WIFSTOPPED(status) && signal == SIGTRAP) {
            siginfo_t siginfo;
            ptrace(PTRACE_GETSIGINFO, target.pid, 0, &siginfo);

            if (siginfo.si_code == TRAP_TRACE) is_trace= true;

            if (siginfo.si_code == TRAP_BRKPT || siginfo.si_code == SI_KERNEL) {
                bp= BPAddressInfo_arr_search_ie(&bp_info, rip - 1);
            } else {
                bp= BPAddressInfo_arr_search_ie(&bp_info, rip);
            }
        }

        if (!is_trace && !(r6 & 0b1111) && bp && bp->canonical_bp.type == BP_SOFTWARE) {
            show_log("Declared as hitting a software breakpoint, reducing rip by 1\n");
            // if we're not hardware i.e. software then we are one ahead
            rip--;
            ptrace(PTRACE_POKEUSER, target.pid, (void*)offsetof(struct user, regs.rip), (void*)rip);
        }

        const uintptr_t c_vaddr= target.target_addr_runtime_to_virtual(rip);
        if (c_vaddr != -1)
            g_idle_add(hit_addr, GUINT_TO_POINTER(c_vaddr));

        if (!bp || bp->bp_count == 0) {
            hlog("The tracee stopped via a non breakpoint event with rip= %p\n", (void*)rip);
        } else {
            const AddrLineRes res= addr2line(c_vaddr);
            uint32_t line;
            if (!res.succ) line= -1;
            else line= res.line;
            const uintptr_t rip_p= rip;
            hlog("The tracee stopped via %s breakpoint at %#lx which is in line %u, there are %u breakpoints at this location\n",
                BP_TYPE_STRS[bp->canonical_bp.type],
                rip_p,
                line,
                bp->bp_count
            );
        }

        if (bp) {
            bool succ;
            const uintptr_t cfa= target.target_get_cfa(&succ);
            BPArray bp_copy= BP_arr_construct(bp->bps.pos);
            for (int i = 0; i < bp->bps.pos; i++) {
                BP_arr_add(&bp_copy, BP_arr_get(&bp->bps, i));
            }

            for (int i = 0; i < bp_copy.pos; ++i) {
                const BP* point= BP_arr_ptr(&bp_copy, i);

                if (point->cfa == -1 || point->cfa == cfa) {
                    if (point->callback) {
                        point->callback(point->data);
                    }
                }
            }

            printf("test");
        }

        target.target_breakpoint_hit_cleanup();

        if (WIFSTOPPED(status)) {
            hlog("Target stopped by signal %d\n", WSTOPSIG(status));

            const int signal= WSTOPSIG(status);

            if (signal == SIGILL || signal == SIGSEGV) {
                target.target_get_data_runtime(rip, 16);
                AddrLineRes res= addr2line(target.target_addr_runtime_to_virtual(rip));
                show_log("The line is %lu succ: %u\n", res.line, res.succ);
            }
            if (signal == SIGTRAP) {
                switch (handle_signal_trap(bp)) {
                    case ACTION_HANDLE_PROC_WAIT:
                        goto proc_wait_loop;
                    case ACTION_HANDLE_CONTINUE:
                        break;
                    default: assert(false);
                }
            }

            void* q_status;
            while (q_status= queueb_pop_blocking(&action_q), q_status) {
                Action* action= q_status;

                switch (handle_action(action)) {
                    case ACTION_HANDLE_EXIT:
                        goto end;
                    case ACTION_HANDLE_PROC_WAIT:
                        goto proc_wait_loop;
                    case ACTION_HANDLE_CONTINUE:
                        continue;
                }
            }
        proc_wait_loop:;
        }
    }
end:;
}
