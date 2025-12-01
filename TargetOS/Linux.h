//
// Created by jamestbest on 10/20/25.
//

#ifndef ANURA_LINUX_H
#define ANURA_LINUX_H

#include <stdint.h>

#include "../Target.h"

void target_set_register();
void target_set_debug_register(int db_reg);

uint64_t target_launch_process(const char* path);

void linux_init_target(Target* target);

#endif //ANURA_LINUX_H
