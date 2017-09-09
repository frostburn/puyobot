#ifndef PUYOBOT_TEMPLATE_BOTTOM_MATCH_H_GUARD
#define PUYOBOT_TEMPLATE_BOTTOM_MATCH_H_GUARD

#include "puyobot/bitboard.h"
#include "puyobot/state.h"
#include "puyobot/template/bottom.h"

typedef struct BottomMatchResult
{
    puyos_t all;
    puyos_t all_template;
    puyos_t all_chain;
    puyos_t on_chain;
    puyos_t chain_garbage;
    puyos_t on_spam;
    puyos_t off_template;
    puyos_t on_trigger;
    puyos_t on_single_conflicts;
    puyos_t trigger_continuation;
    int num_color_conflicts;
    int num_spam_conflicts;
    int num_on_top;
} BottomMatchResult;

void print_bottom_match_result (BottomMatchResult result);

BottomMatchResult match_bottom(State *s, BottomTemplate *template);

double bottom_match_score(BottomTemplate *template, BottomMatchResult result);

double simple_bottom_match_score(BottomTemplate *template, BottomMatchResult result);

#endif
