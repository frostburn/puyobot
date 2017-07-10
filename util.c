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

int cmp_int(const void *a, const void *b) {
    int *x = (int *) a;
    int *y = (int *) b;
    return *x - *y;
}
