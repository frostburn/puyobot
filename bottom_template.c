typedef struct bottom_template
{
    puyos_t *floor;
    puyos_t trigger_front;
    int num_links;
    int num_colors;
    char *conflicts;
    int score;
    float *weights;
} bottom_template;

typedef struct bottom_match_result
{
    puyos_t all;
    puyos_t all_template;
    puyos_t all_chain;
    puyos_t on_chain;
    puyos_t on_spam;
    puyos_t off_template;
    puyos_t on_trigger_front;
    puyos_t on_single_conflicts;
    int num_color_conflicts;
    int num_spam_conflicts;
    int num_on_top;
} bottom_match_result;

void free_bottom_template(bottom_template *template) {
    free(template->floor);
    free(template->conflicts);
    free(template->weights);
    free(template);
}

#include "bottom.c"

void print_bottom_match_result (bottom_match_result result)
{
    puyos_t p[4];
    p[0] = result.on_chain;
    p[1] = result.on_spam;
    p[2] = result.off_template;
    p[3] = result.all & ~(p[0] | p[1] | p[2]);
    print_bottom(p, 4);
    printf("number off-screen=%d ", result.num_on_top);
    printf("legend: 0~chain, 1~spam, 2~off template, 3~other\n");

    p[0] = result.on_trigger_front;
    p[1] = result.on_single_conflicts;
    p[2] = result.all_template & ~(p[0] | p[1]);
    print_bottom(p, 3);
    printf("number of conflicts=%d+%d ", result.num_color_conflicts, result.num_spam_conflicts);
    printf("legend: 0~trigger front, 1~conflicts, 2~rest of template\n");
}

bottom_template* bottom_chain_of_fours(int num_links) {
    puyos_t *floor = malloc(num_links * sizeof(puyos_t));
    puyos_t *temp = malloc(num_links * sizeof(puyos_t));
    int *color_order = malloc(num_links * sizeof(int));
    while (1) {
        memset(floor, 0, num_links * sizeof(puyos_t));
        // Insert the trigger. Always a tetrominoe.
        // Can be red without loss of generality.
        // Falls into place so doesn't always end up being the trigger.
        int i = jrand() % NUM_TETROMINOES;
        floor[0] |= TETROMINOES[i] << (
            jrand() % (WIDTH - TETROMINO_DIMS[i][0]) +
            (jrand() % (HEIGHT - TETROMINO_DIMS[i][1])) * V_SHIFT
        );
        puyos_t allowed = FULL ^ floor[0];
        for (int k = 1; k < num_links; ++k) {
            int j = CLEAR_THRESHOLD;
            while (j) {
                puyos_t p = 1ULL << (jrand() % (WIDTH * HEIGHT));
                if (p & allowed) {
                    floor[k] |= p;
                    allowed ^= p;
                    --j;
                }
            }
            if (has_gap(floor[k])) {
                allowed ^= floor[k];
                floor[k] = 0;
                --k;
                continue;
            }
        }
        memcpy(temp, floor, num_links * sizeof(puyos_t));
        int chain = resolve_bottom(temp, num_links, color_order);
        if (chain == num_links) {
            handle_bottom_gravity(floor, num_links);
            memcpy(temp, floor, num_links * sizeof(puyos_t));
            for (int i = 0; i < num_links; ++i) {
                floor[i] = temp[color_order[i]];
            }
            break;
        }
    }
    free(temp);
    bottom_template *template = calloc(1, sizeof(bottom_template));
    template->floor = floor;
    template->num_links = num_links;
    template->num_colors = num_links;
    return template;
}

puyos_t _reverse_bottom_cut(puyos_t *floor, puyos_t trigger_front, int num_colors) {
    if (!trigger_front) {
        return 0;
    }
    puyos_t all = 0;
    for (int i = 0; i < num_colors; ++i) {
        all |= floor[i];
    }
    puyos_t reverse_cut = trigger_front;
    reverse_cut &= (all >> V_SHIFT) | BOTTOM;
    if (!reverse_cut) {
        // Not enough support. Stretch to bottom to compensate.
        reverse_cut = beam_down(trigger_front) & ~all;
    }
    // If the trigger has been mutilated enough this might loop forever but we don't care for now.
    while (popcount(floor[0]) + popcount(reverse_cut) < CLEAR_THRESHOLD) {
        reverse_cut |= reverse_cut >> V_SHIFT;
    }
    floor[0] |= reverse_cut;
    return reverse_cut;
}

int extend_bottom_chain(bottom_template *template, puyos_t fixed, int allow_cuts) {
    if (template->trigger_front) {
        return 0;
    }
    int num_colors = template->num_colors;
    puyos_t *floor = template->floor;
    if (!num_colors) {
        int i = jrand() % NUM_TETROMINOES;
        template->num_colors = 1;
        template->num_links = 1;
        template->floor = malloc(sizeof(puyos_t));
        template->floor[0] = TETROMINOES[i] << (jrand() % (WIDTH - TETROMINO_DIMS[i][0]));
        handle_bottom_gravity(template->floor, 1);
        return 1;
    }

    puyos_t *temp = malloc(num_colors * sizeof(puyos_t));
    memcpy(temp, floor, num_colors * sizeof(puyos_t));
    int color_cleared;
    clear_bottom_groups(temp, num_colors, &color_cleared);
    puyos_t trigger = 0;
    for (int i = 0; i < num_colors; ++i) {
        if (floor[i] != temp[i]) {
            trigger = floor[i] ^ temp[i];
            break;
        }
    }
    if (!trigger) {
        free(temp);
        return 0;
    }
    memcpy(temp, floor, num_colors * sizeof(puyos_t));
    int chain = resolve_bottom(temp, num_colors, NULL);
    trigger = beam_down(trigger);

    int n = NUM_TRANSLATED_TETROMINOES;
    puyos_t *tetrominoes = malloc(n * sizeof(puyos_t));
    memcpy(tetrominoes, TRANSLATED_TETROMINOES, n * sizeof(puyos_t));
    shuffle(tetrominoes, n);

    puyos_t *temp2 = malloc((num_colors + 1) * sizeof(puyos_t));
    int *color_order = malloc((num_colors + 1) * sizeof(int));
    for (int i = 0; i < n; ++i) {
        if (beam_up(tetrominoes[i]) & fixed) {
            continue;
        }
        if (tetrominoes[i] & trigger) {
            puyos_t tetromino = tetrominoes[i];
            puyos_t lifter = tetromino;
            memcpy(temp, floor, num_colors * sizeof(puyos_t));
            while (lifter) {
                puyos_t lift = beam_up(tetromino);
                for (int j = 0; j < num_colors; ++j) {
                    puyos_t p = temp[j];
                    temp[j] = (p & ~lift) | ((p & lift) >> V_SHIFT);
                }
                lifter &= lifter >> V_SHIFT;
            }
            memcpy(temp2 + 1, temp, num_colors * sizeof(puyos_t));
            if (allow_cuts) {
                temp2[0] = beam_down(tetromino);
            } else {
                temp2[0] = tetromino;
            }
            int new_chain = resolve_bottom(temp2, num_colors + 1, color_order);
            if (fixed && color_order[0] != 0) {
                continue;  // Fixed chains require the new material to become the trigger.
            }
            if (new_chain > chain) {
                for (int i = 0; i < num_colors + 1; ++i) {
                    int j = color_order[i] - 1;
                    if (j >= 0) {
                        temp2[i] = temp[j];
                    } else {
                        temp2[i] = tetromino;
                    }
                }
                puyos_t gap = 0;
                if (allow_cuts) {
                    puyos_t all = 0;
                    for (int i = 0; i < num_colors + 1; ++i) {
                        all |= temp2[i];
                    }
                    gap = beam_down(temp2[0]) & ~all;
                    // We could do a half-assed cut like this, but the full cut bellow is cleaner.
                    // temp2[0] &= ~beam_up(gap);

                    // Eliminate (some) frivolous gaps where gravity still makes a tetromino.
                    gap &= ~cross(temp2[0] & ~beam_up(gap));
                }
                handle_bottom_gravity(temp2, num_colors + 1);
                if (gap) {
                    // Make a full cut
                    puyos_t rest = 0;
                    for (int i = 1; i < num_colors + 1; ++i) {
                        rest |= temp2[i];
                    }
                    for (int i = 0; i < WIDTH * HEIGHT; ++i) {
                        puyos_t p = 1ULL << i;
                        if (p & temp2[0]) {
                            p = beam_up(p);
                            if (!(p & rest)) {
                                temp2[0] &= ~p;
                            }
                        }
                    }
                    template->trigger_front = cross(temp2[0]) & ~(temp2[0] | rest);
                }
                free(template->floor);
                template->floor = temp2;
                ++template->num_colors;
                template->num_links = new_chain;
                free(tetrominoes);
                free(temp);
                free(color_order);
                return 1;
            }

        }
    }
    free(temp);
    free(tetrominoes);
    free(temp2);
    free(color_order);
    return 0;
}

int tail_bottom_chain(bottom_template *template) {
    int num_colors = template->num_colors;
    puyos_t *floor = template->floor;
    puyos_t all = 0;
    for (int i = 0; i < num_colors; ++i) {
        all |= floor[i];
    }

    puyos_t *temp = malloc((num_colors + 1) * sizeof(puyos_t));

    int n = NUM_TOP_TETROMINOES;
    puyos_t *tetrominoes = malloc(n * sizeof(puyos_t));
    memcpy(tetrominoes, TOP_TETROMINOES, n * sizeof(puyos_t));
    shuffle(tetrominoes, n);

    int *color_order = malloc((num_colors + 1) * sizeof(int));
    for (int i = 0; i < n; ++i) {
        if (!(tetrominoes[i] & all)) {
            puyos_t tetromino = tetrominoes[i];
            memcpy(temp, floor, num_colors * sizeof(puyos_t));
            temp[num_colors] = tetromino;
            handle_bottom_gravity(temp, num_colors + 1);
            _reverse_bottom_cut(temp, template->trigger_front, num_colors + 1);
            int new_chain = resolve_bottom(temp, num_colors + 1, color_order);
            if (new_chain > template->num_links) {
                for (int i = 0; i < new_chain; ++i) {
                    int j = color_order[i];
                    if (j < num_colors) {
                        temp[i] = floor[j];
                    } else {
                        temp[i] = tetromino;
                    }
                }
                handle_bottom_gravity(temp, num_colors + 1);
                if (template->trigger_front) {
                    puyos_t new_front = template->trigger_front;
                    for (int i = 0; i < new_chain; ++i) {
                        new_front &= ~temp[i];
                    }
                    if (!new_front) {
                        continue;
                    }
                    template->trigger_front = new_front;
                }
                free(template->floor);
                template->floor = temp;
                ++template->num_colors;
                template->num_links = new_chain;
                free(tetrominoes);
                free(color_order);
                return 1;
            }

        }
    }
    free(temp);
    free(tetrominoes);
    free(color_order);
    return 0;
}

int spam_bottom(bottom_template *template) {
    puyos_t *floor = template->floor;
    int num_colors = template->num_colors;
    handle_bottom_gravity(floor, num_colors);
    puyos_t all = 0;
    for (int i = 0; i < num_colors; ++i) {
        all |= floor[i];
    }
    all |= beam_up(template->trigger_front);
    int free_space = WIDTH * HEIGHT - popcount(all);
    int j = num_colors;
    template->num_colors += free_space;
    floor = realloc(floor, template->num_colors * sizeof(puyos_t));
    template->floor = floor;
    for (int i = 0; i < WIDTH * HEIGHT; ++i) {
        puyos_t p = 1ULL << i;
        if (!(p & all)) {
            floor[j++] = p;
        }
    }
    return free_space;
}

int sprinkle_bottom(bottom_template *template) {
    puyos_t *floor = template->floor;
    int num_colors = template->num_colors;
    handle_bottom_gravity(floor, num_colors);
    puyos_t all = 0;
    for (int i = 0; i < num_colors; ++i) {
        all |= floor[i];
    }
    puyos_t front = template->trigger_front;
    if (template->trigger_front) {
        puyos_t gaps = beam_down(front) & ~(all | front);
        all |= beam_up(front) & ~beam_up(gaps);
        for (int i = 0; i < WIDTH; ++i) {
            puyos_t slice = beam_down(1ULL << i) & front;
            if (popcount(slice) > 1) {
                all ^= beam_up(slice);
            }
        }
    }
    int free_space = popcount(TOP & ~all);
    int j = num_colors;
    template->num_colors += free_space;
    floor = realloc(floor, template->num_colors * sizeof(puyos_t));
    template->floor = floor;
    for (int i = 0; i < WIDTH; ++i) {
        puyos_t p = 1ULL << i;
        if (!(p & all)) {
            floor[j++] = p;
        }
    }
    handle_bottom_gravity(floor, template->num_colors);
    return free_space;
}

int check_assignments(bottom_template *template, int *assignments) {
    int num_colors = template->num_colors;
    for (int i = 0; i < num_colors; ++i) {
        for (int j = i + 1; j < num_colors; ++j) {
            if (assignments[i] == assignments[j] && template->conflicts[i + j * num_colors]) {
                return 0;
            }
        }
    }
    // Need to double check in case of higher level correlations.
    puyos_t *temp = malloc(num_colors * sizeof(puyos_t));
    memcpy(temp, template->floor, num_colors * sizeof(puyos_t));
    int chain = resolve_bottom(temp, num_colors, NULL);
    memset(temp, 0, num_colors * sizeof(puyos_t));
    for (int i = 0; i < num_colors; ++i) {
        temp[assignments[i]] |= template->floor[i];
    }
    int new_chain = resolve_bottom(temp, num_colors, NULL);
    free(temp);
    return new_chain == chain;
}

int _assign(bottom_template *template, int *assignments, int index, int num) {
    int num_colors = template->num_colors;
    if (index >= num_colors) {
        return check_assignments(template, assignments);
    }
    int bound = num;
    if (index + 1 < bound) {
        bound = index + 1;
    }
    for (int i = 0; i < bound; ++i) {
        // Short circuit for the most common conflict.
        if (assignments[index - 1] == i && template->conflicts[index - 1 + index * num_colors]) {
            continue;
        }
        assignments[index] = i;
        int valid = _assign(template, assignments, index + 1, num);
        if (valid) {
            return 1;
        }
    }
    return 0;
}

int* minimum_assignments(bottom_template *template, int *min_colors) {
    int *assignments = malloc(template->num_colors * sizeof(int));
    for (int num = 1; num <= template->num_colors; ++num) {
        assignments[0] = 0;
        if (_assign(template, assignments, 1, num)) {
            *min_colors = num;
            return assignments;
        }
    }
    assert(0);
}

state* state_from_bottom(bottom_template *template) {
    assert(template->conflicts);
    if (!template->num_colors) {
        return calloc(1, sizeof(state));
    }
    int num = template->num_colors;
    int *assignments = minimum_assignments(template, &num);
    if (num > NUM_COLORS - 1) {
        free(assignments);
        return NULL;
    }
    state *s= calloc(1, sizeof(state));
    for (int i = 0; i < template->num_colors; ++i) {
        s->floors[1][assignments[i]] |= template->floor[i];
    }
    free(assignments);
    return s;
}

bottom_match_result match_bottom(state *s, bottom_template *template) {
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
    puyos_t on_spam = 0;
    puyos_t on_single_conflicts = 0;
    for (int j = 0; j < num_colors; ++j) {
        all_template |= template->floor[j];
        if (j < num_links) {
            all_chain |= template->floor[j];
        }
    }
    for (int i = 0; i < NUM_COLORS - 1; ++i) {
        all |= floor[i];
        for (int j = 0; j < num_colors; ++j) {
            all_template |= template->floor[j];
            puyos_t overlap = template->floor[j] & floor[i];
            if (overlap) {
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
                on_chain |= overlap;
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
                // spam vs. spam conflicts ignored.
            }
        }
    }
    free(assignments);

    puyos_t on_trigger_front = all & template->trigger_front;

    int num_on_top = 0;
    for (int i = 0; i < NUM_COLORS - 1; ++i) {
        num_on_top += popcount(s->floors[0][i]);
    }

    return (bottom_match_result) {
        .all = all,
        .all_template = all_template,
        .all_chain = all_chain,
        .on_chain = on_chain,
        .on_spam = on_spam,
        .off_template = (all & ~all_template),
        .on_trigger_front = on_trigger_front,
        .on_single_conflicts = on_single_conflicts,
        .num_color_conflicts = num_color_conflicts,
        .num_spam_conflicts = num_spam_conflicts,
        .num_on_top = num_on_top,
    };
}

float bottom_match_score(bottom_template *template, bottom_match_result result) {
    float penalty = 0;
    penalty += 1.2 * result.num_color_conflicts;
    penalty += popcount(result.on_single_conflicts);
    penalty += 0.5 * result.num_spam_conflicts;
    if (result.on_trigger_front == template->trigger_front) {
        penalty += 1.3;
    }

    float minor_penalty = 0;
    minor_penalty += 0.05 * popcount(result.off_template);
    minor_penalty += 0.08 * popcount(result.on_spam);
    minor_penalty += 0.09 * popcount(result.on_trigger_front) / (float) popcount(template->trigger_front);
    minor_penalty += 0.04 * result.num_on_top;

    float score;
    if (!template->weights) {
        score = popcount(result.on_chain) / (float) popcount(result.all_chain);
    } else {
        float weight = 0;
        float total_mass = 0;
        for (int i = 0; i < template->num_links; ++i) {
            weight += popcount(template->floor[i] & result.on_chain) * template->weights[i];
            total_mass += popcount(template->floor[i]) * template->weights[i];
        }
        score = weight / total_mass;
    }
    score -= minor_penalty;

    if (penalty) {
        return score - penalty - 2;
    }
    return score;
}

puyos_t cut_bottom_trigger(bottom_template *template) {
    assert(template->num_links > 0);
    puyos_t *floor = template->floor;

    puyos_t cut = 0;
    puyos_t rest = 0;
    for (int j = 1; j < template->num_links; ++j) {
        rest |= floor[j];
    }
    int success = 0;
    for (int i = 0; i < WIDTH * HEIGHT; ++i) {
        puyos_t p = 1ULL << i;
        if (p & floor[0]) {
            p = beam_up(p);
            if (!(p & rest)) {
                cut |= floor[0] & p;
                success = 1;
            }
        }
    }
    if (!success) {
        return 0;
    }
    template->floor[0] &= ~cut;
    handle_bottom_gravity(floor, template->num_colors);
    template->trigger_front = cross(floor[0]) & ~(rest | floor[0]);
    return cut;
}

puyos_t reverse_bottom_cut(bottom_template *template) {
    puyos_t reverse_cut = _reverse_bottom_cut(template->floor, template->trigger_front, template->num_colors);
    if (reverse_cut) {
        template->trigger_front = 0;
    }
    return reverse_cut;
}

void prepare_bottom_template(bottom_template *template) {
    assert(template->trigger_front);
    sprinkle_bottom(template);
    reverse_bottom_cut(template);
    template->conflicts = color_conflicts(template->floor, template->num_colors);
    cut_bottom_trigger(template);
}
