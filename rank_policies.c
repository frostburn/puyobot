#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <omp.h>

#include "puyobot/solver/policy.h"
#include "puyobot/solver/ranking.h"
#include "puyobot/template/bottom.h"
#include "puyobot/template/pattern.h"

State* get_initial_state() {
    BottomTemplate *template;
    State *state;
    while (1) {
        template = bottom_chain_of_fours(7);
        if (!extend_bottom_chain(template, 0, 0)) {
            continue;
        }
        if (!extend_bottom_chain(template, 0, 0)) {
            continue;
        }
        if (!chip_bottom_trigger(template)) {
            free_bottom_template(template);
            continue;
        }
        calculate_bottom_conflicts(template);
        state = any_state_from_bottom(template, NUM_DEAL_COLORS, 10000);
        if (!state) {
            free_bottom_template(template);
            continue;
        }
        break;
    }
    free_bottom_template(template);
    return state;
}

RankingOptions get_better_initial_state() {
    RankingOptions options = {0};
    options.num_deals = 3;
    options.min_chain = 1;
    options.num_chains = 1;
    RankingResult result = {0};
    result.options = options;
    while (1) {
        State *state = get_initial_state();
        options.initial_state = *state;
        free(state);
        result = iter_rank_policy(options, group_chain_policy, 25);
        if (result.score > 1280 * result.puyos_played) {
            break;
        }
        printf("No dice, %f. Try again.\n", result.score / (double) result.puyos_played);
    }
    printf("--Group chains (initial)---\n");
    print_ranking_result(result, 1);
    return options;
}

int main() {
    jkiss_init();
    init_tetrominoes();

    omp_set_num_threads(11);
    RankingOptions options = get_better_initial_state();
    print_state(&options.initial_state);
    repr_state(&options.initial_state);
    RankingResult result;
    printf("---Random but alive policy---\n");
    result = iter_rank_policy(options, random_but_alive_policy, 100);
    print_ranking_result(result, 1);
    printf("---Group chain---\n");
    result = iter_rank_policy(options, group_chain_policy, 100);
    print_ranking_result(result, 1);
    printf("---Group chain sandwich---\n");
    result = iter_rank_policy(options, gcs_policy, 10);
    print_ranking_result(result, 1);

    return 0;
}
