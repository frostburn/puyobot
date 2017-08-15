#ifndef PUYOBOT_SOLVER_RANKING_H_GUARD
#define PUYOBOT_SOLVER_RANKING_H_GUARD

#include <stddef.h>

#include "puyobot/solver/policy.h"

typedef struct RankingResult {
    size_t num_deals;
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

RankingResult rank_policy(size_t iterations, size_t num_deals, policy_fun policy);

#endif
