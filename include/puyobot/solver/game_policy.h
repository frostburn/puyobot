#ifndef PUYOBOT_SOLVER_GAME_POLICY_H_GUARD
#define PUYOBOT_SOLVER_GAME_POLICY_H_GUARD

#include "puyobot/deal.h"
#include "puyobot/multiplayer.h"

content_t gcn_practice_policy(void *pg, content_t *deals, int num_deals);

content_t gcn_game_policy(Game *g, int player_index);

#endif
