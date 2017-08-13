#ifndef PUYOBOT_SOLVER_SEARCH_H_GUARD
#define PUYOBOT_SOLVER_SEARCH_H_GUARD

#include "puyobot/solver/eval.h"
#include "puyobot/deal.h"

typedef void* (*copy_fun)(void *s);
typedef void (*delete_fun)(void *s);
typedef int (*step_fun)(void *s, content_t deal, content_t choice, double *score);

typedef struct SearchOptions
{
    copy_fun copy;
    delete_fun delete;
    step_fun step;
    eval_fun eval;
    int depth;
    double tree_factor;
    choice_set_t *choice_sets;
} SearchOptions;

double solve_indeterministic(void *state, SearchOptions options);

double solve_deterministic(void *state, content_t *deals, int num_deals, SearchOptions options);

choice_set_t solve(void *state, content_t *deals, int num_deals, SearchOptions options);

#endif
