//
// Created by james on 30/10/25.
//

#ifndef LINUX_X64_H
#define LINUX_X64_H

#include "../Target.h"

long long place_bp(void* address);
int linux_x64_init_target(Target* target);



#endif //LINUX_X64_H
