#include <assert.h>
#include <stdlib.h>

#include "jkiss/jkiss.h"

#include "puyobot/state.h"
#include "puyobot/solver/search.h"

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
}
