#ifndef PUYOBOT_SOLVER_POLICY_H_GUARD
#define PUYOBOT_SOLVER_POLICY_H_GUARD

#include "jkiss/jkiss.h"

#include "puyobot/deal.h"
#include "puyobot/solver/search.h"
#include "puyobot/state.h"
#include "puyobot/template/bottom.h"

typedef content_t (*policy_fun)(void *s, content_t*, int);


content_t random_policy(void *s, content_t *deals, int  num_deals);

content_t random_but_alive_policy(void *s, content_t *deals, int  num_deals);

content_t random_survival_policy(void *s, content_t *deals, int  num_deals);

content_t frog_policy(void *s, content_t *deals, int num_deals);

content_t half_deep_policy(void *s, content_t *deals, int num_deals);

content_t group_policy(void *s, content_t *deals, int  num_deals);

content_t group_chain_policy(void *s, content_t *deals, int  num_deals);

content_t chainless_policy(void *s, content_t *deals, int num_deals);

content_t gcs_policy(void *s, content_t *deals, int num_deals);

choice_set_t filter_chains(State *state, content_t deal, int max_chain);

choice_set_t filter_landings(State *state);

content_t template_policy(BottomTemplate *template, int depth, double factor, void *s, content_t *deals, int num_deals);

#endif
