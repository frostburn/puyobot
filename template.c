#define NUM_TETROMINOES (19)
#define NUM_TRANSLATED_TETROMINOES (725)
#define NUM_TRANSLATED_TETROMINOES_2 (897)
#define SHOT_PATIENCE (100000)
#define CHAIN_PATIENCE (100 * SHOT_PATIENCE)

static puyos_t TETROMINOES[NUM_TETROMINOES] = {
    195,  // O
    387, 4290,  // Z
    198, 8385,  // S
    15, 266305,  // I
    71, 452, 8323, 12353,  // L
    135, 450, 4289, 8386,  // T
    263, 449, 4163, 12418,  // J
};

static int TETROMINO_DIMS[NUM_TETROMINOES][2] = {
    {2, 2},
    {3, 2}, {2, 3},
    {3, 2}, {2, 3},
    {4, 1}, {1, 4},
    {3, 2}, {3, 2}, {2, 3}, {2, 3},
    {3, 2}, {3, 2}, {2, 3}, {2, 3},
    {3, 2}, {3, 2}, {2, 3}, {2, 3},
};

static puyos_t TRANSLATED_TETROMINOES[NUM_TRANSLATED_TETROMINOES];
static puyos_t TRANSLATED_TETROMINOES_2[2 * NUM_TRANSLATED_TETROMINOES_2];

#include "bottom_template.c"

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

void init_tetrominoes() {
    int n = 0;
    for (int i = 0; i < NUM_TETROMINOES; ++i) {
        for (int j = 0; j <= WIDTH - TETROMINO_DIMS[i][0]; ++j) {
            for (int k = 0; k <= HEIGHT - TETROMINO_DIMS[i][1]; ++k) {
                TRANSLATED_TETROMINOES[n++] = TETROMINOES[i] << (j + k * V_SHIFT);
            }
        }
    }
    n = 0;
    for (int i = 0; i < NUM_TETROMINOES; ++i) {
        for (int j = 0; j <= WIDTH - TETROMINO_DIMS[i][0]; ++j) {
            for (int k = 0; k <= LIFE_HEIGHT - TETROMINO_DIMS[i][1]; ++k) {
                TRANSLATED_TETROMINOES_2[2*n] = TETROMINOES[i];
                TRANSLATED_TETROMINOES_2[2*n + 1] = 0;
                translate_2(TRANSLATED_TETROMINOES_2 + 2*n, j, k + GHOST_Y);
                ++n;
            }
        }
    }
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

    int n = NUM_TRANSLATED_TETROMINOES_2;
    puyos_t *tetrominoes = malloc(2*n * sizeof(puyos_t));
    memcpy(tetrominoes, TRANSLATED_TETROMINOES_2, 2*n * sizeof(puyos_t));
    shuffle_2(tetrominoes, n);
    puyos_t colors[NUM_COLORS - 1] = {RED, GREEN, YELLOW, BLUE, PURPLE};
    shuffle(colors, NUM_COLORS - 1);

    state *cc = malloc(sizeof(state));
    for (int i = 0; i < n; ++i) {
        puyos_t tetromino[2] = {tetrominoes[2*i], tetrominoes[2*i + 1]};
        if (fixed) {
            puyos_t lift[2] = {tetromino[0], tetromino[1]};
            beam_up_2(lift);
            if ((lift[0] & fixed[0]) || (lift[1] & fixed[1])) {
                continue;
            }
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
