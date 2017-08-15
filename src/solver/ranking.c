#include <stdio.h>
#include <stdlib.h>

#include "puyobot/state.h"
#include "puyobot/solver/ranking.h"

void print_ranking_result(RankingResult result, int suppress_zero) {
    size_t resolution = 70;
    size_t most = 0;
    for (int i = !!suppress_zero; i < MAX_CHAIN + 1; ++i) {
        if (result.chain_counts[i] > most) {
            most = result.chain_counts[i];
        }
    }
    for (int i = !!suppress_zero; i < MAX_CHAIN + 1; ++i) {
        printf("%2d:", i);
        for (size_t j = 0; most * j < result.chain_counts[i] * resolution; ++j) {
            printf("#");
        }
        printf("\n");
    }
    printf(
        "deals=%zu\niterations=%zu\nscore=%zu\npuyos played=%zu\npuyos landed=%zu\ngame overs=%zu\nall clears=%zu\n",
        result.num_deals,
        result.iterations,
        result.score,
        result.puyos_played,
        result.puyos_landed,
        result.game_overs,
        result.all_clears
    );
    printf(
        "efficiency=%f\nlanding rate=%f%%\ngame over rate=%f%%\nall clear rate=%f%%\n",
        result.score / ((double) result.puyos_played),
        100.0 * result.puyos_landed / ((double) result.puyos_played),
        100.0 * result.game_overs / ((double) result.iterations),
        100.0 * result.all_clears / ((double) result.iterations)
    );
}

RankingResult add_ranking_result(RankingResult a, RankingResult b) {
    if (a.num_deals != b.num_deals) {
        fprintf(stderr, "Cannot add results from different number of deals\n");
        exit(1);
    }
    RankingResult c = a;
    c.iterations += b.iterations;
    c.score += b.score;
    c.puyos_played += b.puyos_played;
    c.puyos_landed += b.puyos_landed;
    c.game_overs += b.game_overs;
    c.all_clears += b.all_clears;

    for (int i = 0; i < MAX_CHAIN + 1; ++i) {
        c.chain_counts[i] += b.chain_counts[i];
    }

    return c;
}

RankingResult rank_policy(size_t iterations, size_t num_deals, policy_fun policy) {
    RankingResult result = {0};
    result.num_deals = num_deals;
    result.iterations = iterations;

    content_t *deals = malloc(num_deals * sizeof(content_t));
    for (int i = 0; i < num_deals; ++i) {
        deals[i] = rand_piece();
    }
    State *state = calloc(1, sizeof(State));

    for (size_t i = 0; i < iterations; ++i) {
        content_t choice = policy(state, deals, num_deals);
        if (!apply_deal_and_choice(state, deals[0], choice)) {
            ++result.game_overs;
            clear_state(state);
        } else if (state_is_clear(state)) {
            ++result.all_clears;
        }
        int chain = 0;
        puyos_t all[2];
        get_state_mask(state, all);
        puyos_t puyos_being_played = all[0] & TOP;
        if (puyos_being_played) {
            result.puyos_played += 2;
            puyos_t death_line = (all[0] >> V_SHIFT) & DEATH_BLOCK;
            // XXX: Landing rate calculation not exactly precise in the case of suicides.
            if (beam_down(puyos_being_played) & death_line) {
                result.puyos_landed += 1;
            } else {
                result.puyos_landed += 2;
            }
        }
        result.score += resolve(state, &chain);
        ++result.chain_counts[chain];
        for (size_t j = 0; j < num_deals - 1; ++j) {
            deals[j] = deals[j + 1];
        }
        deals[num_deals - 1] = rand_piece();
    }
    return result;
}
