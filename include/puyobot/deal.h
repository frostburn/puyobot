#ifndef PUYOBOT_DEAL_H_GUARD
#define PUYOBOT_DEAL_H_GUARD

#include "puyobot/constants.h"

#define NUM_CHOICES (2 * WIDTH + 2 * (WIDTH - 1))
#define CHOICE_0 (0)
#define CHOICE_90 (64)
#define CHOICE_180 (128)
#define CHOICE_270 (192)
#define CHOICE_PASS (255)
#define CHOICE_X_MASK (0x3f)
#define COLOR1_MASK (15)
#define COLOR2_SHIFT (4)
#define DEATH_SCORE (1e13)

typedef unsigned char content_t;

extern const content_t ROTATIONS[];
extern const content_t CHOICES[];

content_t make_piece(content_t color1, content_t color2);

content_t rand_piece();

content_t deal_color1(content_t deal);

content_t deal_color2(content_t deal);

content_t make_choice(int *x, int *orientation);

content_t rand_choice(content_t min_x, content_t max_x);

void print_deals(content_t *deals, int num_deals);

#endif
