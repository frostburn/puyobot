unsigned long long lrand() {
    return rand() | (((unsigned long long) rand()) << 31) | (((unsigned long long) rand()) << 62);
}

double drand() {
    return rand() / ((double) RAND_MAX);
}

/* Arrange the N elements of ARRAY in random order.
   Only effective if N is much smaller than RAND_MAX;
   if this may not be the case, use a better random
   number generator. */
void shuffle(int *array, size_t n) {
    if (n > 1) {
        for (size_t i = 0; i < n - 1; ++i) {
          size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
          int t = array[j];
          array[j] = array[i];
          array[i] = t;
        }
    }
}
