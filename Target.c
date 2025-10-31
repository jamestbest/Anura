//
// Created by james on 30/10/25.
//

#include "Target.h"

#include "Target/Linux-x64.h"

Target target;

int init_target(TARGETS target_type) {
    switch (target_type) {
        case TARGET_LINUX_X64:
            linux_x64_init_target(&target);
            break;
    }
}
