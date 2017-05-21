#define NUM_TETROMINOES (19)
#define SHOT_PATIENCE (100000)
#define CHAIN_PATIENCE (100 * SHOT_PATIENCE)

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

typedef struct template_result
{
    int target_shots;
    int target_chain;
    int extra_shots;
    int chain;
    int score;
    int puyos_remaining;
    size_t iterations;
} template_result;

typedef struct bottom_template
{
    puyos_t *floor;
    int num_links;
    int num_colors;
    char *conflicts;
} bottom_template;


void print_template_result(template_result result) {
    printf(
        "target shots=%d\ntarget chain=%d\nextra shots=%d\nchain=%d\nscore=%d\npuyos remaining=%d\niterations=%zu\n",
        result.target_shots,
        result.target_chain,
        result.extra_shots,
        result.chain,
        result.score,
        result.puyos_remaining,
        result.iterations
    );
}

#include "bottom.c"

bottom_template* bottom_chain_of_fours(int num_links) {
    puyos_t *floor = malloc(num_links * sizeof(puyos_t));
    puyos_t *temp = malloc(num_links * sizeof(puyos_t));
    int *color_order = malloc(num_links * sizeof(int));
    while (1) {
        memset(floor, 0, num_links * sizeof(puyos_t));
        // Insert the trigger. Always a tetrominoe.
        // Can be red without loss of generality.
        // Falls into place so doesn't always end up being the trigger.
        int i = jrand() % NUM_TETROMINOES;
        floor[0] |= TETROMINOES[i] << (
            jrand() % (WIDTH - TETROMINO_DIMS[i][0]) +
            (jrand() % (HEIGHT - TETROMINO_DIMS[i][1])) * V_SHIFT
        );
        puyos_t allowed = FULL ^ floor[0];
        for (int k = 1; k < num_links; ++k) {
            int j = CLEAR_THRESHOLD;
            while (j) {
                puyos_t p = 1ULL << (jrand() % (WIDTH * HEIGHT));
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
        memcpy(temp, floor, num_links * sizeof(puyos_t));
        int chain = resolve_bottom(temp, num_links, color_order);
        if (chain == num_links) {
            handle_bottom_gravity(floor, num_links);
            memcpy(temp, floor, num_links * sizeof(puyos_t));
            for (int i = 0; i < num_links; ++i) {
                floor[i] = temp[color_order[i]];
            }
            break;
        }
    }
    free(temp);
    bottom_template *template = calloc(1, sizeof(bottom_template));
    template->floor = floor;
    template->num_links = num_links;
    template->num_colors = num_links;
    return template;
}

int extend_bottom_chain(bottom_template *template) {
    int num_colors = template->num_colors;
    puyos_t *floor = template->floor;
    if (!num_colors) {
        int i = jrand() % NUM_TETROMINOES;
        template->num_colors = 1;
        template->num_links = 1;
        template->floor = malloc(sizeof(puyos_t));
        template->floor[0] = TETROMINOES[i] << (jrand() % (WIDTH - TETROMINO_DIMS[i][0]));
        handle_bottom_gravity(template->floor, 1);
        return 1;
    }

    puyos_t *temp = malloc(num_colors * sizeof(puyos_t));
    memcpy(temp, floor, num_colors * sizeof(puyos_t));
    int color_cleared;
    clear_bottom_groups(temp, num_colors, &color_cleared);
    puyos_t trigger = 0;
    for (int i = 0; i < num_colors; ++i) {
        if (floor[i] != temp[i]) {
            trigger = floor[i] ^ temp[i];
            break;
        }
    }
    if (!trigger) {
        free(temp);
        return 0;
    }
    memcpy(temp, floor, num_colors * sizeof(puyos_t));
    int chain = resolve_bottom(temp, num_colors, NULL);
    trigger = beam_down(trigger);

    static puyos_t _tetrominoes[725];
    static int n;
    if (!_tetrominoes[0]) {
        n = 0;
        for (int i = 0; i < NUM_TETROMINOES; ++i) {
            for (int j = 0; j <= WIDTH - TETROMINO_DIMS[i][0]; ++j) {
                for (int k = 0; k <= HEIGHT - TETROMINO_DIMS[i][1]; ++k) {
                    _tetrominoes[n++] = TETROMINOES[i] << (j + k * V_SHIFT);
                }
            }
        }
    }
    puyos_t *tetrominoes = malloc(n * sizeof(puyos_t));
    memcpy(tetrominoes, _tetrominoes, n * sizeof(puyos_t));
    shuffle(tetrominoes, n);

    puyos_t *temp2 = malloc((num_colors + 1) * sizeof(puyos_t));
    for (int i = 0; i < n; ++i) {
        if (tetrominoes[i] & trigger) {
            puyos_t tetromino = tetrominoes[i];
            puyos_t lifter = tetromino;
            memcpy(temp, floor, num_colors * sizeof(puyos_t));
            while (lifter) {
                puyos_t lift = beam_up(tetromino);
                for (int j = 0; j < num_colors; ++j) {
                    puyos_t p = temp[j];
                    temp[j] = (p & ~lift) | ((p & lift) >> V_SHIFT);
                }
                lifter &= lifter >> V_SHIFT;
            }
            memcpy(temp2 + 1, temp, num_colors * sizeof(puyos_t));
            temp2[0] = tetromino;
            int new_chain = resolve_bottom(temp2, num_colors + 1, NULL);
            if (new_chain > chain) {
                memcpy(temp2 + 1, temp, num_colors * sizeof(puyos_t));
                temp2[0] = tetromino;
                handle_bottom_gravity(temp2, num_colors + 1);
                free(template->floor);
                template->floor = temp2;
                ++template->num_colors;
                template->num_links = new_chain;
                free(tetrominoes);
                free(temp);
                return 1;
            }

        }
    }
    free(temp);
    free(tetrominoes);
    free(temp2);
    return 0;
}

int spam_bottom(bottom_template *template) {
    puyos_t *floor = template->floor;
    int num_colors = template->num_colors;
    handle_bottom_gravity(floor, num_colors);
    puyos_t all = 0;
    for (int i = 0; i < num_colors; ++i) {
        all |= floor[i];
    }
    int free_space = WIDTH * HEIGHT - popcount(all);
    int j = num_colors;
    template->num_colors += free_space;
    floor = realloc(floor, template->num_colors * sizeof(puyos_t));
    template->floor = floor;
    for (int i = 0; i < WIDTH * HEIGHT; ++i) {
        puyos_t p = 1ULL << i;
        if (!(p & all)) {
            floor[j++] = p;
        }
    }
    return free_space;
}

int sprinkle_bottom(bottom_template *template) {
    puyos_t *floor = template->floor;
    int num_colors = template->num_colors;
    handle_bottom_gravity(floor, num_colors);
    puyos_t all = 0;
    for (int i = 0; i < num_colors; ++i) {
        all |= floor[i];
    }
    int free_space = popcount(TOP & ~all);
    int j = num_colors;
    template->num_colors += free_space;
    floor = realloc(floor, template->num_colors * sizeof(puyos_t));
    template->floor = floor;
    for (int i = 0; i < WIDTH; ++i) {
        puyos_t p = 1ULL << i;
        if (!(p & all)) {
            floor[j++] = p;
        }
    }
    handle_bottom_gravity(floor, template->num_colors);
    return free_space;
}

int _assign(bottom_template *template, int *assignments, int index, int num) {
    int num_colors = template->num_colors;
    if (index >= num_colors) {
        for (int i = 0; i < num_colors; ++i) {
            for (int j = 0; j < num_colors; ++j) {
                if (assignments[i] == assignments[j] && template->conflicts[i + j * num_colors]) {
                    return 0;
                }
            }
        }
        // Need to double check in case of higher level correlations.
        puyos_t *temp = calloc(num, sizeof(puyos_t));
        for (int i = 0; i < template->num_colors; ++i) {
            temp[assignments[i]] |= template->floor[i];
        }
        int new_chain = resolve_bottom(temp, num, NULL);
        free(temp);
        return new_chain >= template->num_links;
    }
    for (int i = 0; i < num; ++i) {
        assignments[index] = i;
        int valid = _assign(template, assignments, index + 1, num);
        if (valid) {
            return 1;
        }
    }
    return 0;
}

int* minimum_assignments(bottom_template *template, int *min_colors) {
    int *assignments = malloc(template->num_colors * sizeof(int));
    for (int num = 1; num <= template->num_colors; ++num) {
        assignments[0] = 0;
        if (_assign(template, assignments, 1, num)) {
            *min_colors = num;
            return assignments;
        }
    }
    free(assignments);
    return NULL;
}

state* state_from_bottom(bottom_template *template) {
    assert(template->conflicts);
    state *s= calloc(1, sizeof(state));
    if (!template->num_colors) {
        return s;
    }
    int num = template->num_colors;
    int *assignments = minimum_assignments(template, &num);
    if (num > NUM_COLORS - 1) {
        free(assignments);
        return NULL;
    }
    for (int i = 0; i < template->num_colors; ++i) {
        s->floors[1][assignments[i]] |= template->floor[i];
    }
    free(assignments);
    return s;
}

unsigned char find_trigger(state *s, puyos_t *out) {
    unsigned char color_flags = 0;
    out[0] = 0;
    out[1] = 0;
    state *c = copy_state(s);
    clear_groups(c, 0);
    for (int i = 0; i < NUM_COLORS - 1; ++i) {
        for (int j = 0; j < NUM_FLOORS; ++j) {
            out[j] |= s->floors[j][i] ^ c->floors[j][i];
            if (out[j]) {
                color_flags |= 1 << i;
            }
        }
    }
    free(c);
    return color_flags;
}

int extend_chain(state *s, puyos_t *fixed) {
    assert(NUM_FLOORS == 2);
    assert(NUM_COLORS == 6);
    if (state_is_clear(s)) {
        int i = jrand() % NUM_TETROMINOES;
        s->floors[1][0] = TETROMINOES[i] << (jrand() % (WIDTH - TETROMINO_DIMS[i][0]));
        handle_gravity(s);
        return 1;
    }

    puyos_t trigger[2];
    if (!find_trigger(s, trigger)) {
        return 0;
    }
    state *c = copy_state(s);
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
        puyos_t tetromino[2] = {tetrominoes[2*i], tetrominoes[2*i + 1]};
        if (fixed && ((tetromino[0] & fixed[0]) || (tetromino[1] & fixed[1]))) {
            continue;
        }
        if ((tetromino[0] & trigger[0]) || (tetromino[1] & trigger[1])) {
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

template_result _chainify_cleanup(state *s, puyos_t *shots, template_result result) {
    assert(shots[0] || shots[1]);
    state *c = malloc(sizeof(state));
    for (int k = 0; k < NUM_FLOORS; ++k) {
        for (int j = 0; j < WIDTH * HEIGHT; ++j) {
            puyos_t p = 1ULL << j;
            if (!(p & shots[k])) {
                continue;
            }
            memcpy(c, s, sizeof(state));
            for (int i = 0; i < NUM_COLORS; ++i) {
                c->floors[k][i] &= ~p;
            }
            int chain;
            resolve(c, &chain);
            if (chain >= result.chain) {
                result.chain = chain;
                for (int i = 0; i < NUM_COLORS; ++i) {
                    s->floors[k][i] &= ~p;
                }
                --result.extra_shots;
            }
        }
    }

    memcpy(c, s, sizeof(state));
    result.score = resolve(c, NULL);
    result.puyos_remaining = 0;
    for (int i = 0; i < NUM_COLORS; ++i) {
        int count = 0;
        for (int j = 0; j < NUM_FLOORS; ++j) {
            count += popcount(c->floors[j][i]);
        }
        result.puyos_remaining += count;
    }
    free(c);
    return result;
}

template_result chainify(state *s, size_t shot_patience, size_t chain_patience) {
    template_result result;
    int popcounts[NUM_COLORS - 1];
    int target_shotcounts[NUM_COLORS - 1];
    int shotcounts[NUM_COLORS - 1];
    int target_chain = 0;
    puyos_t all[NUM_FLOORS];
    result.target_shots = 0;
    result.extra_shots = 0;
    get_state_mask(s, all);
    for (int i = 0; i < NUM_COLORS - 1; ++i) {
        popcounts[i] = 0;
        for (int j = 0; j < NUM_FLOORS; ++j) {
            popcounts[i] += popcount(s->floors[j][i]);
        }
        int chain = ceil_div(popcounts[i], CLEAR_THRESHOLD);
        if (popcounts[i] > 1) {
            target_chain += chain;
            target_shotcounts[i] = chain * CLEAR_THRESHOLD - popcounts[i];
            result.target_shots += target_shotcounts[i];
        } else {
            target_shotcounts[i] = 0;
        }
        shotcounts[i] = target_shotcounts[i];
    }
    result.target_chain = target_chain;
    int free_space = popcount(FULL & ~all[1]) + popcount(FULL & ~DEATH_BLOCK & ~all[0]);

    if (target_chain == 0) {
        shotcounts[0] = CLEAR_THRESHOLD;
        target_shotcounts[0] = CLEAR_THRESHOLD;
        result.target_shots = CLEAR_THRESHOLD;
        result.target_chain = 1;
        target_chain = 1;
    }
    while (result.target_shots == 0) {
        int i = jrand() % (NUM_COLORS - 1);
        if (popcounts[i] > 0) {
            ++shotcounts[i];
            ++target_shotcounts[i];
            ++result.target_shots;
            ++result.target_chain;
            ++target_chain;
            break;
        }
    }
    while (result.target_shots > free_space) {
        int i = jrand() % (NUM_COLORS - 1);
        if (shotcounts[i] > 0) {
            --shotcounts[i];
            --target_shotcounts[i];
            --result.target_shots;
        }
    }

    state *c = malloc(sizeof(state));
    state *cc = malloc(sizeof(state));
    size_t iteration = 1;
    while (1) {
        memcpy(c, s, sizeof(state));
        puyos_t allowed[2] = {FULL &~ DEATH_BLOCK & ~all[0], FULL & ~all[1]};
        puyos_t shots[2] = {0, 0};
        for (int k = 0; k < NUM_COLORS - 1; ++k) {
            int j = shotcounts[k];
            while (j) {
                puyos_t p = 1ULL << (jrand() % (WIDTH * HEIGHT));
                int i = jrand() % NUM_FLOORS;
                if (p & allowed[i]) {
                    c->floors[i][k] |= p;
                    allowed[i] ^= p;
                    shots[i] |= p;
                    --j;
                }
            }
        }
        memcpy(cc, c, sizeof(state));
        int chain;
        int score = resolve(cc, &chain);
        if (chain >= target_chain) {
            memcpy(s, c, sizeof(state));
            result.chain = chain;
            result.score = score;
            result.iterations = iteration;
            result = _chainify_cleanup(s, shots, result);
            handle_gravity(s);
            break;
        }
        if ((iteration + 1) % shot_patience == 0 && (result.target_shots + result.extra_shots < free_space)) {
            int j = jrand() % (NUM_COLORS - 1);
            if (popcounts[j]) {
                ++shotcounts[j];
                ++result.extra_shots;
            }
        }
        if ((iteration + 1) % chain_patience == 0) {
            --target_chain;
            for (int i = 0; i < NUM_COLORS - 1; ++i) {
                shotcounts[i] = target_shotcounts[i];
            }
            result.extra_shots = 0;
        }
        ++iteration;
    }
    free(c);
    free(cc);
    return result;
}
