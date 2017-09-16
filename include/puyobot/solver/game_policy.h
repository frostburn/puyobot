#ifndef PUYOBOT_SOLVER_GAME_POLICY_H_GUARD
#define PUYOBOT_SOLVER_GAME_POLICY_H_GUARD

#include "puyobot/deal.h"
#include "puyobot/multiplayer.h"

typedef void (*multi_policy_fun)(void *s, content_t*);

content_t gcn_practice_policy(void *pg, content_t *deals, int num_deals);

content_t gcn_game_policy(Game *g, int player_index);

void multi_random_policy(void *g, content_t *choices);

void multi_random_alive_policy(void *g, content_t *choices);

#endif
