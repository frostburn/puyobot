typedef struct state
{
    puyos_t floors[NUM_FLOORS][NUM_COLORS];
} state;

state* copy_state(state *s) {
    state *c = malloc(sizeof(state));
    memcpy(c, s, sizeof(state));
    return c;
}

void clear_state(state *s) {
    memset(s, 0, sizeof(state));
}

int state_is_clear(state *s) {
    for (int j = 0; j < NUM_FLOORS; ++j) {
        for (int i = 0; i < NUM_COLORS; ++i) {
            if (s->floors[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

void shift_down(state *s) {
    for (int j = 0; j < NUM_COLORS; ++j) {
        puyos_t leak = 0;
        for (int i = 0; i < NUM_FLOORS; ++i) {
            puyos_t temp = s->floors[i][j] & BOTTOM;
            s->floors[i][j] <<= V_SHIFT;
            s->floors[i][j] |= leak >> TOP_TO_BOTTOM;
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

void repr_state(state *s) {
    printf("(state){{");
    for (int i = 0; i < NUM_FLOORS; ++i) {
        printf("{");
        for (int j = 0; j < NUM_COLORS; ++j) {
            printf("%lluull, ", s->floors[i][j]);
        }
        printf("\b\b}, ");
    }
    printf("\b\b}}\n");
}

int state_euler(state *s) {
    int total = 0;
    puyos_t bottom_edges = 0;
    puyos_t bottom_vertices = 0;
    puyos_t bottom_corner = 0;
    for (int j = 0; j < NUM_COLORS - 1; ++j) {
        for (int i = 0; i < NUM_FLOORS; ++i) {
            total += euler(s->floors[i][j]);

            bottom_edges >>= TOP_TO_BOTTOM;
            bottom_vertices >>= TOP_TO_BOTTOM;

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

int state_popcount(state *s) {
    int total = 0;
    for (int i = 0; i < NUM_FLOORS; ++i) {
        for (int j = 0; j < NUM_COLORS; ++j) {
            total += popcount(s->floors[i][j]);
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
        puyos_t top[2] = {s->floors[0][i] & LIFE_BLOCK, s->floors[1][i]};

        for (int j = HEIGHT * WIDTH - 2; j >= (GHOST_Y + 1) * WIDTH; j -= 2) {
            puyos_t top_group[2] = {3ULL << j, 0};
            flood_2(top_group, top);
            top[0] ^= top_group[0];
            top[1] ^= top_group[1];
            int group_size = popcount_2(top_group);
            if (group_size >= CLEAR_THRESHOLD) {
                s->floors[0][i] ^= top_group[0];
                s->floors[1][i] ^= top_group[1];
                num_cleared += group_size;

                group_size -= CLEAR_THRESHOLD;
                if (group_size >= NUM_GROUP_BONUS) {
                    group_size = NUM_GROUP_BONUS - 1;
                }
                group_bonus += GROUP_BONUS[group_size];
                color_flags |= 1 << i;

                s->floors[0][GARBAGE] &= ~((cross(top_group[0]) & LIFE_BLOCK) | ((top_group[1] & TOP) << TOP_TO_BOTTOM));
                s->floors[1][GARBAGE] &= ~(cross(top_group[1]) | ((top_group[0] & BOTTOM) >> TOP_TO_BOTTOM));
            }
            if (!top[0]) {
                break;
            }
        }
        puyos_t bottom = s->floors[1][i];
        for (int j = HEIGHT * WIDTH - 2; j >= 0; j -= 2) {
            puyos_t bottom_group = 3ULL << j;
            bottom_group = flood(bottom_group, bottom);
            if (!bottom_group) {
                continue;
            }
            bottom ^= bottom_group;
            int group_size = popcount(bottom_group);
            if (group_size >= CLEAR_THRESHOLD) {
                s->floors[1][i] ^= bottom_group;
                num_cleared += group_size;

                group_size -= CLEAR_THRESHOLD;
                if (group_size >= NUM_GROUP_BONUS) {
                    group_size = NUM_GROUP_BONUS - 1;
                }
                group_bonus += GROUP_BONUS[group_size];
                color_flags |= 1 << i;

                s->floors[0][GARBAGE] &= ~((bottom_group & TOP) << TOP_TO_BOTTOM);
                s->floors[1][GARBAGE] &= ~cross(bottom_group);
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

int handle_gravity(state *s) {
    assert(NUM_FLOORS == 2);
    puyos_t all[2];
    for (int i = 0; i < NUM_FLOORS; ++i) {
        all[i] = 0;
        for (int j = 0; j < NUM_COLORS; ++j) {
            all[i] |= s->floors[i][j];
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
            falling = s->floors[1][i] & ~bellow;
            s->floors[1][i] = (falling << V_SHIFT) | (s->floors[1][i] & bellow);
            all[1] |= s->floors[1][i];
        }

        bellow = (all[0] >> V_SHIFT) | ((all[1] & TOP) << TOP_TO_BOTTOM);
        all[0] = 0;
        for (int i = 0; i < NUM_COLORS; ++i) {
            falling = s->floors[0][i] & ~bellow;

            s->floors[1][i] |= (falling & BOTTOM) >> TOP_TO_BOTTOM;

            s->floors[0][i] = (falling << V_SHIFT) | (s->floors[0][i] & bellow);
            all[0] |= s->floors[0][i];
        }
        ++iterations;
    } while (temp[0] != all[0] || temp[1] != all[1]);
    return iterations;
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
        int iterations = handle_gravity(s);
        if (iterations == 1 && chain > 0) {
            break;
        }
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

void get_state_mask(state *s, puyos_t *out) {
    for (int i = 0; i < NUM_FLOORS; ++i) {
        out[i] = 0;
        for (int j = 0; j < NUM_COLORS; ++j) {
            out[i] |= s->floors[i][j];
        }
    }
}

int state_is_full(state *s) {
    puyos_t all = 0;
    for (int i = 0; i < NUM_COLORS; ++i) {
        all |= s->floors[0][i];
    }
    return !(GHOST_LINE & ~all);
}

void assert_sanity(state *s) {
    for (int j = 0; j < NUM_FLOORS; ++j) {
        for (int i = 0; i < NUM_COLORS; ++i) {
            assert(!(s->floors[j][i] & ~FULL));
            for (int k = 0; k < NUM_COLORS; ++k) {
                if (k == i) {
                    continue;
                }
                assert(!(s->floors[j][i] & s->floors[j][k]));
            }
        }
    }
}

void blast_state(state *s, int num_shots) {
    puyos_t all[NUM_FLOORS];
    get_state_mask(s, all);
    puyos_t allowed[2] = {FULL &~ DEATH_BLOCK & ~all[0], FULL & ~all[1]};
    while (num_shots && (allowed[0] || allowed[1])) {
        puyos_t p = 1ULL << (jrand() % (WIDTH * HEIGHT));
        int i = jrand() % NUM_FLOORS;
        int k = jrand() % NUM_DEAL_COLORS;
        if (p & allowed[i]) {
            s->floors[i][k] |= p;
            allowed[i] ^= p;
            num_shots--;
        }
    }
}
