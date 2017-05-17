#define NUM_TETROMINOES (19)

puyos_t TETROMINOES[NUM_TETROMINOES] = {
    195,  // O
    387, 4290,  // Z
    198, 8385,  // S
    15, 266305,  // I
    71, 452, 8323, 12353,  // L
    135, 450, 4289, 8386,  // T
    263, 449, 4163, 12418,  // J
};

int TETROMINO_DIMS[NUM_TETROMINOES][2] = {
    {2, 2},
    {3, 2}, {2, 3},
    {3, 2}, {2, 3},
    {4, 1}, {1, 4},
    {3, 2}, {3, 2}, {2, 3}, {2, 3},
    {3, 2}, {3, 2}, {2, 3}, {2, 3},
    {3, 2}, {3, 2}, {2, 3}, {2, 3},
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

int has_gap(puyos_t puyos) {
    puyos = beam_up(puyos);
    // Check if there is a gap.
    int run = 0;
    int gap = 0;
    for (int i = 0; i < WIDTH; ++i) {
        puyos_t probe = 1ULL << i;
        if (probe & puyos) {
            run = 1;
            if (gap) {
                return 1;
            }
        } else if (run) {
            gap = 1;
        }
    }
    return 0;
}

state* chain_of_fours(int num_links) {
    assert(WIDTH == 6);
    assert(NUM_FLOORS == 2);
    state *s = calloc(1, sizeof(state));
    state *c = calloc(1, sizeof(state));
    int num_colors = num_links;
    if (num_colors > NUM_COLORS - 1) {
        num_colors = NUM_COLORS - 1;
    }

    while (1) {
        clear_state(s);
        puyos_t *floor = s->floors[1];
        // Insert the trigger. Always a tetrominoe.
        // Can be red without loss of generality.
        // Falls into place so doesn't always end up being the trigger.
        floor[0] = 0;
        int i = rand() % NUM_TETROMINOES;
        floor[0] |= TETROMINOES[i] << (
            rand() % (WIDTH - TETROMINO_DIMS[i][0]) +
            (rand() % (HEIGHT - TETROMINO_DIMS[i][1])) * V_SHIFT
        );
        puyos_t allowed = FULL ^ floor[0];
        for (int k = 1; k < num_colors; ++k) {
            int j = 4;
            while (j) {
                puyos_t p = 1ULL << (rand() % (WIDTH * HEIGHT));
                if (p & allowed) {
                    floor[k] |= p;
                    allowed ^= p;
                    --j;
                }
            }
            if (has_gap(floor[k])) {
                allowed ^= floor[k];
                floor[k] = 0;
                --k;
                continue;
            }
        }
        int k = num_links - num_colors;
        while (k) {
            int color = rand() % (NUM_COLORS - 1);
            int j = 4;
            while (j) {
                puyos_t p = 1ULL << (rand() % (WIDTH * HEIGHT));
                if (p & allowed) {
                    floor[color] |= p;
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

int expand_chain(state *s) {
    assert(NUM_FLOORS == 2);
    assert(NUM_COLORS == 6);
    if (state_is_clear(s)) {
        int i = rand() % NUM_TETROMINOES;
        s->floors[1][0] = TETROMINOES[i] << (rand() % (WIDTH - TETROMINO_DIMS[i][0]));
        handle_bottom_gravity(s, 1);
        return 1;
    }

    state *c = copy_state(s);
    clear_groups(c, 0);
    puyos_t trigger[2] = {0, 0};
    for (int i = 0; i < NUM_COLORS - 1; ++i) {
        for (int j = 0; j < NUM_FLOORS; ++j) {
            trigger[j] |= s->floors[j][i] ^ c->floors[j][i];
        }
    }
    if (!(trigger[0] || trigger[1])) {
        free(c);
        return 0;
    }
    memcpy(c, s, sizeof(state));
    int chain;
    resolve(c, &chain);
    beam_down_2(trigger);

    static puyos_t _tetrominoes[2 * 897];
    static int n;
    if (!_tetrominoes[0]) {
        n = 0;
        for (int i = 0; i < NUM_TETROMINOES; ++i) {
            for (int j = 0; j <= WIDTH - TETROMINO_DIMS[i][0]; ++j) {
                for (int k = 0; k <= LIFE_HEIGHT - TETROMINO_DIMS[i][1]; ++k) {
                    _tetrominoes[2*n] = TETROMINOES[i];
                    _tetrominoes[2*n + 1] = 0;
                    translate_2(_tetrominoes + 2*n, j, k + GHOST_Y);
                    ++n;

                }
            }
        }
    }
    puyos_t *tetrominoes = malloc(2*n * sizeof(puyos_t));
    memcpy(tetrominoes, _tetrominoes, 2*n * sizeof(puyos_t));
    shuffle_2(tetrominoes, n);
    puyos_t colors[NUM_COLORS - 1] = {RED, GREEN, YELLOW, BLUE, PURPLE};
    shuffle(colors, NUM_COLORS - 1);

    state *cc = malloc(sizeof(state));
    for (int i = 0; i < n; ++i) {
        if ((tetrominoes[2*i] & trigger[0]) || (tetrominoes[2*i + 1] & trigger[1])) {
            puyos_t tetromino[2] = {tetrominoes[2*i], tetrominoes[2*i + 1]};
            puyos_t lifter[2] = {tetromino[0], tetromino[1]};
            memcpy(c, s, sizeof(state));
            while (lifter[0] || lifter[1]) {
                puyos_t lift[2] = {lifter[0], lifter[1]};
                beam_up_2(lift);
                for (int j = 0; j < NUM_COLORS; ++j) {
                    puyos_t p[2] = {c->floors[0][j], c->floors[1][j]};
                    c->floors[0][j] = (p[0] & ~lift[0]) | ((p[1] & TOP & lift[1]) << TOP_TO_BOTTOM) | ((p[0] & lift[0]) >> V_SHIFT);
                    c->floors[1][j] = (p[1] & ~lift[1]) | ((p[1] & lift[1]) >> V_SHIFT);
                }
                lifter[0] &= (lifter[0] >> V_SHIFT) | ((lifter[1] & TOP) << TOP_TO_BOTTOM);
                lifter[1] &= lifter[1] >> V_SHIFT;
            }
            for (int k = 0; k < NUM_COLORS - 1; ++k) {
                int j = colors[k];
                memcpy(cc, c, sizeof(state));
                cc->floors[0][j] |= tetromino[0];
                cc->floors[1][j] |= tetromino[1];
                int new_chain;
                resolve(cc, &new_chain);
                if (new_chain > chain) {
                    c->floors[0][j] |= tetromino[0];
                    c->floors[1][j] |= tetromino[1];
                    memcpy(s, c, sizeof(state));
                    handle_gravity(s);
                    free(c);
                    free(tetrominoes);
                    free(cc);
                    return 1;
                }
            }

        }
    }
    free(c);
    free(tetrominoes);
    free(cc);
    return 0;
}
