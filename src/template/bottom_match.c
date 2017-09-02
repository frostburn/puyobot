#include <assert.h>
#include <stdio.h>

#include "puyobot/bottom.h"
#include "puyobot/template/bottom_match.h"

void print_bottom_match_result (BottomMatchResult result)
{
    puyos_t p[6];
    p[0] = result.on_chain;
    p[1] = result.chain_garbage;
    p[2] = result.all_chain ^ result.on_chain;
    p[3] = result.on_spam;
    p[4] = result.off_template;
    p[5] = result.all & ~(p[0] | p[1] | p[2] | p[3]);
    print_bottom(p, 6);
    printf("number off-screen=%d ", result.num_on_top);
    printf("legend: 0~on chain, 1~chain garbage, 2~chain remaining, 3~spam, 4~off template, 5~other\n");

    p[0] = result.on_trigger;
    p[1] = result.on_single_conflicts;
    p[2] = result.trigger_continuation;
    p[3] = result.all_template & ~(p[0] | p[1] | p[2]);
    print_bottom(p, 4);
    printf("number of conflicts=%d+%d ", result.num_color_conflicts, result.num_spam_conflicts);
    printf("legend: 0~trigger, 1~conflicts, 2~trigger continuation, 3~rest of template\n");
}


BottomMatchResult match_bottom(State *s, BottomTemplate *template) {
    assert(template->conflicts);
    int num_colors = template->num_colors;
    int num_links = template->num_links;
    int *assignments = malloc(num_colors * sizeof(int));
    for (int j = 0; j < num_colors; ++j) {
        assignments[j] = -1 - j;
    }
    puyos_t chain = 0;
    for (int j = 0; j < num_links; ++j) {
        chain |= template->floor[j];
    }
    puyos_t *floor = s->floors[1];
    puyos_t all = 0;
    puyos_t all_template = 0;
    puyos_t all_chain = 0;
    puyos_t on_chain = 0;
    puyos_t chain_garbage = 0;
    puyos_t on_spam = 0;
    puyos_t on_single_conflicts = 0;
    for (int j = 0; j < num_colors; ++j) {
        all_template |= template->floor[j];
        if (j < num_links) {
            all_chain |= template->floor[j];
        }
    }
    for (int i = 0; i < NUM_COLORS; ++i) {
        all |= floor[i];
        for (int j = 0; j < num_colors; ++j) {
            all_template |= template->floor[j];
            puyos_t overlap = template->floor[j] & floor[i];
            if (overlap && i < NUM_COLORS - 1) {
                if (assignments[j] >= 0) {
                    puyos_t prev_overlap = template->floor[j] & floor[assignments[j]];
                    if (popcount(overlap) > popcount(prev_overlap)) {
                        assignments[j] = i;
                    }
                    on_single_conflicts |= overlap | prev_overlap;
                    continue;
                }
                assignments[j] = i;
            }
            if (j < num_links) {
                if (i == NUM_COLORS - 1) {
                    chain_garbage |= overlap;
                } else {
                    on_chain |= overlap;
                }
            } else {
                on_spam |= overlap;
            }
        }
    }
    int num_color_conflicts = 0;
    int num_spam_conflicts = 0;
    for (int i = 0; i < num_links; ++i) {
        for (int j = i + 1; j < num_colors; ++j) {
            if (assignments[i] == assignments[j] && template->conflicts[i + j * num_colors]) {
                if (j < num_links) {
                    num_color_conflicts++;
                } else {
                    num_spam_conflicts++;
                }
                // spam vs. spam conflicts ignored as they shouldn't exist in the first place.
            }
        }
    }
    free(assignments);

    puyos_t rest_of_the_chain = all_chain ^ template->floor[0];
    puyos_t on_trigger = 0;
    for (int i = 0; i < NUM_COLORS - 1; ++i) {
        if (floor[i] & template->floor[0]) {
            on_trigger = flood(template->floor[0], floor[i]);
            if (on_trigger & rest_of_the_chain) {
                on_trigger = 0;
                continue;
            }
            break;
        }
    }
    puyos_t trigger_front = cross(on_trigger) & ~(all_chain | beam_down(on_trigger));
    puyos_t trigger_continuation = 0;
    for (int i = 0; i < NUM_COLORS - 1; ++i) {
        if (floor[i] & trigger_front) {
            puyos_t temp = flood(trigger_front, floor[i]);
            if (popcount(temp) > popcount(trigger_continuation)) {
                trigger_continuation = temp;
            }
        }
    }

    on_spam &= ~(on_trigger | trigger_continuation);

    int num_on_top = 0;
    for (int i = 0; i < NUM_COLORS - 1; ++i) {
        num_on_top += popcount(s->floors[0][i]);
    }

    return (BottomMatchResult) {
        .all = all,
        .all_template = all_template,
        .all_chain = all_chain,
        .on_chain = on_chain,
        .chain_garbage = chain_garbage,
        .on_spam = on_spam,
        .off_template = (all & ~all_template),
        .on_trigger = on_trigger,
        .on_single_conflicts = on_single_conflicts,
        .trigger_continuation = trigger_continuation,
        .num_color_conflicts = num_color_conflicts,
        .num_spam_conflicts = num_spam_conflicts,
        .num_on_top = num_on_top,
    };
}

double bottom_match_score(BottomTemplate *template, BottomMatchResult result) {
    double penalty = 0;
    penalty += 1.2 * result.num_color_conflicts;
    penalty += popcount(result.on_single_conflicts);
    penalty += 0.5 * result.num_spam_conflicts;

    double minor_penalty = 0;
    minor_penalty += 0.09 * popcount(result.chain_garbage);
    minor_penalty += 0.05 * popcount(result.off_template);
    minor_penalty += 0.08 * popcount(result.on_spam);
    minor_penalty += 0.04 * result.num_on_top;

    double score;
    if (!template->weights) {
        score = popcount(result.on_chain) / (double) popcount(result.all_chain);
    } else {
        double weight = 0;
        double total_mass = 0;
        for (int i = 0; i < template->num_links; ++i) {
            weight += popcount(template->floor[i] & result.on_chain) * template->weights[i];
            total_mass += popcount(template->floor[i]) * template->weights[i];
        }
        score = weight / total_mass;
    }
    score += popcount(result.on_trigger) * 0.05;
    score += popcount(result.trigger_continuation) * 0.03;
    score -= minor_penalty;

    if (penalty) {
        return score - penalty - 2;
    }
    return score;
}