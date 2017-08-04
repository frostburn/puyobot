#ifndef PUYOBOT_BOTTOM_H_GUARD
#define PUYOBOT_BOTTOM_H_GUARD

#include <stdlib.h>
#include <stdio.h>

#include "puyobot/constants.h"
#include "puyobot/bitboard.h"
#include "puyobot/deal.h"

#define MAX_BOTTOM_CHAIN ((WIDTH * HEIGHT) / CLEAR_THRESHOLD)

char color_label(int color);

void print_bottom_spam(puyos_t *floor, int num_links, int num_colors);

void print_bottom(puyos_t *floor, int num_colors);

int handle_bottom_gravity(puyos_t *floor, int num_colors);

int clear_bottom_groups(puyos_t *floor, int num_colors, int *color_cleared);

int resolve_bottom(puyos_t *floor, int num_colors, int *color_order);

char* color_conflicts(puyos_t *floor, int num_colors);

void print_conflicts(char* conflicts, int num_colors);

int bottom_deal_and_choice(puyos_t *floor, content_t deal, content_t choice);

void mirror_bottom(puyos_t *floor, int num_colors);

#endif
