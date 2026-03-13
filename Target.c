//
// Created by james on 30/10/25.
//

#include <assert.h>
#include <stdbool.h>

#include "Target.h"

#include "Target/Linux-x64.h"

Target target;

ARRAY_ADD(LabelledReg, LabelledReg)

int init_target(TARGETS target_type) {
    target.sw_bp_should_continue= false;
    target.sw_bp_to_readd_addr= -1;

    switch (target_type) {
        case TARGET_LINUX_X64:
            linux_x64_init_target(&target);
            return 0;
    }

    assert(false);
}
