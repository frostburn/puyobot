#ifndef PUYOBOT_UI_STATE_H_GUARD
#define PUYOBOT_UI_STATE_H_GUARD

#include <ncurses.h>

#include "puyobot/deal.h"
#include "puyobot/state.h"
#include "puyobot/multiplayer.h"

void wprint_state(WINDOW *win, State *s);

void preview_deal_and_choice(WINDOW *win, State *s, content_t deal, content_t choice);

void preview_deals(WINDOW *win, content_t *deals, int num_deals);

void wprint_player(WINDOW *win, Player *p);

void wprint_game_status(WINDOW *win, Game *g, int player_index);

State* pop_preview(State *s);

#endif
