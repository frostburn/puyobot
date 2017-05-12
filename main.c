#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define WIDTH (6)
#define HEIGTH (10)
#define H_SHIFT (1)
#define V_SHIFT (6)
#define FULL (0xfffffffffffffffULL)
#define TOP (0x3fULL)
#define BOTTOM (0xfc0000000000000ULL)
#define LEFT_WALL (0x41041041041041ULL)
#define RIGHT_BLOCK (0xfbefbefbefbefbeULL)

#define NUM_FLOORS (2)
#define TOTAL_HEIGHT (20)
#define NUM_COLORS (6)
#define RED (0)
#define GREEN (1)
#define YELLOW (2)
#define BLUE (3)
#define PURPLE (4)
#define GARBAGE (5)
#define CLEAR_THRESHOLD (4)


typedef unsigned long int puyos_t;

typedef struct state
{
    puyos_t floors[NUM_FLOORS][NUM_COLORS];
    int chain;
} state;

int popcount(puyos_t puyos) {
    return __builtin_popcountll(puyos);
}

puyos_t lrand() {
    return rand() | (((puyos_t) rand()) << 31) | (((puyos_t) rand()) << 62);
}

void print_puyos(puyos_t puyos) {
    printf(" ");
    for (int i = 0; i < WIDTH; ++i) {
        printf(" %c", 'A' + i);
    }
    printf("\n");
    for (int i = 0; i < 64; ++i) {
        if (i % V_SHIFT == 0) {
            int j = i / V_SHIFT;
            if (j < 10) {
                printf("%d", j);
            } else {
                printf("%c", 'a' + j - 10);
            }
        }
        if ((1ULL << i) & puyos) {
            printf(" @");
        } else {
            printf("  ");
        }
        if (i % V_SHIFT == V_SHIFT - 1){
            printf("\n");
        }
    }
    printf("\n");
}

void print_state(state *s) {
    printf(" ");
    for (int i = 0; i < WIDTH; ++i) {
        printf(" %c", 'A' + i);
    }
    printf("\n");
    for (int j = 0; j < NUM_FLOORS; ++j) {
        for (int i = 0; i < HEIGTH * WIDTH; ++i) {
            if (i % V_SHIFT == 0) {
                int l = i / V_SHIFT + j * HEIGTH;
                if (l < 10) {
                    printf("%d", l);
                } else {
                    printf("%c", 'a' + l - 10);
                }
            }
            puyos_t p = (1ULL << i);
            int any = 0;
            for (int k = 0; k < NUM_COLORS; ++k) {
                if (p & s->floors[j][k]) {
                    printf("\x1b[3%d;1m", k + 1);
                    if (k == GARBAGE) {
                        printf(" ◎");
                    } else {
                        printf(" ●");
                    }
                    any  = 1;
                    break;
                }
            }
            printf("\x1b[0m");
            if (!any) {
                printf("  ");
            }
            if (i % V_SHIFT == V_SHIFT - 1){
                printf("\n");
            }
        }
    }
    printf("chain=%d\n", s->chain);
}

puyos_t cross(puyos_t puyos) {
    return (
        puyos |
        ((puyos & RIGHT_BLOCK) >> H_SHIFT) |
        ((puyos << H_SHIFT) & RIGHT_BLOCK) |
        (puyos << V_SHIFT) |
        (puyos >> V_SHIFT)
    );
}

puyos_t flood(register puyos_t source, register puyos_t target) {
    source &= target;
    if (!source){
        return source;
    }
    register puyos_t temp;
    do {
        temp = source;
        source |= (
            ((source & RIGHT_BLOCK) >> H_SHIFT) |
            ((source << H_SHIFT) & RIGHT_BLOCK) |
            (source << V_SHIFT) |
            (source >> V_SHIFT)
        ) & target;
    } while (temp != source);
    return source;
}

void clear_groups(state *s) {
    assert(NUM_FLOORS == 2);
    assert(WIDTH % 2 == 0);
    for (int i = 0; i < NUM_COLORS - 1; ++i) {
        puyos_t top = s->floors[0][i];
        puyos_t bottom = s->floors[1][i];

        for (int j = 0; j < HEIGTH * WIDTH; j += 2) {
            puyos_t top_group = 3ULL << j;
            puyos_t bottom_group = 0;
            puyos_t top_extra = 0;

            top_group = flood(top_group, top);
            top ^= top_group;
            if (top_group & BOTTOM) {
                bottom_group = top_group >> (V_SHIFT * (HEIGTH - 1));
                bottom_group = flood(bottom_group, bottom);
                bottom ^= bottom_group;
                if (bottom_group & TOP) {
                    top_extra = bottom_group << (V_SHIFT * (HEIGTH - 1));
                    top_extra = flood(top_group, top);
                    top ^= top_extra;
                    top_group |= top_extra;
                    // XXX: It's not correct to stop here, but we don't care.
                    // Groups that snake around top and bottom are rare under normal play.
                }
            }
            if (popcount(top_group) + popcount(bottom_group) >= CLEAR_THRESHOLD) {
                s->floors[0][i] ^= top_group;
                s->floors[1][i] ^= bottom_group;

                s->floors[0][GARBAGE] &= ~(cross(top_group) | ((bottom_group & TOP) << (V_SHIFT * (HEIGTH - 1))));
                s->floors[1][GARBAGE] &= ~(cross(bottom_group) | ((top_group & BOTTOM) >> (V_SHIFT * (HEIGTH - 1))));
            }

            bottom_group = 3ULL << j;
            bottom_group = flood(bottom_group, bottom);
            bottom ^= bottom_group;
            if (popcount(bottom_group) >= CLEAR_THRESHOLD) {
                s->floors[1][i] ^= bottom_group;

                s->floors[0][GARBAGE] &= ~((bottom_group & TOP) << (V_SHIFT * (HEIGTH - 1)));
                s->floors[1][GARBAGE] &= ~cross(bottom_group);
            }
        }
    }
}

// Reference for bit parallel gravity
puyos_t drop_once(puyos_t puyos) {
    puyos_t line = BOTTOM;
    for (int i = 0; i < HEIGTH; ++i) {
        puyos_t above = (puyos << V_SHIFT) & line;
        line >>= V_SHIFT;
        puyos = (puyos & ~line) | above | ((above & puyos) >> V_SHIFT);
    }
    return puyos;
}

void handle_gravity(state *s) {
    puyos_t all[2];
    for (int i = 0; i < NUM_FLOORS; ++i) {
        all[i] = 0;
        for (int j = 0; j < NUM_COLORS; ++j) {
            all[i] |= s->floors[i][j];
        }
    }
    for (int i = 0; i < WIDTH; ++i) {
        int pile_size = 0;
        for (int j = 0; j < TOTAL_HEIGHT; ++j) {
            int scanner_floor = 1;
            int scanner_y = HEIGTH - 1 - j;
            if (j >= HEIGTH) {
                scanner_floor = 0;
                scanner_y = 2 * HEIGTH - 1 - j;
            }
            puyos_t scanner = 1ULL << (scanner_y * WIDTH + i);
            if (!(scanner & all[scanner_floor])) {
                continue;
            }
            for (int k = 0; k < NUM_COLORS; ++k) {
                if (scanner & s->floors[scanner_floor][k]) {
                    s->floors[scanner_floor][k] ^= scanner;
                    int pile_floor = 1;
                    int pile_y = HEIGTH - 1 - pile_size;
                    if (pile_size >= HEIGTH) {
                        pile_floor = 0;
                        pile_y = 2 * HEIGTH - 1 - pile_size;
                    }
                    s->floors[pile_floor][k] |= 1ULL << (pile_y * WIDTH + i);
                    ++pile_size;
                    break;
                }
            }
        }
    }
}

void test_lrand() {
    srand(time(NULL));
    puyos_t result = 0;
    for (int i = 0; i < 10000; ++i) {
        result |= lrand();
    }
    assert(result == ~0);
}

void test_gravity() {
    state *s = calloc(1, sizeof(state));
    srand(time(NULL));
    int puyo_count = 0;
    for (int j = 0; j < NUM_FLOORS; j++) {
        puyos_t allowed = FULL;
        for (int i = 0; i < NUM_COLORS; ++i) {
            puyos_t candidates = lrand() & lrand() & lrand();
            candidates &= allowed;
            allowed ^= candidates;
            s->floors[j][i] = candidates;
            puyo_count += popcount(candidates);
        }
    }
    print_state(s);
    handle_gravity(s);
    int new_puyo_count = 0;
    for (int j = 0; j < NUM_FLOORS; j++) {
        for (int i = 0; i < NUM_COLORS; ++i) {
            new_puyo_count += popcount(s->floors[j][i]);
        }
    }
    print_state(s);
    assert(puyo_count == new_puyo_count);
}

void test_clear() {
    int test_color;
    puyos_t test_group;
    state *s = calloc(1, sizeof(state));
    srand(time(NULL));
    for (int i = 0; i < WIDTH * HEIGTH; ++i) {
        for (int j = 0; j < NUM_FLOORS; ++j) {
            int color = rand() % NUM_COLORS;
            s->floors[j][color] |= 1ULL << i;
        }
    }
    for (test_color = 0; test_color < NUM_COLORS - 1; ++test_color) {
        for (int i = 0; i < WIDTH * HEIGTH; ++i) {
            test_group = flood(1ULL << i, s->floors[0][test_color]);
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
    clear_groups(s);
    print_state(s);
    if (test_group) {
        assert(!(s->floors[0][test_color] & test_group));
        assert(!(s->floors[0][GARBAGE] & cross(test_group)));
    }
}

int main() {
    test_lrand();
    test_gravity();
    test_clear();
    return 0;
}
