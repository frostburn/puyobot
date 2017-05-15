#define NUM_CHOICES (22)
#define CHOICE_0 (0)
#define CHOICE_90 (64)
#define CHOICE_180 (128)
#define CHOICE_270 (192)
#define CHOICE_X_MASK (0x3f)
#define COLOR1_MASK (15)
#define COLOR2_SHIFT (4)


typedef unsigned char content_t;
typedef unsigned char num_t;

typedef struct value_node
{
    float value;
    struct dealt_node *deals;
    num_t num_deals;
} value_node;

typedef struct dealt_node
{
    content_t content;
    float probability;
    struct choice_branch *choices;
    num_t num_choices;
} dealt_node;

typedef struct choice_branch
{
    content_t content;
    float probability;
    size_t visits;
    struct value_node *destination;
} choice_branch;

typedef float (*eval_fun)(state *s);

static content_t CHOICES[NUM_CHOICES] = {
    0 | CHOICE_0, 1 | CHOICE_0, 2 | CHOICE_0, 3 | CHOICE_0, 4 | CHOICE_0, 5 | CHOICE_0,
    0 | CHOICE_90, 1 | CHOICE_90, 2 | CHOICE_90, 3 | CHOICE_90, 4 | CHOICE_90,
    0 | CHOICE_180, 1 | CHOICE_180, 2 | CHOICE_180, 3 | CHOICE_180, 4 | CHOICE_180, 5 | CHOICE_180,
    0 | CHOICE_270, 1 | CHOICE_270, 2 | CHOICE_270, 3 | CHOICE_270, 4 | CHOICE_270,
};

content_t make_piece(content_t color1, content_t color2) {
    return color1 | (color2 << COLOR2_SHIFT);
}

content_t rand_piece() {
    return make_piece(rand() % (NUM_COLORS - 1), rand() % (NUM_COLORS - 1));
}

int apply_deal_and_choice(state *s, content_t deal, content_t choice) {
    content_t color1 = deal & COLOR1_MASK;
    content_t color2 = deal >> COLOR2_SHIFT;
    content_t orientation = choice & ~CHOICE_X_MASK;
    content_t color1_x = choice & CHOICE_X_MASK;
    content_t color2_x = color1_x;
    content_t color1_y = 0;
    content_t color2_y = 1;
    if (orientation == CHOICE_90) {
        color2_x++;
        color2_y--;
    } else if (orientation == CHOICE_180) {
        color1_y++;
        color2_y--;
    } else if (orientation == CHOICE_270) {
        color1_x++;
        color2_y--;
    }
    puyos_t all = 0;
    for (int i = 0; i < NUM_COLORS; ++i) {
        all |= s->floors[0][i];
    }
    puyos_t puyo1 = 1ULL << (color1_x + V_SHIFT * color1_y);
    s->floors[0][color1] |= puyo1;
    puyos_t puyo2 = 1ULL << (color2_x + V_SHIFT * color2_y);
    s->floors[0][color2] |= puyo2;

    if (
        ((1ULL << (color1_x + V_SHIFT * GHOST_Y)) & all) &&
        ((1ULL << (color2_x + V_SHIFT * GHOST_Y)) & all)
    ) {
        return 0;
    }
    return 1;
}

// Deterministic deals
void append_deals(value_node *root, content_t *deals, size_t num_deals) {
    if (!num_deals) {
        return;
    }
    root->num_deals = 1;
    root->deals = calloc(root->num_deals, sizeof(dealt_node));
    root->deals[0].content = deals[0];
    root->deals[0].probability = 1;
    root->deals[0].num_choices = NUM_CHOICES;
    root->deals[0].choices = calloc(root->deals[0].num_choices, sizeof(choice_branch));
    for (num_t k = 0; k < root->deals[0].num_choices; ++k) {
        root->deals[0].choices[k].content = CHOICES[k];
        root->deals[0].choices[k].probability = 1.0 / NUM_CHOICES;
        root->deals[0].choices[k].destination = calloc(1, sizeof(value_node));
        append_deals(root->deals[0].choices[k].destination, deals + 1, num_deals - 1);
    }
}

// Probabilistic deals
void expand(value_node *root) {
    if (root->num_deals == 0) {
        root->num_deals = NUM_COLORS - 1 + ((NUM_COLORS - 1) * (NUM_COLORS - 2)) / 2;
        root->deals = calloc(root->num_deals, sizeof(dealt_node));
        num_t i = 0;
        for (num_t j = 0; j < NUM_COLORS - 1; ++j) {
            for (num_t k = 0; k < NUM_COLORS - 1; ++k) {
                // Symmetry reduction
                if (k > j) {
                    continue;
                }
                root->deals[i].content = make_piece(j, k);
                if (j == k) {
                    root->deals[i].probability = 1.0;
                } else {
                    root->deals[i].probability = 2.0;
                }
                root->deals[i].probability /= (NUM_COLORS - 1) * (NUM_COLORS - 1);
                root->deals[i].num_choices = NUM_CHOICES;
                root->deals[i].choices = calloc(root->deals[i].num_choices, sizeof(choice_branch));
                for (num_t k = 0; k < root->deals[i].num_choices; ++k) {
                    root->deals[i].choices[k].content = CHOICES[k];
                    root->deals[i].choices[k].probability = 1.0 / NUM_CHOICES;
                    root->deals[i].choices[k].destination = calloc(1, sizeof(value_node));
                }
                ++i;
            }
        }
        assert(i == root->num_deals);
    } else {
        for (num_t i = 0; i < root->num_deals; ++i) {
            for (num_t k = 0; k < root->deals[i].num_choices; ++k) {
                expand(root->deals[i].choices[k].destination);
            }
        }
    }
}

float evaluate(state *s, value_node *root, eval_fun f) {
    if (root->num_deals == 0) {
        return f(s);
    }
    root->value = 0;
    for (num_t j = 0; j < root->num_deals; ++j) {
        float deal_value = 0;
        num_t num_best = 0;
        for (num_t k = 0; k < root->deals[j].num_choices; ++k) {
            state *child = copy_state(s);
            int legal = apply_deal_and_choice(child, root->deals[j].content, root->deals[j].choices[k].content);
            if (legal) {
                float current_value = resolve(child, NULL);
                float future_value = evaluate(child, root->deals[j].choices[k].destination, f);
                if (current_value > future_value) {
                    root->deals[j].choices[k].destination->value = current_value;
                } else {
                    root->deals[j].choices[k].destination->value = future_value;
                }
            } else {
                root->deals[j].choices[k].destination->value = -1;
            }
            free(child);
            if (root->deals[j].choices[k].destination->value > deal_value) {
                deal_value = root->deals[j].choices[k].destination->value;
                num_best = 1;
            } else if (root->deals[j].choices[k].destination->value == deal_value) {
                ++num_best;
            }
        }
        for (num_t k = 0; k < root->deals[j].num_choices; ++k) {
            if (root->deals[j].choices[k].destination->value == deal_value) {
                root->deals[j].choices[k].probability = 1.0 / num_best;
            } else {
                root->deals[j].choices[k].probability = 0;
            }
        }
        root->value += root->deals[j].probability * deal_value;
    }
    return root->value;
}

content_t best_choice(value_node *root) {
    if (root->num_deals != 1) {
        return 0;
    }
    content_t best_action = 0;
    float highest_probability = 0;
    for (num_t i = 0; i < root->deals->num_choices; ++i) {
        if (root->deals->choices[i].probability > highest_probability) {
            highest_probability = root->deals->choices[i].probability;
            best_action = root->deals->choices[i].content;
        }
    }
    return best_action;
}

content_t choose(value_node *root) {
    if (root->num_deals != 1) {
        return 0;
    }
    double prob = drand();
    for (num_t i = 0; i < root->deals->num_choices; ++i) {
        prob -= root->deals->choices[i].probability;
        if (prob <= 0) {
            return root->deals->choices[i].content;
        }
    }
    return 0;
}

void free_tree(value_node *root) {
    for (num_t i = 0; i < root->num_deals; ++i) {
        for (num_t k = 0; k < root->deals[i].num_choices; ++k) {
            free_tree(root->deals[i].choices[k].destination);
        }
        free(root->deals[i].choices);

    }
    free(root->deals);
    free(root);
}

float eval_fun_zero(state *s) {
    return 0;
}

float eval_fun_random(state *s) {
    state *c;
    int total_score = 0;
    for (int i = 0; i < 10; ++i) {
        c = copy_state(s);
        for (int j = 0; j < 25; ++j) {
            if(!apply_deal_and_choice(c, rand_piece(), CHOICES[rand() % NUM_CHOICES])) {
                break;
            }
            total_score += resolve(c, NULL);
        }
        free(c);
    }
    return total_score * 0.1;
}

float eval_fun_weighted(state *s) {
    state *c;
    double total_score = 0;
    double total_weight = 0;
    for (int i = 0; i < 10; ++i) {
        c = copy_state(s);
        for (int j = 0; j < 25; ++j) {
            if(!apply_deal_and_choice(c, rand_piece(), CHOICES[rand() % NUM_CHOICES])) {
                break;
            }
            double score = resolve(c, NULL);
            int num_remaining = 0;
            for (int j = 0; j < NUM_FLOORS; ++j) {
                for (int i = 0; i < NUM_COLORS; ++i) {
                    num_remaining += popcount(c->floors[j][i]);
                }
            }
            score += 1.5 * exp(-0.3 * num_remaining);
            double weight = exp(-state_euler(c));
            total_weight += weight;
            total_score += score * weight;
        }
        free(c);
    }
    return total_score / total_weight;
}

content_t solve(state *s, content_t *deals, size_t num_deals, size_t depth, eval_fun f) {
    value_node *root = calloc(1, sizeof(value_node));
    append_deals(root, deals, num_deals);
    for (size_t i = 0; i < depth; ++i) {
        expand(root);
    }
    evaluate(s, root, f);
    content_t choice = choose(root);
    free_tree(root);
    return choice;
}

#include "montecarlo.c"