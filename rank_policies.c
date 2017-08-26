#include <stdlib.h>
#include <stdio.h>

#include <omp.h>

#include "puyobot/solver/policy.h"
#include "puyobot/solver/ranking.h"

int main() {
    jkiss_init();

    omp_set_num_threads(11);

    RankingOptions options = {0};
    options.num_deals = 3;
    options.min_chain = 1;
    options.num_chains = 20;
    RankingResult result;
    result = rank_policy(options, random_policy);
    printf("---Random policy (baseline)---\n");
    print_ranking_result(result, 1);
    printf("---Random but alive policy---\n");
    result = rank_policy(options, random_but_alive_policy);
    print_ranking_result(result, 1);
    printf("---Group chain---\n");
    result = rank_policy(options, group_chain_policy);
    print_ranking_result(result, 1);
    // printf("---Frog policy---\n");
    // result = rank_policy(1000, 3, frog_policy);
    // print_ranking_result(result, 1);
    // printf("---Group chain sandwich---\n");
    // result = rank_policy(options, gcs_policy);
    // print_ranking_result(result, 1);
    // printf("---Half deep---\n");
    // result = rank_policy(200, 3, half_deep_policy);
    // print_ranking_result(result, 1);
    // printf("---Chainless policy---\n");
    // result = rank_policy(1000, 3, chainless_policy);
    // print_ranking_result(result, 1);

    return 0;
}
