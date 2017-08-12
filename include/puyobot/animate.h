#ifndef PUYOBOT_ANIMATE_H_GUARD
#define PUYOBOT_ANIMATE_H_GUARD

#include "puyobot/state.h"

#define ANIM_DELAY (20000)

void redraw_state(State *s);

void animate_gravity(State *s, void (*callback)(State*));

void animate(State *s, void (*callback)(State*));

void redraw_bottom_spam(puyos_t *floor, int num_links, int num_colors);

int animate_bottom_gravity(puyos_t *floor, int num_links, int num_colors, void (*callback)(puyos_t*, int, int));

void animate_bottom(puyos_t *floor, int num_links, int num_colors, void (*callback)(puyos_t*, int, int));

#endif
