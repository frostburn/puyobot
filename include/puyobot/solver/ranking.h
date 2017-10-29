#ifndef PUYOBOT_SOLVER_RANKING_H_GUARD
#define PUYOBOT_SOLVER_RANKING_H_GUARD

#include <stddef.h>

#include "puyobot/state.h"
#include "puyobot/solver/policy.h"

typedef struct RankingOptions {
    State initial_state;
    size_t iterations;
    size_t num_deals;
    size_t min_chain;
    size_t num_chains;
} RankingOptions;

typedef struct RankingResult {
    RankingOptions options;
    size_t iterations;
    size_t score;
    size_t puyos_played;
    size_t puyos_landed;
    size_t game_overs;
    size_t all_clears;
    size_t chain_counts[MAX_CHAIN + 1];
} RankingResult;

void print_ranking_result(RankingResult result, int suppress_zero);

RankingResult add_ranking_result(RankingResult a, RankingResult b);

RankingResult rank_policy(RankingOptions options, policy_fun policy);

RankingResult iter_rank_policy(RankingOptions options, policy_fun policy, size_t iterations);

RankingResult iter_rank_policy_parallel(RankingOptions options, policy_fun policy, size_t iterations);

#endif
