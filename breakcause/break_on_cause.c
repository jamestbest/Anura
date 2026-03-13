int error() {
    return false;       // cause point
}

int test() {
    int* c= malloc(sizeof(int) * 3);
    *c= false;
    for (int i= 0; i < 3; i++) {
        c[i]= true && false;
    }

    // int b= false; // cause point

    return c[2];
}

int funca() {
    if (a == b) {
        return error();
    }

    if (c == d) return true;    // cause point

    return funcb();
}

int funcb() {
    bool a= funcc()
    if (a != b) {
        return a && funca() && error();
    }

    return error();
}

int do_something(int* ptr) {
    switch (*ptr) {
        case 1: return funca();
        case 2: return funcb();
        case 3: return false;       // cause point
    }
}

int main() {
    int success= true;
    int* a= malloc(sizeof(int) * 3);
    do {
        success= do_somthing(a);
    } while (success);
}