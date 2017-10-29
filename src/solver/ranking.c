#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <omp.h>

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
        "deals=%zu\nmax iterations=%zu\nminimum chain=%zu\nmax chains=%zu\niterations=%zu\nscore=%zu\npuyos played=%zu\npuyos landed=%zu\ngame overs=%zu\nall clears=%zu\n",
        result.options.num_deals,
        result.options.iterations,
        result.options.min_chain,
        result.options.num_chains,
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
    if (a.options.num_deals != b.options.num_deals) {
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

RankingResult rank_policy(RankingOptions options, policy_fun policy) {
    RankingResult result = {0};
    result.options = options;
    result.iterations = 0;

    content_t *deals = malloc(options.num_deals * sizeof(content_t));
    for (size_t i = 0; i < options.num_deals; ++i) {
        deals[i] = rand_piece();
    }
    State *state = malloc(sizeof(State));
    memcpy(state, &options.initial_state, sizeof(State));

    size_t num_chains = 0;
    if (!options.iterations) {
        options.iterations = ~options.iterations;
    }
    for (size_t i = 0; i < options.iterations; ++i) {
        content_t choice = policy(state, deals, (int) options.num_deals);
        if (!apply_deal_and_choice(state, deals[0], choice)) {
            ++result.game_overs;
            clear_state(state);
        } else if (state_is_clear(state)) {
            ++result.all_clears;
        }
        int chain = 0;
        puyos_t all[2];
        get_state_mask(state, all);
        puyos_t puyos_being_played = all[0] & (TOP | (TOP << V_SHIFT));
        if (puyos_being_played) {
            result.puyos_played += 2;
            puyos_t ghosts = all[0] & GHOST_LINE;
            // XXX: Landing rate calculation not exactly precise in the case of suicides.
            if (beam_down(puyos_being_played) & ghosts) {
                result.puyos_landed += 1;
            } else {
                result.puyos_landed += 2;
            }
        }
        result.score += resolve(state, &chain);
        ++result.chain_counts[chain];
        ++result.iterations;
        if (options.min_chain && chain >= options.min_chain) {
            num_chains++;
            if (options.num_chains && num_chains >= options.num_chains) {
                break;
            }
        }
        for (size_t j = 0; j < options.num_deals - 1; ++j) {
            deals[j] = deals[j + 1];
        }
        deals[options.num_deals - 1] = rand_piece();
    }
    return result;
}

RankingResult iter_rank_policy(RankingOptions options, policy_fun policy, size_t iterations) {
    RankingResult result = {0};
    result.options = options;
    for (size_t i = 0; i < iterations; ++i) {
        result = add_ranking_result(
            result,
            rank_policy(options, policy)
        );
    }
    return result;
}

RankingResult iter_rank_policy_parallel(RankingOptions options, policy_fun policy, size_t iterations) {
    RankingResult *results = malloc(iterations * sizeof(RankingResult));
    #pragma omp parallel for
    for (size_t i = 0; i < iterations; ++i) {
        results[i].options = options;
        results[i] = rank_policy(options, policy);
    }
    RankingResult result = {0};
    result.options = options;
    for (size_t i = 0; i < iterations; ++i) {
        result = add_ranking_result(
            result,
            results[i]
        );
    }
    free(results);
    return result;
}
