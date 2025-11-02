//
// Created by jamestbest on 10/20/25.
//

#include "Linux.h"

#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <unistd.h>

PROCESS_ID launch_process(const char* path, uint32_t argc, const char* argv[]) {
    pid_t pid= fork();

    if (pid == 0) {
        ptrace(PTRACE_TRACEME, getpid(), NULL, 0);

        // we're the sub proc
        int ret= execvp(path, (char* const*)argv);

        exit(0);
    }

    return pid;
}

long long attach_process(PROCESS_ID pid) {
    return ptrace(PTRACE_ATTACH, pid, 0, 0);
}

void linux_init_target(Target* target) {
    target->target_launch_process= launch_process;
    target->target_attach_process= attach_process;
}