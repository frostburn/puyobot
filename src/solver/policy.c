#include <assert.h>
#include <stdlib.h>

#include "puyobot/solver/policy.h"
#include "puyobot/state.h"

content_t random_policy(void *s, content_t *deals, int  num_deals) {
    return CHOICES[jrand() % NUM_CHOICES];
}

content_t random_but_alive_policy(void *s, content_t *deals, int  num_deals) {
    State *c = copy_state(s);
    unsigned int legal = (1 << NUM_CHOICES) - 1;
    while (legal) {
        int index = jrand() % NUM_CHOICES;
        while (!(legal & (1 << index))) {
            index = (index + 1) % NUM_CHOICES;
        }
        content_t choice = CHOICES[index];
        if (apply_deal_and_choice(c, deals[0], choice)) {
            free(c);
            return choice;
        }
        legal ^= 1 << index;
    }
    free(c);
    return CHOICE_PASS;
}

static choice_set_t FROG_CHOICES[3+1] = {
    CHOICE_SET_FROG_LEFT,
    CHOICE_SET_FROG_LEFT,
    CHOICE_SET_FROG_LEFT,
    CHOICE_SET_FROG_LEFT,
};
content_t frog_policy(void *s, content_t *deals, int num_deals) {
    assert(num_deals == 3);
    SearchOptions options = simple_search_options(eval_zero, 1, 1);
    options.choice_sets = FROG_CHOICES;
    return rand_choice(solve(s, deals, num_deals, options));
}

static choice_set_t HALF_DEEP_CHOICES[3+2] = {
    CHOICE_SET_ALL,
    CHOICE_SET_ALL,
    CHOICE_SET_ALL,
    CHOICE_SET_HALF,
    CHOICE_SET_HALF,
};
content_t half_deep_policy(void *s, content_t *deals, int num_deals) {
    assert(num_deals == 3);
    SearchOptions options = simple_search_options(eval_zero, 2, 1);
    options.choice_sets = HALF_DEEP_CHOICES;
    return rand_choice(solve(s, deals, num_deals, options));
}

content_t group_policy(void *s, content_t *deals, int  num_deals) {
    SearchOptions options = simple_search_options(eval_groups, 0, 0.015);
    return rand_choice(solve(s, deals, num_deals, options));
}

double _eval_groups_chains(void *s) {
    double groups = eval_groups(s);
    double chains = eval_chains(s);
    return groups + 20 * chains;
}

content_t group_chain_policy(void *s, content_t *deals, int  num_deals) {
    double factor = 0.0017;
    double pc = state_popcount(s);
    if (pc > 50) {
        factor = 0.03;
    }
    if (pc > 68) {
        factor = 0.1;
    }
    SearchOptions options = simple_search_options(_eval_groups_chains, 0, factor);
    return rand_choice(solve(s, deals, num_deals, options));
}
