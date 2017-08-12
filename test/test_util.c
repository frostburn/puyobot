#include <assert.h>
#include <stdio.h>

#include "jkiss/jkiss.h"

#include "puyobot/util.h"

void test_shuffle() {
    short int values[] = {0, 1, 2, 3, 4, 5, 6, 7};
    shuffle(values, 8, sizeof(short int));
    unsigned char flags = 0;
    for (int i = 0; i < 8; ++i) {
        printf("%d\n", values[i]);
        flags |= 1 << values[i];
    }
    assert(flags = ~0);
}

void test_bitset() {
    assert(bitset_rand_index(0) == -1);
    for (int i = 0; i < 100; ++i) {
        unsigned int bitset = jrand();
        if (!bitset) {
            continue;
        }
        int counts[32] = {0};
        for (int j = 0; j < 10000; ++j) {
            counts[bitset_rand_index(bitset)]++;
        }
        for (int j = 0; j < 32; ++j) {
            printf("%d ", counts[j]);
            assert(counts[j] == 0 || counts[j] > 10);
        }
        printf("\n");
    }
}

int main() {
    jkiss_init();

    test_shuffle();
    test_bitset();
}
