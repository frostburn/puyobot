#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "puyobot/solver/policy.h"

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

choice_set_t filter_chains(State *state, content_t deal, int max_chain) {
    choice_set_t valid = 0;
    State *copy = malloc(sizeof(State));
    for (int i = 0; i < NUM_CHOICES; ++i) {
        memcpy(copy, state, sizeof(State));
        if (!apply_deal_and_choice(copy, deal, CHOICES[i])) {
            continue;
        }
        int chain = 0;
        resolve(copy, &chain);
        if (chain <= max_chain) {
            valid |= 1 << i;
        }
    }
    free(copy);
    return valid;
}

content_t chainless_policy(void *s, content_t *deals, int num_deals) {
    SearchOptions options = simple_search_options(eval_zero, 1, 1);
    options.choice_sets = malloc((num_deals + 1) * sizeof(choice_set_t));
    for (int i = 0; i < num_deals + 1; ++i) {
        options.choice_sets[i] = CHOICE_SET_ALL;
    }
    if (state_popcount(s) + 16 < TOTAL_SPACE) {
        options.choice_sets[0] = filter_chains(s, deals[0], 0);
        if (!options.choice_sets[0]) {
            options.choice_sets[0] = CHOICE_SET_ALL;
        }
    }
    content_t choice = rand_choice(solve(s, deals, num_deals, options));
    free(options.choice_sets);
    return choice;
}
