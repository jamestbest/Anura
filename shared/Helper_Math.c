//
// Created by jamescoward on 08/11/2023.
//

#include "Helper_Math.h"

size_t smin(const size_t a, const size_t b) {
    return a < b ? a : b;
}

uint umin(const uint a, const uint b) {
    return a < b ? a : b;
}

int min(const int a, const int b) {
    return a < b ? a : b;
}

size_t smax(const size_t a, const size_t b) {
    return a > b ? a : b;
}

uint umax(const uint a, const uint b) {
    return a > b ? a : b;
}

int max(const int a, const int b) {
    return a > b ? a : b;
}


