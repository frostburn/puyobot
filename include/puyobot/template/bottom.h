#ifndef PUYOBOT_TEMPLATE_BOTTOM_H_GUARD
#define PUYOBOT_TEMPLATE_BOTTOM_H_GUARD

#include "puyobot/bitboard.h"
#include "puyobot/state.h"

typedef struct BottomTemplate
{
    puyos_t *floor;
    puyos_t trigger_front;
    int num_links;
    int num_colors;
    char *conflicts;
    float *weights;
} BottomTemplate;

void free_bottom_template(BottomTemplate *template);

void print_bottom_template(BottomTemplate *template);

BottomTemplate* bottom_chain_of_fours(int num_links);

int extend_bottom_chain(BottomTemplate *template, puyos_t fixed, int allow_cuts);

int tail_bottom_chain(BottomTemplate *template);

int spam_bottom(BottomTemplate *template);

int spam_bottom_aura(BottomTemplate *template);

int sprinkle_bottom(BottomTemplate *template);

int* minimum_assignments(BottomTemplate *template, int *min_colors);

State* min_state_from_bottom(BottomTemplate *template);

State* any_state_from_bottom(BottomTemplate *template, int num_colors, size_t patience);

puyos_t reverse_bottom_cut(BottomTemplate *template);

puyos_t cut_bottom_trigger(BottomTemplate *template);

void calculate_bottom_conflicts(BottomTemplate *template);

#endif
