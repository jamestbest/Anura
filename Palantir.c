//
// Created by James Coward on 10/17/25.
//

// Palantir is the temporary TUI for Anura
// it is a seperate thread from the main processing
// adds actions to a queue for the main processor to deal with later

#include "Palantir.h"

#include "main.h"
#include "QueueB.h"
#include "Saruman/Saruman.h"

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "break_on_cause.h"
#include "Helper_File.h"

int tui_setup() {
    target.target_interrupt_handler_setup();
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
 * Same for the SIGSTOP generative queue -- just a mutex protected count
 * Just pop the top SIGSTOP generator off when verifying, we can't know which of the signals is external, just that
 *  ones at the end are extra
 * INTERNAL_SIGSTOP -> INTERNAL_SIGSTOP -> EXTERNAL_SIGSTOP -> INTERNAL_SIGSTOP
 *  queue: SIGSTOP, SIGSTOP, SIGSTOP
 * handling:
 *  receive stop, pop queue -> receive stop, pop queue -> receive stop, pop queue -> receive stop, NOTHING TO POP
 * any actions are all delt with at each SIGSTOP, even if bp_add, bp_add placed two SIGSTOPs it shouldn't matter
 * as they'll both be in the generator list.
 */

#define NO_DATA (ACTION_DATA){.NO_DATA= 0}
#define MATCH_STR(str) (strncmp(buff.data, str, sizeof(str) - 1) == 0)
#include <fcntl.h>
int tui_loop() {
    FILE* pipe_file= fdopen(tui_pipe[0], "r");
    if (!pipe_file) show_err("Unable to open file for tui reading\n");

    Buffer buff= buffer_create(BUFF_MIN);
    while (true) {
        int line= -1;
        if (!get_line(pipe_file, &buff)) {
            show_err("Failed to get line ending look");
            break;
        }

        if (MATCH_STR("set")) {
            sscanf(buff.data, "set %d", &line);
            LineAddrRes res= line2startaddr(line);
            if (!res.succ) {
                show_err("There is no code on line %d\n", line);
                continue;
            }

            queueb_push_blocking(
                &action_q,
                create_action(
                    ACTION_BP_ADD,
                    (ACTION_DATA){
                        .BP_ADD= {
                            .addr= res.addr,
                            .line= line
                        }
                    }
                )
            );
        } else if (MATCH_STR("del")) {
            sscanf(buff.data, "del %d", &line);
            LineAddrRes res= line2startaddr(line);
            if (!res.succ) {
                printf("There is no code on line %d\n", line);
                continue;
            }

            queueb_push_blocking(
                &action_q,
                create_action(
                    ACTION_BP_REMOVE,
                    (ACTION_DATA){
                        .BP_REMOVE= {
                            .addr= res.addr,
                            .line= line
                        }
                    }
                )
            );
        } else if (MATCH_STR("cont")) {
            printf("tui Continuing process\n");
            errno= 0;
            queueb_push_blocking(
                &action_q,
                create_action(
                    ACTION_CF_CONTINUE,
                    NO_DATA
                )
            );
        } else if (MATCH_STR("astep")) {
            printf("Assembly level single step\n");
            queueb_push_blocking(
                &action_q,
                create_action(
                    ACTION_CF_SINGLE_STEP,
                    (ACTION_DATA){.CF_SINGLE_STEP= {.assembly_level= true}}
                )
            );
        } else if (MATCH_STR("exit")) {
            printf("Exiting process\n");
            queueb_push_blocking(
                &action_q,
                create_action(
                    ACTION_CF_EXIT,
                    NO_DATA
                )
            );
            target.target_interrupt();
            break;
        } else if (MATCH_STR("into")) {
            printf("Stepping into\n");
            queueb_push_blocking(
                &action_q,
                create_action(
                    ACTION_CF_STEP_INTO,
                    NO_DATA
                )
            );
        } else if (MATCH_STR("over")) {
            printf("Stepping over\n");
            queueb_push_blocking(
                &action_q,
                create_action(
                    ACTION_CF_STEP_OVER,
                    NO_DATA
                )
            );
        } else if (MATCH_STR("out")) {
            printf("Stepping out\n");
            queueb_push_blocking(
                &action_q,
                create_action(
                    ACTION_CF_STEP_OUT,
                    NO_DATA
                )
            );
        } else if (MATCH_STR("regs")) {
            printf("Sending request for registers\n");
            queueb_push_blocking(
                &action_q,
                create_action(
                    ACTION_DS_REGS,
                    NO_DATA
                )
            );
        } else if (MATCH_STR("list")) {
            print_breakpoints();
        } else if (MATCH_STR("stack")) {
            queueb_push_blocking(
                &action_q,
                create_action(
                    ACTION_DS_STACK_UNWIND,
                    NO_DATA
                )
            );
        } else if (MATCH_STR("cause")) {
            char choice[21];
            sscanf(buff.data, "cause %[^\n]20s", choice);
            ACTION_DATA data;
            if (strncmp(choice, "EXAMPLE", sizeof("EXAMPLE") - 1) == 0) {
                data.BP_CAUSE.is_simple= false;
            } else if (strncmp(choice, "SIMPLE", sizeof("SIMPLE") - 1) == 0) {
                data.BP_CAUSE.is_simple= true;
            } else {
                show_err("Invalid break cause input, expected EXAMPLE or SIMPLE\n");
                continue;
            }
            queueb_push_blocking(
                &action_q,
                create_action(ACTION_BP_CAUSE,
                    data
                )
            );
        } else {
            printf("Unable to match command `%s`\n", buff.data);
        }
    }

    return 0;
}

void display_labelled_regs(LabelledRegs lregs) {
    printf("Regs: \n");
    for (size_t i = 0; i < lregs.regs.pos; ++i) {
        const LabelledReg* reg= LabelledReg_arr_ptr(&lregs.regs, i);

        printf("\t%s (%u): %#lx\n", reg->name, reg->reg_num, reg->reg.value.general);
    }
    newline();
}

