#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "jkiss/jkiss.h"

#include "puyobot/state.h"

const int COLOR_BONUS[NUM_COLORS] = {0, 0, 3, 6, 12, 24};
const int GROUP_BONUS[NUM_GROUP_BONUS] = {0, 2, 3, 4, 5, 6, 7, 10};
const int CHAIN_POWERS[NUM_CHAIN_POWERS] = {
    0, 8, 16, 32, 64, 96, 128, 160, 192, 224, 256, 288,
    320, 352, 384, 416, 448, 480, 512, 544, 576, 608, 640, 672
};

State* copy_state(State *state) {
    State *copy = malloc(sizeof(State));
    memcpy(copy, state, sizeof(State));
    return copy;
}

void clear_state(State *state) {
    memset(state, 0, sizeof(State));
}

int state_is_clear(State *state) {
    for (int j = 0; j < NUM_FLOORS; ++j) {
        for (int i = 0; i < NUM_COLORS; ++i) {
            if (state->floors[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

void shift_down(State *state) {
    for (int j = 0; j < NUM_COLORS; ++j) {
        puyos_t leak = 0;
        for (int i = 0; i < NUM_FLOORS; ++i) {
            puyos_t temp = state->floors[i][j] & BOTTOM;
            state->floors[i][j] <<= V_SHIFT;
            state->floors[i][j] |= leak >> TOP_TO_BOTTOM;
            state->floors[i][j] &= FULL;
            leak = temp;
        }
    }
}

void print_state(State *state) {
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
                if (p & state->floors[j][k]) {
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

void repr_state(State *state) {
    printf("(state){{");
    for (int i = 0; i < NUM_FLOORS; ++i) {
        printf("{");
        for (int j = 0; j < NUM_COLORS; ++j) {
            printf("%lluull, ", state->floors[i][j]);
        }
        printf("\b\b}, ");
    }
    printf("\b\b}}\n");
}

int state_euler(State *state) {
    int total = 0;
    puyos_t bottom_edges = 0;
    puyos_t bottom_vertices = 0;
    puyos_t bottom_corner = 0;
    for (int j = 0; j < NUM_COLORS - 1; ++j) {
        for (int i = 0; i < NUM_FLOORS; ++i) {
            total += euler(state->floors[i][j]);

            bottom_edges >>= TOP_TO_BOTTOM;
            bottom_vertices >>= TOP_TO_BOTTOM;

            puyos_t temp = state->floors[i][j] & TOP;
            int edges = popcount(temp & bottom_edges);
            int vertices = popcount((temp | ((temp & RIGHT_BLOCK) >> H_SHIFT)) & bottom_vertices);
            if (bottom_corner && (temp & 1)) {
                vertices += 1;
            }
            total += edges;
            total -= vertices;

            // This is always broken so leaving the debug print here.
            // printf("double e=%d v=%d\n", edges, vertices);

            bottom_edges = state->floors[i][j] & BOTTOM;
            bottom_vertices = bottom_edges | ((bottom_edges & RIGHT_BLOCK) >> H_SHIFT);
            bottom_corner = bottom_edges & LEFT_WALL;
        }
    }
    return total;
}

int state_popcount(State *state) {
    int total = 0;
    for (int i = 0; i < NUM_FLOORS; ++i) {
        for (int j = 0; j < NUM_COLORS; ++j) {
            total += popcount(state->floors[i][j]);
        }
    }
    return total;
}

int clear_groups(State *state, int chain_number) {
    assert(NUM_FLOORS == 2);
    assert(WIDTH % 2 == 0);
    int num_cleared = 0;
    int group_bonus = 0;
    unsigned char color_flags = 0;
    for (int i = 0; i < NUM_COLORS - 1; ++i) {
        puyos_t top[2] = {state->floors[0][i] & LIFE_BLOCK, state->floors[1][i]};

        for (int j = HEIGHT * WIDTH - 2; j >= (GHOST_Y + 1) * WIDTH; j -= 2) {
            puyos_t top_group[2] = {3ULL << j, 0};
            flood_2(top_group, top);
            top[0] ^= top_group[0];
            top[1] ^= top_group[1];
            int group_size = popcount_2(top_group);
            if (group_size >= CLEAR_THRESHOLD) {
                state->floors[0][i] ^= top_group[0];
                state->floors[1][i] ^= top_group[1];
                num_cleared += group_size;

                group_size -= CLEAR_THRESHOLD;
                if (group_size >= NUM_GROUP_BONUS) {
                    group_size = NUM_GROUP_BONUS - 1;
                }
                group_bonus += GROUP_BONUS[group_size];
                color_flags |= 1 << i;

                state->floors[0][GARBAGE] &= ~((cross(top_group[0]) & LIFE_BLOCK) | ((top_group[1] & TOP) << TOP_TO_BOTTOM));
                state->floors[1][GARBAGE] &= ~(cross(top_group[1]) | ((top_group[0] & BOTTOM) >> TOP_TO_BOTTOM));
            }
            if (!top[0]) {
                break;
            }
        }
        puyos_t bottom = state->floors[1][i];
        for (int j = HEIGHT * WIDTH - 2; j >= 0; j -= 2) {
            puyos_t bottom_group = 3ULL << j;
            bottom_group = flood(bottom_group, bottom);
            if (!bottom_group) {
                continue;
            }
            bottom ^= bottom_group;
            int group_size = popcount(bottom_group);
            if (group_size >= CLEAR_THRESHOLD) {
                state->floors[1][i] ^= bottom_group;
                num_cleared += group_size;

                group_size -= CLEAR_THRESHOLD;
                if (group_size >= NUM_GROUP_BONUS) {
                    group_size = NUM_GROUP_BONUS - 1;
                }
                group_bonus += GROUP_BONUS[group_size];
                color_flags |= 1 << i;

                state->floors[0][GARBAGE] &= ~((bottom_group & TOP) << TOP_TO_BOTTOM);
                state->floors[1][GARBAGE] &= ~cross(bottom_group);
            }
            if (!bottom) {
                break;
            }
        }
    }
    int color_bonus = COLOR_BONUS[popcount(color_flags)];
    if (chain_number >= NUM_CHAIN_POWERS) {
        chain_number = NUM_CHAIN_POWERS - 1;
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

int handle_gravity(State *state) {
    assert(NUM_FLOORS == 2);
    puyos_t all[2];
    for (int i = 0; i < NUM_FLOORS; ++i) {
        all[i] = 0;
        for (int j = 0; j < NUM_COLORS; ++j) {
            all[i] |= state->floors[i][j];
        }
    }

    int iterations = 0;
    puyos_t temp[2];
    do {
        temp[0] = all[0];
        temp[1] = all[1];
        puyos_t bellow, falling;
        bellow = (all[1] >> V_SHIFT) | BOTTOM;
        all[1] = 0;
        for (int i = 0; i < NUM_COLORS; ++i) {
            falling = state->floors[1][i] & ~bellow;
            state->floors[1][i] = (falling << V_SHIFT) | (state->floors[1][i] & bellow);
            all[1] |= state->floors[1][i];
        }

        bellow = (all[0] >> V_SHIFT) | ((all[1] & TOP) << TOP_TO_BOTTOM);
        all[0] = 0;
        for (int i = 0; i < NUM_COLORS; ++i) {
            falling = state->floors[0][i] & ~bellow;

            state->floors[1][i] |= (falling & BOTTOM) >> TOP_TO_BOTTOM;

            state->floors[0][i] = (falling << V_SHIFT) | (state->floors[0][i] & bellow);
            all[0] |= state->floors[0][i];
        }
        ++iterations;
    } while (temp[0] != all[0] || temp[1] != all[1]);
    return iterations;
}

void kill_puyos(State *state) {
    for (int i = 0; i < NUM_COLORS; ++i) {
        state->floors[0][i] &= ~DEATH_BLOCK;
    }
}

int resolve(State *state, int *chain_out) {
    int chain = -1;
    int total_score = 0;
    while(1) {
        ++chain;
        int iterations = handle_gravity(state);
        if (iterations == 1 && chain > 0) {
            break;
        }
        kill_puyos(state);
        int score = clear_groups(state, chain);
        if (!score) {
            break;
        }
        total_score += score;
    }
    int all_clear_bonus = 0;
    if (total_score) {
        all_clear_bonus = 8500;
        for (int i = 0; i < NUM_COLORS; ++i) {
            if (state->floors[NUM_FLOORS - 1][i]) {
                all_clear_bonus = 0;
                break;
            }
        }
    }
    if (chain_out) {
        *chain_out = chain;
    }
    return total_score + all_clear_bonus;
}

void get_state_mask(State *state, puyos_t *out) {
    for (int i = 0; i < NUM_FLOORS; ++i) {
        out[i] = 0;
        for (int j = 0; j < NUM_COLORS; ++j) {
            out[i] |= state->floors[i][j];
        }
    }
}

int state_is_full(State *state) {
    puyos_t all = 0;
    for (int i = 0; i < NUM_COLORS; ++i) {
        all |= state->floors[0][i];
    }
    return !(GHOST_LINE & ~all);
}

void assert_sanity(State *state) {
    for (int j = 0; j < NUM_FLOORS; ++j) {
        for (int i = 0; i < NUM_COLORS; ++i) {
            assert(!(state->floors[j][i] & ~FULL));
            for (int k = 0; k < NUM_COLORS; ++k) {
                if (k == i) {
                    continue;
                }
                assert(!(state->floors[j][i] & state->floors[j][k]));
            }
        }
    }
}

void blast_state(State *state, int num_shots) {
    puyos_t all[NUM_FLOORS];
    get_state_mask(state, all);
    puyos_t allowed[2] = {FULL &~ DEATH_BLOCK & ~all[0], FULL & ~all[1]};
    while (num_shots && (allowed[0] || allowed[1])) {
        puyos_t p = 1ULL << (jrand() % (WIDTH * HEIGHT));
        int i = jrand() % NUM_FLOORS;
        int k = jrand() % NUM_DEAL_COLORS;
        if (p & allowed[i]) {
            state->floors[i][k] |= p;
            allowed[i] ^= p;
            num_shots--;
        }
    }
}

int apply_deal_and_choice(State *state, content_t deal, content_t choice) {
    if (choice == CHOICE_PASS) {
        return 0;
    }
    content_t color1 = deal & COLOR1_MASK;
    content_t color2 = deal >> COLOR2_SHIFT;
    content_t orientation = choice & ~CHOICE_X_MASK;
    content_t color1_x = choice & CHOICE_X_MASK;
    content_t color2_x = color1_x;
    content_t color1_y = 0;
    content_t color2_y = 1;
    if (orientation == CHOICE_90) {
        color2_x++;
        color2_y--;
    } else if (orientation == CHOICE_180) {
        color1_y++;
        color2_y--;
    } else if (orientation == CHOICE_270) {
        color1_x++;
        color2_y--;
    }
    puyos_t all = 0;
    for (int i = 0; i < NUM_COLORS; ++i) {
        all |= state->floors[0][i];
    }

    if (
        ((1ULL << (color1_x + V_SHIFT * GHOST_Y)) & all) &&
        ((1ULL << (color2_x + V_SHIFT * GHOST_Y)) & all)
    ) {
        return 0;
    }

    puyos_t puyo1 = 1ULL << (color1_x + V_SHIFT * color1_y);
    state->floors[0][color1] |= puyo1;
    puyos_t puyo2 = 1ULL << (color2_x + V_SHIFT * color2_y);
    state->floors[0][color2] |= puyo2;

    return 1;
}

void clear_deal_and_choice(State *state) {
    for (int i = 0; i < NUM_COLORS; ++i) {
        state->floors[0][i] &= ~(TOP | (TOP << V_SHIFT));
    }
}

int step_state(void *s, content_t deal, content_t choice, double *score) {
    int valid = apply_deal_and_choice(s, deal, choice);
    if (!valid) {
        return 0;
    }
    *score = resolve(s, NULL);
    return 1;
}

void *_copy_state(void *s) {
    return copy_state(s);
}

SearchOptions simple_search_options(eval_fun eval, int depth, double tree_factor) {
    return (SearchOptions) {
        .copy = _copy_state,
        .delete = free,
        .step = step_state,
        .eval = eval,
        .depth = depth,
        .tree_factor = tree_factor,
        .choice_sets = NULL,
    };
}
