//
// Created by jamestbest on 3/9/26.
//

#include "break_cause_simple.h"

#define FAIL 1
#define SUCCESS 0

int t3() {
    return 0;
}

int t2() {
    return 0;
}

int t1() {
    if (t2()) {
        return FAIL;
    }

    if (t3()) {
        return SUCCESS;
    }

    return FAIL;
}

int main() {
    int res= SUCCESS;

    if (t1() == FAIL) {
        res= FAIL;
    }

    return res;
}
