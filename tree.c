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
    struct value_node *destination;
} choice_branch;

static content_t CHOICES[NUM_CHOICES] = {
    0 | CHOICE_0, 1 | CHOICE_0, 2 | CHOICE_0, 3 | CHOICE_0, 4 | CHOICE_0, 5 | CHOICE_0,
    0 | CHOICE_90, 1 | CHOICE_90, 2 | CHOICE_90, 3 | CHOICE_90, 4 | CHOICE_90,
    0 | CHOICE_180, 1 | CHOICE_180, 2 | CHOICE_180, 3 | CHOICE_180, 4 | CHOICE_180, 5 | CHOICE_180,
    0 | CHOICE_270, 1 | CHOICE_270, 2 | CHOICE_270, 3 | CHOICE_270, 4 | CHOICE_270,
};

content_t make_piece(content_t color1, content_t color2) {
    return color1 | (color2 << COLOR2_SHIFT);
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
    puyos_t puyo1 = 1ULL << (color1_x + V_SHIFT * color1_y);
    if (s->floors[0][color1] & puyo1) {
        return -1;
    } else {
        s->floors[0][color1] |= puyo1;
    }
    puyos_t puyo2 = 1ULL << (color2_x + V_SHIFT * color2_y);
    if (s->floors[0][color2] & puyo2) {
        return -1;
    } else {
        s->floors[0][color2] |= puyo2;
    }
    return resolve(s);
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
        root->num_deals = NUM_COLORS * NUM_COLORS;
        root->deals = calloc(root->num_deals, sizeof(dealt_node));
        for (num_t i = 0; i < root->num_deals; ++i) {
            root->deals[i].content = make_piece(i % NUM_COLORS, i / NUM_COLORS);
            root->deals[i].probability = 1.0 / root->num_deals;
            root->deals[i].num_choices = NUM_CHOICES;
            root->deals[i].choices = calloc(root->deals[i].num_choices, sizeof(choice_branch));
            for (num_t k = 0; k < root->deals[i].num_choices; ++k) {
                root->deals[i].choices[k].content = CHOICES[k];
                root->deals[i].choices[k].probability = 1.0 / NUM_CHOICES;
                root->deals[i].choices[k].destination = calloc(1, sizeof(value_node));
            }
        }
    } else {
        for (num_t i = 0; i < root->num_deals; ++i) {
            for (num_t k = 0; k < root->deals[i].num_choices; ++k) {
                expand(root->deals[i].choices[k].destination);
            }
        }
    }
}

float evaluate(state *s, value_node *root) {
    root->value = 0;
    for (num_t j = 0; j < root->num_deals; ++j) {
        float deal_value = 0;
        num_t num_best = 0;
        for (num_t k = 0; k < root->deals[j].num_choices; ++k) {
            state *child = copy_state(s);
            float current_value = apply_deal_and_choice(child, root->deals[j].content, root->deals[j].choices[k].content);
            float future_value = evaluate(child, root->deals[j].choices[k].destination);
            if (current_value > future_value) {
                root->deals[j].choices[k].destination->value = current_value;
            } else {
                root->deals[j].choices[k].destination->value = future_value;
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
    double prob = rand() / ((double) RAND_MAX);
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

content_t solve(state *s, content_t *deals, size_t num_deals, size_t depth) {
    value_node *root = calloc(1, sizeof(value_node));
    append_deals(root, deals, num_deals);
    for (size_t i = 0; i < depth; ++i) {
        expand(root);
    }
    evaluate(s, root);
    content_t choice = choose(root);
    free_tree(root);
    return choice;
}