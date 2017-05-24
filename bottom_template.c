typedef struct bottom_template
{
    puyos_t *floor;
    puyos_t trigger_front;
    int num_links;
    int num_colors;
    char *conflicts;
    int score;
} bottom_template;

void free_bottom_template(bottom_template *template) {
    free(template->floor);
    free(template->conflicts);
    free(template);
}


#include "bottom.c"

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

int extend_bottom_chain(bottom_template *template) {
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
    for (int i = 0; i < n; ++i) {
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
            temp2[0] = tetromino;
            int new_chain = resolve_bottom(temp2, num_colors + 1, NULL);
            if (new_chain > chain) {
                memcpy(temp2 + 1, temp, num_colors * sizeof(puyos_t));
                temp2[0] = tetromino;
                handle_bottom_gravity(temp2, num_colors + 1);
                free(template->floor);
                template->floor = temp2;
                ++template->num_colors;
                template->num_links = new_chain;
                free(tetrominoes);
                free(temp);
                return 1;
            }

        }
    }
    free(temp);
    free(tetrominoes);
    free(temp2);
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

int _assign(bottom_template *template, int *assignments, int index, int num) {
    int num_colors = template->num_colors;
    if (index >= num_colors) {
        for (int i = 0; i < num_colors; ++i) {
            for (int j = 0; j < num_colors; ++j) {
                if (assignments[i] == assignments[j] && template->conflicts[i + j * num_colors]) {
                    return 0;
                }
            }
        }
        // Need to double check in case of higher level correlations.
        puyos_t *temp = malloc(template->num_colors * sizeof(puyos_t));
        memcpy(temp, template->floor, template->num_colors * sizeof(puyos_t));
        int chain = resolve_bottom(temp, template->num_colors, NULL);
        memset(temp, 0, template->num_colors * sizeof(puyos_t));
        for (int i = 0; i < template->num_colors; ++i) {
            temp[assignments[i]] |= template->floor[i];
        }
        int new_chain = resolve_bottom(temp, num, NULL);
        free(temp);
        return new_chain == chain;
    }
    for (int i = 0; i < num; ++i) {
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
    free(assignments);
    return NULL;
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

double bottom_match_score(state *s, bottom_template *template) {
    assert(template->conflicts);
    int num_colors = template->num_colors;
    int *assignments = malloc(num_colors * sizeof(int));
    for (int j = 0; j < num_colors; ++j) {
        assignments[j] = -1 - j;
    }
    puyos_t chain = 0;
    for (int j = 0; j < template->num_links; ++j) {
        chain |= template->floor[j];
    }
    puyos_t *floor = s->floors[1];
    puyos_t all = 0;
    puyos_t on_chain = 0;
    puyos_t on_spam = 0;
    puyos_t on_single_conflicts = 0;
    for (int i = 0; i < NUM_COLORS - 1; ++i) {
        all |= floor[i];
        for (int j = 0; j < num_colors; ++j) {
            puyos_t overlap = template->floor[j] & floor[i];
            if (overlap) {
                if (assignments[j] >= 0) {
                    on_single_conflicts |= overlap;
                    continue;
                }
                assignments[j] = i;
            }
            if (j < template->num_links) {
                on_chain |= overlap;
            } else {
                on_spam |= overlap;
            }
        }
    }
    int num_color_conflicts = 0;
    for (int i = 0; i < num_colors; ++i) {
        for (int j = 0; j < num_colors; ++j) {
            if (assignments[i] == assignments[j] && template->conflicts[i + j * num_colors]) {
                num_color_conflicts++;
            }
        }
    }
    double penalty = 0.0;
    penalty += popcount(on_single_conflicts) * 0.95;
    penalty += num_color_conflicts * 0.97;
    penalty += popcount(all & template->trigger_front) * 0.123;
    free(assignments);
    return popcount(on_chain) / (double)popcount(chain);
}

int cut_bottom_trigger(bottom_template *template) {
    puyos_t *floor = template->floor;
    int success = 0;
    for (int i = 0; i < WIDTH * HEIGHT; ++i) {
        puyos_t p = 1ULL << i;
        if (p & floor[0]) {
            p = beam_up(p);
            for (int j = 1; j < template->num_links; ++j) {
                if (p & floor[j]) {
                    p = 0;
                    break;
                }
            }
            if (p) {
                floor[0] &= ~p;
                for (int j = template->num_links; j < template->num_colors; ++j) {
                    floor[j] &= ~p;
                }
                template->trigger_front |= p;
                success = 1;
            }
        }
    }
    return success;
}
