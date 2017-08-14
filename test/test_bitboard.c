#include <assert.h>
#include <string.h>

#include "jkiss/jkiss.h"

#include "puyobot/bitboard.h"

void test_euler() {
    puyos_t noise = jlrand() & FULL & ~(TOP | BOTTOM | LEFT_WALL | RIGHT_WALL);
    int e = euler(noise);
    print_puyos(noise);
    printf("e=%d\n", e);
    assert(e == euler(noise >> H_SHIFT));
    assert(e == euler(noise << H_SHIFT));
    assert(e == euler(noise >> V_SHIFT));
    assert(e == euler(noise >> V_SHIFT));
}

void test_flood_2() {
    puyos_t noise[2] = {234987234897892347ULL & FULL, 2348972342394729384ULL & FULL};
    int clears [] = {11, 5, 4, 6, 1, 1, 1, 1, 1, 19};
    int num_clears = 10;
    print_puyos_2(noise);
    int j = 0;
    for (int i = 0; i < WIDTH * HEIGHT; i += 2) {
        puyos_t source[2] = {3ULL << i, 0};
        flood_2(source, noise);
        if (source[0] || source[1]) {
            noise[0] ^= source[0];
            noise[1] ^= source[1];
            print_puyos_2(noise);
            int cleared = popcount(source[0]) + popcount(source[1]);
            printf("cleared=%d\n", cleared);
            assert(clears[j++] == cleared);
        }
    }
    assert(j == num_clears);
    noise[0] = 234987234897892347ULL & FULL;
    noise[1] = 2348972342394729384ULL & FULL;
    int clears2[] = {19, 1, 11, 1, 1};
    num_clears = 5;
    j = 0;
    for (int i = 0; i < WIDTH * HEIGHT; i += 2) {
        puyos_t source[2] = {0, 3ULL << i};
        flood_2(source, noise);
        if (source[0] || source[1]) {
            noise[0] ^= source[0];
            noise[1] ^= source[1];
            print_puyos_2(noise);
            int cleared = popcount(source[0]) + popcount(source[1]);
            printf("cleared=%d\n", cleared);
            assert(clears2[j++] == cleared);
        }
    }
    assert(j == num_clears);
}

void test_mirror() {
    puyos_t puyos = 234928348972394;
    print_puyos(puyos);
    print_puyos(mirror_puyos(puyos));
    for (int i = 0; i < WIDTH; ++i) {
        assert(mirror_puyos(1ULL << i) == (1ULL << (WIDTH - 1 - i)));
    }
}

void test_translate() {
    puyos_t puyos[2] = {23498723498729384, 9829472398472934};
    print_puyos_2(puyos);
    puyos_t temp[2];
    memcpy(temp, puyos, 2 * sizeof(puyos_t));
    translate_2(temp, 1, 1);
    print_puyos_2(temp);
    right_2(puyos);
    down_2(puyos);
    assert(puyos[0] == temp[0]);
    assert(puyos[1] == temp[1]);

    puyos[0] = 23498723498729384;
    puyos[1] = 9829472398472934;
    memcpy(temp, puyos, 2 * sizeof(puyos_t));
    translate_2(temp, -2, -1);
    print_puyos_2(temp);
    left_2(puyos);
    left_2(puyos);
    up_2(puyos);
    assert(puyos[0] == temp[0]);
    assert(puyos[1] == temp[1]);
}

int main() {
    test_euler();
    test_flood_2();
    test_mirror();
    test_translate();
    return 0;
}
