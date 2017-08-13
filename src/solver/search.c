#include <assert.h>
#include <math.h>
#include <omp.h>
#include <stdio.h>

#include "puyobot/solver/search.h"

double solve_indeterministic(void *state, SearchOptions options) {
    double total = 0;
    for (int i = 0; i < NUM_DEAL_COLORS; ++i) {
        for (int j = i; j < NUM_DEAL_COLORS; ++j) {
             content_t deal = make_piece(i, j);
             double child_value = solve_deterministic(state, &deal, 1, options);
             total += child_value;
             if (i != j) {
                total += child_value;
             }
        }
    }
    return total / (NUM_DEAL_COLORS * NUM_DEAL_COLORS);
}

double solve_deterministic(void *state, content_t *deals, int num_deals, SearchOptions options) {
    if (!num_deals) {
        if (options.depth) {
            options.depth -= 1;
            return solve_indeterministic(state, options);
        } else {
            return options.eval(state);
        }
    }
    choice_set_t banned_choices = 0;
    SearchOptions child_options = options;
    if (options.choice_sets) {
        banned_choices = ~options.choice_sets[0];
        child_options.choice_sets += 1;
    }
    double best = -INFINITY;
    for (int i = 0; i < NUM_CHOICES; ++i) {
        if ((1 << i) & banned_choices) {
            continue;
        }
        void *child = options.copy(state);
        double step_score;
        if (!options.step(child, deals[0], CHOICES[i], &step_score)) {
            continue;
        }
        double child_value = solve_deterministic(child, deals + 1, num_deals - 1, child_options);
        options.delete(child);
        child_value += step_score * options.tree_factor;
        if (child_value > best) {
            best = child_value;
        }
    }
    return best;
}

choice_set_t solve(void *state, content_t *deals, int num_deals, SearchOptions options) {
    assert(num_deals);
    double values[NUM_CHOICES];
    choice_set_t banned_choices = 0;
    SearchOptions child_options = options;
    if (options.choice_sets) {
        banned_choices = ~options.choice_sets[0];
        child_options.choice_sets += 1;
    }
    #pragma omp parallel for
    for (int i = 0; i < NUM_CHOICES; ++i) {
        values[i] = NAN;
        if ((1 << i) & banned_choices) {
            continue;
        }
        void *child = options.copy(state);
        double step_score;
        if (!options.step(child, deals[0], CHOICES[i], &step_score)) {
            continue;
        }
        double child_value = solve_deterministic(child, deals + 1, num_deals - 1, child_options);
        child_value += step_score * options.tree_factor;
        values[i] = child_value;
    }
    double best = -INFINITY;
    choice_set_t choices = 0;
    for (int i = 0; i < NUM_CHOICES; ++i) {
        if (isnan(values[i])) {
            continue;
        }
        if (values[i] > best) {
            choices = 1 << i;
            best = values[i];
        } else if (values[i] == best) {
            choices |= 1 << i;
        }
    }
    return choices;
}
