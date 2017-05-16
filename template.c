#define NUM_TETROMINOES (19)

puyos_t TETROMINOES[NUM_TETROMINOES] = {
    195,  // O
    387, 4290,  // Z
    198, 8385,  // S
    15, 266305,  // I
    71, 452, 12353, 8323,  // L
    135, 450, 4289, 8386, // T
    263, 449, 4163, 12418,  // J
};

void handle_bottom_gravity(state *s, int num_colors) {
    puyos_t all;
    all = 0;
    puyos_t *floor = s->floors[1];
    for (int j = 0; j < num_colors; ++j) {
        all |= floor[j];
    }

    puyos_t temp;
    do {
        temp = all;
        puyos_t bellow, falling;
        bellow = (all >> V_SHIFT) | BOTTOM;
        all = 0;
        for (int i = 0; i < num_colors; ++i) {
            falling = floor[i] & ~bellow;
            floor[i] = (falling << V_SHIFT) | (floor[i] & ~falling);
            all |= floor[i];
        }
    } while (temp != all);
}

int clear_bottom_groups(state *s, int num_colors) {
    int num_cleared = 0;
    puyos_t *floor = s->floors[1];
    for (int i = 0; i < num_colors; ++i) {
        puyos_t bottom = floor[i];
        for (int j = 0; j < HEIGHT * WIDTH; j += 2) {
            puyos_t bottom_group = 3ULL << j;
            bottom_group = flood(bottom_group, bottom);
            bottom ^= bottom_group;
            int group_size = popcount(bottom_group);
            if (group_size >= CLEAR_THRESHOLD) {
                floor[i] ^= bottom_group;
                num_cleared += group_size;
            }
            if (!bottom) {
                break;
            }
        }
    }
    return num_cleared;
}

int resolve_bottom(state *s, int num_colors) {
    int chain = -1;
    while(1) {
        ++chain;
        handle_bottom_gravity(s, num_colors);
        if (!clear_bottom_groups(s, num_colors)) {
            break;
        }
    }
    return chain;
}

state* chain_of_fours(int num_links) {
    state *s = calloc(1, sizeof(state));
    state *c = calloc(1, sizeof(state));
    int num_colors = num_links;
    if (num_colors > NUM_COLORS - 1) {
        num_colors = NUM_COLORS - 1;
    }

    while (1) {
        clear_state(s);
        // Insert the trigger. Always a tetrominoe.
        // Can be red without loss of generality.
        // Falls into place so doesn't always end up being the trigger.
        while (1) {
            s->floors[1][0] = 0;
            s->floors[1][0] |= TETROMINOES[rand() % NUM_TETROMINOES] << (
                rand() % (WIDTH - 1) +
                (rand() % (HEIGHT - 1)) * V_SHIFT
            );
            if (popcount(s->floors[1][0]) == 4) {
                break;
            }
        }
        puyos_t allowed = FULL ^ s->floors[1][0];
        for (int k = 1; k < num_colors; ++k) {
            int j = 4;
            while (j) {
                puyos_t p = 1ULL << (rand() % (WIDTH * HEIGHT));
                if (p & allowed) {
                    s->floors[1][k] |= p;
                    allowed ^= p;
                    --j;
                }
            }
        }
        int k = num_links - num_colors;
        while (k) {
            int color = rand() % (NUM_COLORS - 1);
            int j = 4;
            while (j) {
                puyos_t p = 1ULL << (rand() % (WIDTH * HEIGHT));
                if (p & allowed) {
                    s->floors[1][color] |= p;
                    allowed ^= p;
                    --j;
                }
            }
            k--;
        }
        memcpy(c, s, sizeof(state));
        int chain = resolve_bottom(c, num_colors);
        if (state_is_clear(c) && chain==num_links) {
            handle_bottom_gravity(s, num_colors);
            break;
        }
    }
    free(c);
    return s;
}
