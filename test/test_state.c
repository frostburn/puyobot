#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "jkiss/jkiss.h"

#include "puyobot/state.h"

void reference_gravity(State *s) {
    assert(NUM_FLOORS == 2);
    puyos_t all[NUM_FLOORS];
    get_state_mask(s, all);
    for (int i = 0; i < WIDTH; ++i) {
        int pile_size = 0;
        for (int j = 0; j < TOTAL_HEIGHT; ++j) {
            int scanner_floor = 1;
            int scanner_y = HEIGHT - 1 - j;
            if (j >= HEIGHT) {
                scanner_floor = 0;
                scanner_y = 2 * HEIGHT - 1 - j;
            }
            puyos_t scanner = 1ULL << (scanner_y * WIDTH + i);
            if (!(scanner & all[scanner_floor])) {
                continue;
            }
            for (int k = 0; k < NUM_COLORS; ++k) {
                if (scanner & s->floors[scanner_floor][k]) {
                    s->floors[scanner_floor][k] ^= scanner;
                    int pile_floor = 1;
                    int pile_y = HEIGHT - 1 - pile_size;
                    if (pile_size >= HEIGHT) {
                        pile_floor = 0;
                        pile_y = 2 * HEIGHT - 1 - pile_size;
                    }
                    s->floors[pile_floor][k] |= 1ULL << (pile_y * WIDTH + i);
                    ++pile_size;
                    break;
                }
            }
        }
    }
}

void test_gravity() {
    State *s = calloc(1, sizeof(State));
    State *ref = calloc(1, sizeof(State));
    int puyo_count = 0;
    for (int j = 0; j < NUM_FLOORS; j++) {
        puyos_t allowed = FULL;
        for (int i = 0; i < NUM_COLORS; ++i) {
            puyos_t candidates = jlrand() & jlrand() & jlrand();
            candidates &= allowed;
            allowed ^= candidates;
            s->floors[j][i] = candidates;
            puyo_count += popcount(candidates);
            ref->floors[j][i] = s->floors[j][i];
        }
    }
    print_state(s);
    handle_gravity(s);
    reference_gravity(ref);
    print_state(s);
    print_state(ref);
    int new_puyo_count = 0;
    for (int j = 0; j < NUM_FLOORS; j++) {
        for (int i = 0; i < NUM_COLORS; ++i) {
            new_puyo_count += popcount(s->floors[j][i]);
            assert(ref->floors[j][i] == s->floors[j][i]);
        }
    }
    assert(puyo_count == new_puyo_count);
}


void test_clear() {
    int test_color;
    puyos_t test_group;
    State *s = calloc(1, sizeof(State));
    for (int i = 0; i < WIDTH * HEIGHT; ++i) {
        for (int j = 0; j < NUM_FLOORS; ++j) {
            int color = jrand() % NUM_COLORS;
            s->floors[j][color] |= 1ULL << i;
        }
    }
    for (test_color = 0; test_color < NUM_COLORS - 1; ++test_color) {
        for (int i = 0; i < WIDTH * HEIGHT; ++i) {
            test_group = flood(1ULL << i, s->floors[1][test_color]);
            if (popcount(test_group) >= CLEAR_THRESHOLD) {
                break;
            }
            else {
                test_group = 0;
            }
        }
        if (popcount(test_group) >= CLEAR_THRESHOLD) {
            break;
        }
    }
    print_state(s);
    print_puyos(test_group);
    clear_groups(s, 0);
    print_state(s);
    if (test_group) {
        assert(!(s->floors[1][test_color] & test_group));
        assert(!(s->floors[1][GARBAGE] & cross(test_group)));
    }
}

void test_clear_with_shift() {
    puyos_t noise = 3833423454597982578ULL & FULL;
    for (int i = GHOST_Y + 1; i < HEIGHT + 1; ++i) {
        State *s = calloc(1, sizeof(State));
        s->floors[0][0] = noise;
        for (int j = 0; j < i; ++j) {
            shift_down(s);
        }
        print_state(s);
        printf("%d, %d\n", popcount(s->floors[0][0]), popcount(s->floors[1][0]));
        assert(popcount(s->floors[0][0]) + popcount(s->floors[1][0]) == 29);
        clear_groups(s, 0);
        print_state(s);
        assert(popcount(s->floors[0][0]) + popcount(s->floors[1][0]) == 12);
        free(s);
    }
}

void test_ghost_chain() {
    State *s = calloc(1, sizeof(State));
    s->floors[1][GARBAGE] = FULL;
    s->floors[0][GARBAGE] = ~DEATH_BLOCK;
    s->floors[0][RED] = 15ULL << (V_SHIFT * (GHOST_Y + 1));
    s->floors[0][RED] |= 3ULL << (2 + V_SHIFT * (GHOST_Y));
    s->floors[0][RED] |= 3ULL << (4 + V_SHIFT * (GHOST_Y + 2));
    s->floors[0][GARBAGE] &= ~s->floors[0][RED];
    print_state(s);
    int chain = 0;
    resolve(s, &chain);
    print_state(s);
    assert(chain == 2);
    assert(!s->floors[0][RED]);
}

void test_state_euler() {
    State *s = calloc(1, sizeof(State));
    s->floors[0][0] = jlrand() & FULL & ~(LEFT_WALL | RIGHT_WALL);
    s->floors[1][0] = jlrand() & FULL & ~(BOTTOM | LEFT_WALL | RIGHT_WALL);
    int e = state_euler(s);
    print_state(s);
    printf("e=%d\n", e);
    s->floors[0][0] <<= 1;
    s->floors[1][0] <<= 1;
    assert(e == state_euler(s));
    s->floors[0][0] >>= 2;
    s->floors[1][0] >>= 2;
    assert(e == state_euler(s));
    shift_down(s);
    assert(e == state_euler(s));
    s->floors[0][0] <<= 2;
    s->floors[1][0] <<= 2;
    assert(e == state_euler(s));
}


int main() {
    jkiss_init();

    test_gravity();
    test_clear();
    test_clear_with_shift();
    test_state_euler();
    test_ghost_chain();
}