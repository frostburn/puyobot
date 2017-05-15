#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

#define WIDTH (6)
#define HEIGHT (10)
#define H_SHIFT (1)
#define V_SHIFT (6)
#define FULL (0xfffffffffffffffULL)
#define TOP (0x3fULL)
#define BOTTOM (0xfc0000000000000ULL)
#define LEFT_WALL (0x41041041041041ULL)
#define RIGHT_BLOCK (0xfbefbefbefbefbeULL)
#define RIGHT_WALL (0x820820820820820ULL)
#define LEFT_BLOCK (0x7df7df7df7df7dfULL)

#define GHOST_Y (7)
#define DEATH_BLOCK (0x3ffffffffffULL)
#define GHOST_LINE (0xfc0000000000ULL)
#define LIFE_BLOCK (0xfff000000000000ULL)

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

#include "util.c"
#include "bitboard.c"
#include "scoring.c"

typedef struct state
{
    puyos_t floors[NUM_FLOORS][NUM_COLORS];
} state;

state* copy_state(state *s) {
    state *c = malloc(sizeof(state));
    memcpy(c, s, sizeof(state));
    return c;
}

void shift_down(state *s) {
    for (int j = 0; j < NUM_COLORS; ++j) {
        puyos_t leak = 0;
        for (int i = 0; i < NUM_FLOORS; ++i) {
            puyos_t temp = s->floors[i][j] & BOTTOM;
            s->floors[i][j] <<= V_SHIFT;
            s->floors[i][j] |= leak >> (V_SHIFT * (HEIGHT - 1));
            s->floors[i][j] &= FULL;
            leak = temp;
        }
    }
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
                    if (j == 0 && i / WIDTH == GHOST_Y) {
                        printf("\x1b[3%dm", k + 1);
                    } else {
                        printf("\x1b[3%d;1m", k + 1);
                    }
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
}

int state_euler(state *s) {
    int total = 0;
    puyos_t bottom_edges = 0;
    puyos_t bottom_vertices = 0;
    puyos_t bottom_corner = 0;
    for (int j = 0; j < NUM_COLORS - 1; ++j) {
        for (int i = 0; i < NUM_FLOORS; ++i) {
            total += euler(s->floors[i][j]);

            bottom_edges >>= (V_SHIFT * (HEIGHT - 1));
            bottom_vertices >>= (V_SHIFT * (HEIGHT - 1));

            puyos_t temp = s->floors[i][j] & TOP;
            int edges = popcount(temp & bottom_edges);
            int vertices = popcount((temp | ((temp & RIGHT_BLOCK) >> H_SHIFT)) & bottom_vertices);
            if (bottom_corner && (temp & 1)) {
                vertices += 1;
            }
            total += edges;
            total -= vertices;

            // This is always broken so leaving the debug print here.
            // printf("double e=%d v=%d\n", edges, vertices);

            bottom_edges = s->floors[i][j] & BOTTOM;
            bottom_vertices = bottom_edges | ((bottom_edges & RIGHT_BLOCK) >> H_SHIFT);
            bottom_corner = bottom_edges & LEFT_WALL;
        }
    }
    return total;
}

int clear_groups(state *s, int chain_number) {
    assert(NUM_FLOORS == 2);
    assert(WIDTH % 2 == 0);
    int num_cleared = 0;
    int group_bonus = 0;
    unsigned char color_flags = 0;
    for (int i = 0; i < NUM_COLORS - 1; ++i) {
        puyos_t top = s->floors[0][i] & LIFE_BLOCK;
        puyos_t bottom = s->floors[1][i];

        for (int j = 0; j < HEIGHT * WIDTH; j += 2) {
            puyos_t top_group = 3ULL << j;
            puyos_t bottom_group = 0;
            puyos_t top_extra = 0;

            top_group = flood(top_group, top);
            top ^= top_group;
            if (top_group & BOTTOM) {
                bottom_group = (top_group & BOTTOM) >> (V_SHIFT * (HEIGHT - 1));
                bottom_group = flood(bottom_group, bottom);
                bottom ^= bottom_group;
                if (bottom_group & TOP) {
                    top_extra = (bottom_group & TOP) << (V_SHIFT * (HEIGHT - 1));
                    top_extra = flood(top_extra, top);
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

                group_size -= CLEAR_THRESHOLD;
                if (group_size >= MAX_GROUP_BONUS) {
                    group_size = MAX_GROUP_BONUS - 1;
                }
                group_bonus += GROUP_BONUS[group_size];
                color_flags |= 1 << i;

                s->floors[0][GARBAGE] &= ~((cross(top_group) & LIFE_BLOCK) | ((bottom_group & TOP) << (V_SHIFT * (HEIGHT - 1))));
                s->floors[1][GARBAGE] &= ~(cross(bottom_group) | ((top_group & BOTTOM) >> (V_SHIFT * (HEIGHT - 1))));
            }
        }
        for (int j = 0; j < HEIGHT * WIDTH; j += 2) {
            puyos_t bottom_group = 3ULL << j;
            bottom_group = flood(bottom_group, bottom);
            bottom ^= bottom_group;
            int group_size = popcount(bottom_group);
            if (group_size >= CLEAR_THRESHOLD) {
                s->floors[1][i] ^= bottom_group;
                num_cleared += group_size;

                group_size -= CLEAR_THRESHOLD;
                if (group_size >= MAX_GROUP_BONUS) {
                    group_size = MAX_GROUP_BONUS - 1;
                }
                group_bonus += GROUP_BONUS[group_size];
                color_flags |= 1 << i;

                s->floors[0][GARBAGE] &= ~((bottom_group & TOP) << (V_SHIFT * (HEIGHT - 1)));
                s->floors[1][GARBAGE] &= ~cross(bottom_group);
            }
        }
    }
    int color_bonus = COLOR_BONUS[popcount(color_flags)];
    if (chain_number >= MAX_CHAIN_POWER) {
        chain_number = MAX_CHAIN_POWER - 1;
    }
    int chain_power = CHAIN_POWERS[chain_number];
    int clear_bonus = chain_power + color_bonus + group_bonus;
    if (clear_bonus < 1) {
        clear_bonus = 1;
    } else if (clear_bonus > MAX_CLEAR_BONUS) {
        clear_bonus = MAX_CLEAR_BONUS;
    }
    return (10 * num_cleared) * clear_bonus;
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

void kill_puyos(state *s) {
    for (int i = 0; i < NUM_COLORS; ++i) {
        s->floors[0][i] &= ~DEATH_BLOCK;
    }
}

int resolve(state *s, int *chain_out) {
    int chain = -1;
    int total_score = 0;
    while(1) {
        ++chain;
        handle_gravity(s);
        kill_puyos(s);
        int score = clear_groups(s, chain);
        if (!score) {
            break;
        }
        total_score += score;
    }
    int all_clear_bonus = 8500;
    for (int i = 0; i < NUM_COLORS; ++i) {
        if (s->floors[NUM_FLOORS - 1][i]) {
            all_clear_bonus = 0;
            break;
        }
    }
    if (chain_out) {
        *chain_out = chain;
    }
    return total_score + all_clear_bonus;
}

#include "tree.c"
#include "demo.c"
#include "test.c"

int main() {
    test_all();
    mc_demo();
    return 0;
}
