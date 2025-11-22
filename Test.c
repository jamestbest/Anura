//
// Created by james on 26/10/25.
//

#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void vlog(bool is_t, const char* message, va_list args) {
    printf("LOG(%s): ", is_t ? "TARGET" : " HOST ");
    vprintf(message, args);
}

void tlog(const char* message, ...) {
    va_list args;
    va_start(args, message);
    vlog(true, message, args);
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

uint i=0;
int target_func(bool second) {
    tlog("PID: %d\n", getpid());
    tlog("I is at %p\n", &i);


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

    if (second) return 0;
    target_func(true);

    return 0;
}

int main(int argc, char** argv) {
    target_func(false);

    while (true) {}
    return 0;
}

