#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "jkiss/jkiss.h"

#include "puyobot/util.h"
#include "puyobot/bottom.h"
#include "puyobot/template/pattern.h"
#include "puyobot/template/bottom.h"

void free_bottom_template(BottomTemplate *template) {
    free(template->floor);
    free(template->conflicts);
    free(template->weights);
    free(template);
}

void print_bottom_template(BottomTemplate *template) {
    if (template->conflicts) {
        print_conflicts(template->conflicts, template->num_colors);
    }
    if (template->weights) {
        for (int i = 0; i < template->num_links; ++i) {
            printf("%d: %f\n", i, template->weights[i]);
        }
    }
    if (template->trigger_front) {
        print_puyos(template->trigger_front);
    }
    print_bottom_spam(template->floor, template->num_links, template->num_colors);
}

void repr_bottom_template_floor(BottomTemplate *template) {
    printf("int num_links = %d;\n", template->num_links);
    printf("int num_colors = %d;\n", template->num_colors);
    printf("puyos_t floor[] = {");
    for (int i = 0; i < template->num_colors; ++i) {
        printf("%lluull, ", template->floor[i]);
    }
    printf("\b\b};\n");
}

BottomTemplate* copy_bottom_template(BottomTemplate *template) {
    BottomTemplate *copy = malloc(sizeof(BottomTemplate));
    *copy = *template;
    copy->floor = malloc(copy->num_colors * sizeof(puyos_t));
    memcpy(copy->floor, template->floor, copy->num_colors * sizeof(puyos_t));
    copy->conflicts = NULL;
    copy->weights = NULL;
    return copy;
}

BottomTemplate* template_from_floor(puyos_t *floor, int num_links) {
    BottomTemplate *template = calloc(1, sizeof(BottomTemplate));
    template->floor = floor;
    template->num_links = num_links;
    template->num_colors = num_links;
    return template;
}

BottomTemplate* bottom_chain_of_fours(int num_links) {
    puyos_t *floor = malloc(num_links * sizeof(puyos_t));
    puyos_t *temp = malloc(num_links * sizeof(puyos_t));
    int *color_order = malloc(num_links * sizeof(int));
    while (1) {
        memset(floor, 0, num_links * sizeof(puyos_t));
        puyos_t allowed = FULL;
        for (int k = 0; k < num_links; ++k) {
            int j = CLEAR_THRESHOLD;
            while (j) {
                puyos_t p = 1ULL << (jrand() % (WIDTH * HEIGHT));
                if (p & allowed) {
                    floor[k] |= p;
                    allowed ^= p;
                    --j;
                }
            }
            if (gap_size(floor[k])) {
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
    free(color_order);
    BottomTemplate *template = calloc(1, sizeof(BottomTemplate));
    template->floor = floor;
    template->num_links = num_links;
    template->num_colors = num_links;
    return template;
}

int extend_bottom_chain(BottomTemplate *template, puyos_t fixed, int allow_cuts) {
    assert(TETROMINOES_INITIALIZED);
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
    shuffle(tetrominoes, n, sizeof(puyos_t));

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
                for (int k = 0; k < num_colors + 1; ++k) {
                    int j = color_order[k] - 1;
                    if (j >= 0) {
                        temp2[k] = temp[j];
                    } else {
                        temp2[k] = tetromino;
                    }
                }
                puyos_t gap = 0;
                if (allow_cuts) {
                    puyos_t all = 0;
                    for (int k = 0; k < num_colors + 1; ++k) {
                        all |= temp2[k];
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
                    for (int k = 1; k < num_colors + 1; ++k) {
                        rest |= temp2[k];
                    }
                    for (int k = 0; k < WIDTH * HEIGHT; ++k) {
                        puyos_t p = 1ULL << k;
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

int tail_bottom_chain(BottomTemplate *template) {
    assert(TETROMINOES_INITIALIZED);
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
    shuffle(tetrominoes, n, sizeof(puyos_t));

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
                for (int k = 0; k < new_chain; ++k) {
                    int j = color_order[k];
                    if (j < num_colors) {
                        temp[k] = floor[j];
                    } else {
                        temp[k] = tetromino;
                    }
                }
                handle_bottom_gravity(temp, num_colors + 1);
                if (template->trigger_front) {
                    puyos_t new_front = template->trigger_front;
                    for (int k = 0; k < new_chain; ++k) {
                        new_front &= ~temp[k];
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

int spam_bottom_aura(BottomTemplate *template) {
    assert(template->num_links == template->num_colors);
    puyos_t all = 0;
    for (int i = 0; i < template->num_links; ++i) {
        all |= template->floor[i];
    }
    puyos_t aura = cross(all) ^ all;
    int num_spam = popcount(aura);
    template->num_colors += num_spam;
    template->floor = realloc(template->floor, template->num_colors * sizeof(puyos_t));
    int j = template->num_links;
    for (int i = 0; i < WIDTH * HEIGHT; ++i) {
        puyos_t p = 1ULL << i;
        if (p & aura) {
            template->floor[j++] = p;
        }
    }
    return num_spam;
}

int spam_bottom(BottomTemplate *template) {
    assert(template->num_links == template->num_colors);
    puyos_t all = 0;
    for (int i = 0; i < template->num_links; ++i) {
        all |= template->floor[i];
    }
    int free_space = WIDTH * HEIGHT - popcount(all);
    template->num_colors += free_space;
    template->floor = realloc(template->floor, template->num_colors * sizeof(puyos_t));
    int j = template->num_links;
    for (int i = 0; i < WIDTH * HEIGHT; ++i) {
        puyos_t p = 1ULL << i;
        if (!(p & all)) {
            template->floor[j++] = p;
        }
    }
    return free_space;
}

int sprinkle_bottom(BottomTemplate *template) {
    // TODO: Allow iterative sprinkles
    handle_bottom_gravity(template->floor, template->num_colors);
    puyos_t all = 0;
    for (int i = 0; i < template->num_links; ++i) {
        all |= template->floor[i];
    }
    puyos_t top = TOP & ~all;
    int num_spam = popcount(top);
    template->num_colors += num_spam;
    template->floor = realloc(template->floor, template->num_colors * sizeof(puyos_t));
    int j = template->num_links;
    for (int i = 0; i < WIDTH; ++i) {
        puyos_t p = 1ULL << i;
        if (p & top) {
            template->floor[j++] = p;
        }
    }
    handle_bottom_gravity(template->floor, template->num_colors);
    return num_spam;
}

int check_assignments(BottomTemplate *template, int *assignments) {
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

int _assign(BottomTemplate *template, int *assignments, int index, int num) {
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

int* minimum_assignments(BottomTemplate *template, int *min_colors) {
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

State* _state_from_assignments(BottomTemplate *template, int* assignments) {
    State *state = calloc(1, sizeof(State));
    for (int i = 0; i < template->num_colors; ++i) {
        state->floors[1][assignments[i]] |= template->floor[i];
    }
    free(assignments);
    return state;
}

State* min_state_from_bottom(BottomTemplate *template) {
    assert(template->conflicts);
    if (!template->num_colors) {
        return calloc(1, sizeof(State));
    }
    int num = template->num_colors;
    int *assignments = minimum_assignments(template, &num);
    if (num > NUM_COLORS - 1) {
        free(assignments);
        return NULL;
    }
    return _state_from_assignments(template, assignments);
}

State* any_state_from_bottom(BottomTemplate *template, int num_colors, size_t patience) {
    assert(num_colors < NUM_COLORS);
    assert(template->conflicts);
    if (!template->num_colors) {
        return calloc(1, sizeof(State));
    }
    int *assignments = malloc(template->num_colors * sizeof(int));
    for (size_t i = 0; i < patience; ++i) {
        for (int j = 0; j < template->num_colors; ++j) {
            assignments[j] = jrand() % num_colors;
        }
        if (check_assignments(template, assignments)) {
            return _state_from_assignments(template, assignments);
        }
    }
    return NULL;
}

puyos_t reverse_bottom_cut(BottomTemplate *template) {
    puyos_t reverse_cut = _reverse_bottom_cut(template->floor, template->trigger_front, template->num_colors);
    if (reverse_cut) {
        template->trigger_front = 0;
    }
    return reverse_cut;
}

puyos_t cut_bottom_trigger(BottomTemplate *template) {
    if (template->trigger_front) {
        return 0;
    }
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

puyos_t chip_bottom_trigger(BottomTemplate *template) {
    if (template->trigger_front) {
        return 0;
    }
    assert(template->num_links > 0);
    puyos_t *floor = template->floor;

    puyos_t cut = 0;
    puyos_t rest = 0;
    for (int j = 1; j < template->num_links; ++j) {
        rest |= floor[j];
    }
    int success = 0;
    int line_perm[WIDTH] = {0, 1, 2, 3, 4, 5};
    for (int j = 0; j < HEIGHT; ++j) {
        shuffle(line_perm, WIDTH, sizeof(int));
        for (int i = 0; i < WIDTH; ++i) {
            puyos_t p = 1ULL << (line_perm[i] + WIDTH * j);
            if (p & floor[0]) {
                if (!(beam_up(p) & rest)) {
                    cut = p;
                    success = 1;
                    break;
                }
            }
        }
        if (success) {
            break;
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

void calculate_bottom_conflicts(BottomTemplate *template) {
    assert(template->num_links);
    puyos_t original = template->floor[0];
    puyos_t original_front = template->trigger_front;
    reverse_bottom_cut(template);
    char *conflicts = calculate_conflicts(template->floor, template->num_links);
    template->floor[0] = original;
    template->trigger_front = original_front;
    if (template->num_links == template->num_colors) {
        template->conflicts = conflicts;
        return;
    }
    template->conflicts = calloc(template->num_colors * template->num_colors, sizeof(char));
    puyos_t all = 0;
    for (int i = 0; i < template->num_links; ++i) {
        for (int j = 0; j < template->num_links; ++j) {
            template->conflicts[i + template->num_colors * j] = conflicts[i + template->num_links * j];
        }
    }
    free(conflicts);
    for (int i = template->num_links; i < template->num_colors; ++i) {
        puyos_t aura;
        if (template->floor[i] & template->trigger_front) {
            // XXX: Does not catch all cross conflicts.
            aura = blob(template->floor[i]);
        } else {
            aura = cross(template->floor[i]);
        }
        // The trigger is special so we ignore conflicts there.
        for (int j = 1; j < template->num_links; ++j) {
            int conflict = 0;
            if (aura & template->floor[j]) {
                conflict = 1;
            }
            template->conflicts[i + template->num_colors * j] = conflict;
            template->conflicts[j + template->num_colors * i] = conflict;
        }
    }
}

void prepare_bottom_template(BottomTemplate *template, int full_cut) {
    if (full_cut) {
        cut_bottom_trigger(template);
    } else {
        chip_bottom_trigger(template);
    }
    puyos_t cut = template->trigger_front;
    spam_bottom_aura(template);
    calculate_bottom_conflicts(template);
    template->trigger_front = cut;
}
