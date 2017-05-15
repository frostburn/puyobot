unsigned long long lrand() {
    return rand() | (((unsigned long long) rand()) << 31) | (((unsigned long long) rand()) << 62);
}

double drand() {
    return rand() / ((double) RAND_MAX);
}
