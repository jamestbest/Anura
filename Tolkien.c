//
// Created by james on 31/10/25.
//

#include "Tolkien.h"

#include <stdarg.h>
#include <stdio.h>
#include "shared/Colours.h"

/* * * * * * * * * * * * * * * * * * * * *\
 *                                        *
 *  WARNING THIS LOGGER IS NOT ATOMIC!!   *
 *                                        *
\* * * * * * * * * * * * * * * * * * * * */

FILE* out;
const char* LOG_PREFIX="";
const char* INF_PREFIX=C_BLU"[[INFO]]"C_RST;
const char* WRN_PREFIX=C_MGN"[[WARN]]"C_RST;
const char* ERR_PREFIX=C_RED"[[ERROR]]"C_RST;

void logger_init() {
    out= stdout;
}

void redirect(FILE* n_out) {
    out= n_out;
}

void log(const char* message, ...) {
    fputs(LOG_PREFIX, out);

    va_list args;
    va_start(args, message);
    vfprintf(out, message, args);
    va_end(args);
}

void inf(const char* message, ...) {
    fputs(INF_PREFIX, out);

    va_list args;
    va_start(args, message);
    vfprintf(out, message, args);
    va_end(args);
}

void wrn(const char* message, ...) {
    fputs(WRN_PREFIX, out);

    va_list args;
    va_start(args, message);
    vfprintf(out, message, args);
    va_end(args);
}

void err(const char* message, ...) {
    fputs(ERR_PREFIX, out);

    va_list args;
    va_start(args, message);
    vfprintf(out, message, args);
    va_end(args);
}

