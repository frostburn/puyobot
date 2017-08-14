#ifndef PUYOBOT_SOLVER_EVAL_H_GUARD
#define PUYOBOT_SOLVER_EVAL_H_GUARD

typedef double (*eval_fun)(void *s);

double eval_zero(void *s);

double eval_groups(void *_s);

double eval_chains(void *_s);

double eval_sandwich(void *s);

#endif
