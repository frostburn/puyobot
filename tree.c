#define NUM_REDUCED_DEALS (NUM_DEAL_COLORS + (NUM_DEAL_COLORS * (NUM_DEAL_COLORS - 1)) / 2)

typedef unsigned char num_t;

typedef struct value_node
{
    float value;
    struct dealt_node *deals;
    num_t num_deals;
    unsigned char evaluated;
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

typedef struct tree_options
{
    copy_fun copy;
    step_fun step;
    eval_fun eval;
    int depth;
    float tree_factor;
} tree_options;

tree_options simple_tree_options(eval_fun eval, int depth, float tree_factor) {
    return (tree_options) {
        .copy = state_copy,
        .step = state_step,
        .eval = eval,
        .depth = depth,
        .tree_factor = tree_factor,
    };
}

// Deterministic deals
void expand_single_deal(value_node *root, content_t *choices, int num_choices) {
    root->evaluated = 0;
    if (root->num_deals == 0) {
        root->num_deals = 1;
        root->deals = calloc(root->num_deals, sizeof(dealt_node));
        root->deals[0].probability = 1;
        root->deals[0].num_choices = num_choices;
        root->deals[0].choices = calloc(num_choices, sizeof(choice_branch));
        for (num_t k = 0; k < num_choices; ++k) {
            root->deals[0].choices[k].content = choices[k];
            root->deals[0].choices[k].probability = 1.0 / num_choices;
            root->deals[0].choices[k].destination = calloc(1, sizeof(value_node));
        }
    } else {
        for (num_t i = 0; i < root->num_deals; ++i) {
            for (num_t k = 0; k < root->deals[i].num_choices; ++k) {
                expand_single_deal(root->deals[i].choices[k].destination, choices, num_choices);
            }
        }
    }
}

void assign_single_deals(value_node *root, content_t *deals, int num_deals) {
    if (!num_deals) {
        return;
    }
    root->evaluated = 0;
    assert(root->num_deals == 1);
    root->deals[0].content = deals[0];
    for (num_t i = 0; i < root->num_deals; ++i) {
        for (num_t k = 0; k < root->deals[i].num_choices; ++k) {
            assign_single_deals(root->deals[i].choices[k].destination, deals + 1, num_deals - 1);
        }
    }
}

// Probabilistic deals
void expand_all_deals(value_node *root, content_t *choices, int num_choices) {
    root->evaluated = 0;
    if (root->num_deals == 0) {
        root->num_deals = NUM_REDUCED_DEALS;
        root->deals = calloc(root->num_deals, sizeof(dealt_node));
        num_t i = 0;
        for (num_t j = 0; j < NUM_DEAL_COLORS; ++j) {
            for (num_t k = 0; k < NUM_DEAL_COLORS; ++k) {
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
                root->deals[i].probability /= NUM_DEAL_COLORS * NUM_DEAL_COLORS;
                root->deals[i].num_choices = num_choices;
                root->deals[i].choices = calloc(num_choices, sizeof(choice_branch));
                for (num_t k = 0; k < num_choices; ++k) {
                    root->deals[i].choices[k].content = choices[k];
                    root->deals[i].choices[k].probability = 1.0 / num_choices;
                    root->deals[i].choices[k].destination = calloc(1, sizeof(value_node));
                }
                ++i;
            }
        }
        assert(i == root->num_deals);
    } else {
        for (num_t i = 0; i < root->num_deals; ++i) {
            for (num_t k = 0; k < root->deals[i].num_choices; ++k) {
                expand_all_deals(root->deals[i].choices[k].destination, choices, num_choices);
            }
        }
    }
}

float evaluate(void *s, value_node *root, tree_options options) {
    if (root->evaluated) {
        return root->value;
    }
    if (root->num_deals == 0) {
        return options.eval(s);
    }
    root->value = 0;
    for (num_t j = 0; j < root->num_deals; ++j) {
        float deal_value = -INFINITY;
        num_t num_best = 0;
        for (num_t k = 0; k < root->deals[j].num_choices; ++k) {
            void *child = options.copy(s);
            double choice_score = options.step(child, root->deals[j].content, root->deals[j].choices[k].content);
            float future_value = evaluate(child, root->deals[j].choices[k].destination, options);
            root->deals[j].choices[k].destination->value = choice_score * options.tree_factor + future_value;
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
    root->evaluated = 1;
    return root->value;
}

#ifdef _OPENMP
    #include <omp.h>
    // Divide first level evaluation between threads.
    // The final evaluations are collected later so we ignore them here.
    void omp_evaluate(void *s, value_node *root, tree_options options) {
        #pragma omp parallel for collapse(2)
        for (num_t j = 0; j < root->num_deals; ++j) {
            for (num_t k = 0; k < root->deals[j].num_choices; ++k) {
                void *child = options.copy(s);
                options.step(child, root->deals[j].content, root->deals[j].choices[k].content);
                evaluate(child, root->deals[j].choices[k].destination, options);
                free(child);
            }
        }
    }
#endif

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

choice_branch** best_choices(value_node *root, int count) {
    if (root->num_deals != 1) {
        return NULL;
    }
    choice_branch **best = malloc(count * sizeof(choice_branch*));
    float *best_scores = malloc(count * sizeof(float));
    for (int i = 0; i < count; ++i) {
        best_scores[i] = -INFINITY;
    }
    for (num_t i = 0; i < root->deals->num_choices; ++i) {
        choice_branch *choice = root->deals->choices + i;
        for (int j = 0; j < count; ++j) {
            if (choice->destination->value > best_scores[j]) {
                for (int k = count - 2; k >= j; --k) {
                    best[k + 1] = best[k];
                    best_scores[k + 1] = best_scores[k];
                }
                best[j] = choice;
                best_scores[j] = choice->destination->value;
                break;
            }
        }
    }
    free(best_scores);
    return best;
}

choice_branch* choose(value_node *root) {
    if (!root || root->num_deals == 0) {
        return NULL;
    }
    if (root->num_deals != 1) {
        return root->deals->choices;
    }
    double prob = jdrand();
    for (num_t i = 0; i < root->deals->num_choices; ++i) {
        prob -= root->deals->choices[i].probability;
        if (prob <= 0) {
            return root->deals->choices + i;
        }
    }
    return root->deals->choices;
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

value_node* solve_tree(void *s, content_t *deals, int num_deals, tree_options options) {
    value_node *root = calloc(1, sizeof(value_node));
    for (int i = 0; i < num_deals; ++i) {
        expand_single_deal(root, CHOICES, NUM_CHOICES);
    }
    assign_single_deals(root, deals, num_deals);
    for (int i = 0; i < options.depth; ++i) {
        expand_all_deals(root, CHOICES, NUM_CHOICES);
    }
    #ifdef _OPENMP
        omp_evaluate(s, root, options);
    #endif
    evaluate(s, root, options);
    return root;
}

content_t solve(void *s, content_t *deals, int num_deals, tree_options options) {
    value_node *root = solve_tree(s, deals, num_deals, options);
    choice_branch *choice = choose(root);
    content_t action = choice->content;
    free_tree(root);
    return action;
}

#include "montecarlo.c"
