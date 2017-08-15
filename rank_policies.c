#include <stdlib.h>
#include <stdio.h>

#include "puyobot/solver/policy.h"
#include "puyobot/solver/ranking.h"

#define NUM_DEALS (3)

int main() {
    jkiss_init();

    RankingResult result;
    result = rank_policy(100000, 3, random_policy);
    printf("---Random policy (baseline)---\n");
    print_ranking_result(result, 1);
    printf("---Random but alive policy---\n");
    result = rank_policy(10000, 3, random_but_alive_policy);
    print_ranking_result(result, 1);
    printf("---Group chain---\n");
    result = rank_policy(100000, 3, group_chain_policy);
    print_ranking_result(result, 1);
    printf("---Frog policy---\n");
    result = rank_policy(10000, 3, frog_policy);
    print_ranking_result(result, 1);
    printf("---Half deep---\n");
    result = rank_policy(200, 3, half_deep_policy);
    print_ranking_result(result, 1);
    printf("---Chainless policy---\n");
    result = rank_policy(10000, 3, chainless_policy);
    print_ranking_result(result, 1);

    return 0;
}
