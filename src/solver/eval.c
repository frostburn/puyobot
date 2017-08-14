#include <stdlib.h>
#include <string.h>

#include "puyobot/state.h"
#include "puyobot/solver/eval.h"

double eval_zero(void *s) {
    return 0;
}

// groups and chains inspired by https://github.com/mbrown1413/Puyo-AI

double eval_groups(void *_s) {
    State *s = _s;
    puyos_t groups[2*MAX_GROUPS];
    double score = 0;
    for (int i = 0; i < NUM_COLORS - 1; ++i) {
        puyos_t puyos[2] = {s->floors[0][i], s->floors[1][i]};
        int num = num_groups_2(puyos, groups);
        for (int j = 0; j < num; ++j) {
            int size = popcount(groups[2*j]) + popcount(groups[2*j + 1]);
            score += size*size;
        }
    }
    return score;
}

double eval_chains(void *_s) {
    State *s = _s;
    int original_pc = state_popcount(s);
    puyos_t all[2];
    get_state_mask(s, all);
    double score = 0;
    State *c = malloc(sizeof(State));
    for (int i = 0; i < NUM_COLORS - 1; ++i) {
        puyos_t handled[2] = {0};
        for (int j = 0; j < WIDTH; ++j) {
            // Resolution is expensive so we try to short-circuit.
            // We also filter out chains that we checked previously.
            // There are some corner cases where the filtering is overaggressive, but they are ignored for now.
            int depth = depth_2(all, j);
            puyos_t p[2];
            point_2(p, j, depth - 1);
            puyos_t target[2] = {
                (s->floors[0][i] & LIFE_BLOCK) | p[0],
                s->floors[1][i] | p [1]
            };
            flood_2(p, target);
            if (popcount_2(p) < CLEAR_THRESHOLD) {
                continue;
            } else if ((p[0] & handled[0]) || (p[1] & handled[1])) {
                continue;
            } else {
                handled[0] |= p[0];
                handled[1] |= p[1];
            }
            // Do the actual thing.
            memcpy(c, s, sizeof(State));
            c->floors[0][i] |= 1ULL << j;
            int chain;
            resolve(c, &chain);
            if (chain > 1) {
                double pc = state_popcount(c);
                double count = original_pc + 1 - pc;
                score += chain * chain * chain / count;
            }
        }
    }
    free(c);
    return score;
}


// Technically we would need a void match in the middle, but that's taken care of by the clears.
#define SANDWICH_LEFT ((1ULL << 1) | (1ULL << V_SHIFT) | (1ULL << (1 + 2 * V_SHIFT)))
#define SANDWICH_RIGHT (1ULL | (1ULL << (1 + V_SHIFT)) | (1ULL << (2 * V_SHIFT)))

#define SANDWICH_AURA ((1ULL << 1) | ((1ULL | (1ULL << 2) | (1ULL << (2 * V_SHIFT)) | (1ULL << (2 + 2 * V_SHIFT)) | (1ULL << (1 + 3 * V_SHIFT))) << V_SHIFT))
double eval_sandwich(void *s) {
    State *state = s;
    double score = 0;
    puyos_t sandwiched[2] = {0};
    for (int k = 0; k < TOTAL_HEIGHT - 2; ++k) {
        for (int i = 0; i < WIDTH - 1; ++i) {
            puyos_t pattern_left[2] = {SANDWICH_LEFT, 0};
            puyos_t pattern_right[2] = {SANDWICH_RIGHT, 0};
            puyos_t pattern_aura[2] = {SANDWICH_AURA, 0};
            translate_2(pattern_left, i, k);
            translate_2(pattern_right, i, k);
            translate_2(pattern_aura, i, k - 1);
            for (int j = 0; j < NUM_COLORS - j; ++j) {
                puyos_t puyos[2] = {state->floors[0][j], state->floors[1][j]};
                puyos_t aura[2] = {puyos[0] & pattern_aura[0], puyos[1] & pattern_aura[1]};
                if (popcount_2(aura) != 1) {
                    continue;
                }
                puyos_t match[2];
                match[0] = puyos[0] & pattern_left[0];
                match[1] = puyos[1] & pattern_left[1];
                if (match[0] == pattern_left[0] && match[1] == pattern_left[1]) {
                    sandwiched[0] |= match[0] | aura[0];
                    sandwiched[1] |= match[1] | aura[1];
                }
                match[0] = puyos[0] & pattern_right[0];
                match[1] = puyos[1] & pattern_right[1];
                if (match[0] == pattern_right[0] && match[1] == pattern_right[1]) {
                    sandwiched[0] |= match[0] | aura[0];
                    sandwiched[1] |= match[1] | aura[1];
                }
            }
        }
    }
    return popcount_2(sandwiched);
}
