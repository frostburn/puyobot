#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define WIDTH (6)
#define HEIGHT (10)
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
        for (int i = 0; i < HEIGHT * WIDTH; ++i) {
            if (i % V_SHIFT == 0) {
                int l = i / V_SHIFT + j * HEIGHT;
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

int clear_groups(state *s) {
    assert(NUM_FLOORS == 2);
    assert(WIDTH % 2 == 0);
    int num_cleared = 0;
    for (int i = 0; i < NUM_COLORS - 1; ++i) {
        puyos_t top = s->floors[0][i];
        puyos_t bottom = s->floors[1][i];

        for (int j = 0; j < HEIGHT * WIDTH; j += 2) {
            puyos_t top_group = 3ULL << j;
            puyos_t bottom_group = 0;
            puyos_t top_extra = 0;

            top_group = flood(top_group, top);
            top ^= top_group;
            if (top_group & BOTTOM) {
                bottom_group = top_group >> (V_SHIFT * (HEIGHT - 1));
                bottom_group = flood(bottom_group, bottom);
                bottom ^= bottom_group;
                if (bottom_group & TOP) {
                    top_extra = bottom_group << (V_SHIFT * (HEIGHT - 1));
                    top_extra = flood(top_group, top);
                    top ^= top_extra;
                    top_group |= top_extra;
                    // XXX: It's not correct to stop here, but we don't care.
                    // Groups that snake around top and bottom are rare under normal play.
                }
            }
            int group_size = popcount(top_group) + popcount(bottom_group);
            if (group_size >= CLEAR_THRESHOLD) {
                s->floors[0][i] ^= top_group;
                s->floors[1][i] ^= bottom_group;
                num_cleared += group_size;

                s->floors[0][GARBAGE] &= ~(cross(top_group) | ((bottom_group & TOP) << (V_SHIFT * (HEIGHT - 1))));
                s->floors[1][GARBAGE] &= ~(cross(bottom_group) | ((top_group & BOTTOM) >> (V_SHIFT * (HEIGHT - 1))));
            }

            bottom_group = 3ULL << j;
            bottom_group = flood(bottom_group, bottom);
            bottom ^= bottom_group;
            group_size = popcount(bottom_group);
            if (group_size >= CLEAR_THRESHOLD) {
                s->floors[1][i] ^= bottom_group;
                num_cleared += group_size;

                s->floors[0][GARBAGE] &= ~((bottom_group & TOP) << (V_SHIFT * (HEIGHT - 1)));
                s->floors[1][GARBAGE] &= ~cross(bottom_group);
            }
        }
    }
    return num_cleared;
}

// Reference for bit parallel "raindrop" gravity
puyos_t drop_once(puyos_t puyos) {
    puyos_t bellow = (puyos >> V_SHIFT) | BOTTOM;
    puyos_t falling = puyos & ~bellow;
    return (falling << V_SHIFT) | (puyos & ~falling);
}

void handle_gravity(state *s) {
    assert(NUM_FLOORS == 2);
    puyos_t all[2];
    for (int i = 0; i < NUM_FLOORS; ++i) {
        all[i] = 0;
        for (int j = 0; j < NUM_COLORS; ++j) {
            all[i] |= s->floors[i][j];
        }
    }

    puyos_t temp[2];
    do {
        temp[0] = all[0];
        temp[1] = all[1];
        puyos_t bellow, falling;
        bellow = (all[1] >> V_SHIFT) | BOTTOM;
        all[1] = 0;
        for (int i = 0; i < NUM_COLORS; ++i) {
            falling = s->floors[1][i] & ~bellow;
            s->floors[1][i] = (falling << V_SHIFT) | (s->floors[1][i] & ~falling);
            all[1] |= s->floors[1][i];
        }

        bellow = (all[0] >> V_SHIFT) | ((all[1] & TOP) << (V_SHIFT * (HEIGHT - 1)));
        all[0] = 0;
        for (int i = 0; i < NUM_COLORS; ++i) {
            falling = s->floors[0][i] & ~bellow;

            s->floors[1][i] |= (falling & BOTTOM) >> (V_SHIFT * (HEIGHT - 1));

            s->floors[0][i] = (falling << V_SHIFT) | (s->floors[0][i] & ~falling);
            all[0] |= s->floors[0][i];
        }
    } while (temp[0] != all[0] || temp[1] != all[1]);
}

void reference_gravity(state *s) {
    assert(NUM_FLOORS == 2);
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

int resolve(state *s) {
    int chain = -1;
    do {
        ++chain;
        handle_gravity(s);
    } while(clear_groups(s));
    return chain;
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
    state *ref = calloc(1, sizeof(state));
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
    state *s = calloc(1, sizeof(state));
    srand(time(NULL));
    for (int i = 0; i < WIDTH * HEIGHT; ++i) {
        for (int j = 0; j < NUM_FLOORS; ++j) {
            int color = rand() % NUM_COLORS;
            s->floors[j][color] |= 1ULL << i;
        }
    }
    for (test_color = 0; test_color < NUM_COLORS - 1; ++test_color) {
        for (int i = 0; i < WIDTH * HEIGHT; ++i) {
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
    state *s = malloc(sizeof(state));
    srand(time(NULL));

    unsigned long total_chain = 0;
    for (unsigned long k = 0; k < 1000000; ++k) {
        for (int j = 0; j < NUM_FLOORS; j++) {
            for (int i = 0; i < NUM_COLORS; ++i) {
                s->floors[j][i] = 0;
            }
        }
        for (int i = 0; i < WIDTH * HEIGHT; ++i) {
            for (int j = 0; j < NUM_FLOORS; ++j) {
                int color = rand() % NUM_COLORS;
                s->floors[j][color] |= 1ULL << i;
            }
        }
        total_chain += resolve(s);
    }
    printf("%lu\n", total_chain);

    return 0;
}
