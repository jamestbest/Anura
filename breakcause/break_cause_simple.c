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
    return 1;
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


// int i= 0;
// int main() {
//     b:;
//     int res= SUCCESS;
//     if (i == 1) goto a;
//     i = 1;
//
//     if (t1() == FAIL) {
//         res= FAIL;
//     }
//     goto b;
//     a:
//
//     return res;
// }

int main() {
    int res= SUCCESS;

    if (t1() == FAIL) {
        res= FAIL;
    }

    return res;
}
