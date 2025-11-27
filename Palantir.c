//
// Created by jamestbest on 10/17/25.
//

// Palantir is the temporary TUI for Anura
// it is a seperate thread from the main processing
// adds actions to a queue for the main processor to deal with later

#include "Palantir.h"

#include "QueueB.h"
#include "Saruman.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "main.h"

int setup() {

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

int tui() {
    while (true) {
        printf("cmd: ");
        char buff[100];
        int line= -1;

        if (!fgets(buff, sizeof(buff), stdin))
            break;

        if (strncmp(buff, "set", sizeof("set") - 1) == 0) {
            sscanf(buff, "set %d", &line);
            LineAddrRes res= line2startaddr(line);
            if (!res.succ) {
                printf("There is no code on line %d\n", line);
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
        } else if (strncmp(buff, "cont", sizeof("cont") - 1) == 0) {
            printf("tui Continuing process\n");
            errno= 0;
            queueb_push_blocking(
                &action_q,
                create_action(
                    ACTION_CF_CONTINUE,
                    (ACTION_DATA){.NO_DATA= 0}
                )
            );
        } else if (strncmp(buff, "astep", sizeof("astep") - 1) == 0) {
            printf("Assembly level single step\n");
            queueb_push_blocking(
                &action_q,
                create_action(
                    ACTION_CF_SINGLE_STEP,
                    (ACTION_DATA){.CF_SINGLE_STEP= {.assembly_level= true}}
                )
            );
        } else if (strncmp(buff, "exit", sizeof("exit") - 1) == 0) {
            printf("Exiting process\n");
            queueb_push_blocking(
                &action_q,
                create_action(
                    ACTION_CF_EXIT,
                    (ACTION_DATA){.NO_DATA= 0}));
            break;
        } else {
            printf("Unable to match command `%s`\n", buff);
        }
    }

    return 0;
}
