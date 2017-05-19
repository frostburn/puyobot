unsigned long long lrand() {
    return rand() | (((unsigned long long) rand()) << 31) | (((unsigned long long) rand()) << 62);
}

double drand() {
    return rand() / ((double) RAND_MAX);
}

int ceil_div(int x, int y) {
    return (x + y  - 1) / y;
}

int scan(unsigned long long flags) {
    if (!flags) {
        return -1;
    }
    int index = 0;
    unsigned long long p = 1;
    while (1) {
        if (flags & p) {
            return index;
        }
        p <<= 1;
        ++index;
    }
}
