#include <assert.h>
#include <stdlib.h>

#include "jkiss/jkiss.h"

#include "puyobot/state.h"
#include "puyobot/solver/search.h"
#include "puyobot/solver/ranking.h"

void test_ranking_add() {
    RankingResult a = {0};
    RankingResult b = {0};

    a.options.num_deals = 1;
    b.options.num_deals = 1;
    a.iterations = 1;
    b.iterations = 2;
    a.chain_counts[0] = 3;
    b.chain_counts[0] = 4;
    a.chain_counts[1] = 5;

    RankingResult c = add_ranking_result(a, b);
    assert(c.options.num_deals == 1);
    assert(c.iterations == 3);
    assert(c.chain_counts[0] == 7);
    assert(c.chain_counts[1] == 5);
    assert(c.chain_counts[2] == 0);
    assert(a.chain_counts[0] == 3);
}

void test_ranking_random() {
    RankingOptions options = {0};
    options.iterations = 100;
    options.num_deals = 1;
    RankingResult result = rank_policy(options, random_policy);
    assert(result.puyos_played >= result.puyos_landed);
}

content_t random_but_lands_policy(void *s, content_t *deals, int num_deals) {
    choice_set_t choice_set = filter_landings(s);
    return rand_choice(choice_set);
}

void test_ranking_lands() {
    RankingOptions options = {0};
    options.iterations = 200;
    options.num_deals = 1;
    RankingResult result = rank_policy(options, random_but_lands_policy);
    print_ranking_result(result, 1);
    assert(result.puyos_played == result.puyos_played);
}

int main() {
    jkiss_init();

    int num_deals = 3;
    content_t deals[num_deals];
    for (int i = 0; i < num_deals; ++i) {
        deals[i] = rand_piece();
    }

    State *state = calloc(sizeof(State), 1);
    SearchOptions options = simple_search_options(eval_groups, 0, 1);
    for (int i = 0; i < 50; ++i) {
        choice_set_t choices = solve(state, deals, num_deals, options);
        print_choice_set(choices);
        double score;
        step_state(state, deals[0], rand_choice(choices), &score);
        print_state(state);
        printf("Score = %f\n\n", score);
        for (int j = 0; j < num_deals; ++j) {
            deals[j] = deals[j + 1];
        }
        deals[num_deals - 1] = rand_piece();
    }

    test_ranking_add();
    test_ranking_random();
    test_ranking_lands();
}
