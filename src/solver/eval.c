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
