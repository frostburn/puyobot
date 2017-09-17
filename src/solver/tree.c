#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <omp.h>

#include "jkiss/util.h"

#include "puyobot/state.h"
#include "puyobot/solver/tree.h"
#include "puyobot/util.h"

#define MC_DEATH_SCORE_TREE (-1)
#define MC_DEATH_SCORE_PLAYOUT (-100)

void print_tree_node(TreeNode *root, int show_children) {
    printf("TreeNode visits=%llu score=%f\n", root->visits, root->score);
    if (!show_children) {
        return;
    }
    for (int i = 0; i < root->num_deals; ++i) {
        print_deals(&root->deals[i].content, 1);
        for (int j = 0; j < root->deals[i].num_choices; ++j) {
            ChoiceNode *c = root->deals[i].choices + j;
            printf(" %d visits=%llu score=%f\n", c->content, c->target.visits, c->target.score);
        }
    }
}

ChoiceNode* mc_tree_choose(DealNode *node, McOptions options) {
    double factor = 0;
    for (int i = 0; i < node->num_choices; ++i) {
        factor += node->choices[i].target.visits;
    }
    factor = sqrt(log(factor)) * options.exploration;
    double best = -INFINITY;
    unsigned int best_choices = 0;
    for (int i = 0; i < node->num_choices; ++i) {
        double visits = node->choices[i].target.visits;
        assert(visits);
        double value = node->choices[i].target.score / visits + factor / sqrt(visits);
        if (value > best) {
            best = value;
            best_choices = 1 << i;
        } else if (value == best) {
            best_choices |= 1 << i;
        }
    }
    return node->choices + bitset_rand_index(best_choices);
}

TreeNode* mc_tree_policy(void *state, TreeNode *root, McOptions options, double *result) {
    if (!root->num_deals) {
        return root;
    }
    double weights[NUM_REDUCED_DEALS * NUM_REDUCED_DEALS];
    for (int i = 0; i < root->num_deals; ++i) {
        weights[i] = root->deals[i].probability;
    }
    size_t index = choose_weighted(weights, root->num_deals);
    ChoiceNode *choice = mc_tree_choose(root->deals + index, options);

    double score;
    int valid = options.step(state, root->deals[index].content, choice->content, &score);
    if (!valid) {
        *result = MC_DEATH_SCORE_TREE;
        return root;
    }
    if (score > options.score_threshold) {
        *result = score;
        return root;
    }
    // XXX: Should we collect sub-threshold score too?
    return mc_tree_policy(state, &choice->target, options, result);
}

double mc_playout(void *state, McOptions options) {
    #define _MAX_DEALS (3)
    content_t deals[_MAX_DEALS];
    for (int i = 0; i < _MAX_DEALS; ++i) {
        deals[i] = rand_piece();
    }
    double score;
    while (1) {
        content_t choice = options.policy(state, deals, _MAX_DEALS);
        int valid = options.step(state, deals[0], choice, &score);
        if (!valid) {
            return MC_DEATH_SCORE_PLAYOUT;
        }
        if (score > options.score_threshold) {
            return score;
        }
        for (int i = 1; i < _MAX_DEALS; ++i) {
            deals[i - 1] = deals[i];
        }
        deals[_MAX_DEALS - 1] = rand_piece();
    }
}

void mc_expand(void *state, TreeNode *root, McOptions options) {
    root->num_deals = NUM_REDUCED_DEALS;
    root->deals = malloc(root->num_deals * sizeof(DealNode));
    DealNode *deal = root->deals;
    for (int a = 0; a < NUM_DEAL_COLORS; ++a) {
        for (int b = a; b < NUM_DEAL_COLORS; ++b) {
            deal->probability = (a == b) ? 2 : 1;
            deal->probability /= NUM_REDUCED_DEALS;
            deal->content = make_piece(a, b);

            deal->num_choices = 0;
            deal->choices = calloc(NUM_CHOICES, sizeof(ChoiceNode));
            for (int i = 0; i < NUM_CHOICES; ++i) {
                void *copy = options.copy(state);
                double score;
                int valid = options.step(copy, deal->content, CHOICES[i], &score);
                options.delete(copy);
                if (!valid) {
                    continue;
                }
                ChoiceNode *choice = deal->choices + deal->num_choices++;
                choice->content = CHOICES[i];
                choice->target.parent = root;
                choice->target.score = 0;
                choice->target.visits = 1;
            }
            if (!deal->num_choices) {
                deal->choices[0].target.score = 0;
                deal->choices[0].target.visits = 1000000;
                deal->choices[0].target.parent = root;
                deal->choices[0].content = CHOICE_PASS;
                deal->num_choices = 1;
            }
            deal->choices = realloc(deal->choices, deal->num_choices * sizeof(ChoiceNode));
            deal++;
        }
    }
}

void mc_single_round(void *state, TreeNode *root, McOptions options) {
    double result = NAN;
    TreeNode *leaf = mc_tree_policy(state, root, options, &result);

    count_t visits = 1;
    if (isnan(result)) {
        mc_expand(state, leaf, options);
        #ifdef USE_OMP_FOR_MC_PLAYOUT
            visits = 0;
            result = 0;
            #pragma omp parallel
            {
                void *copy = options.copy(state);
                double temp = mc_playout(copy, options);
                options.delete(copy);
                #pragma omp critical
                {
                    result += temp;
                    ++visits;
                }
            }
        #else
            result = mc_playout(state, options);
            visits = 1;
        #endif
    }

    TreeNode *node = leaf;
    do {
        node->score += result;
        node->visits += visits;
        node = node->parent;
    } while(node);
}

void mc_init_target(void *state, TreeNode *root, content_t *deals, int num_deals, McOptions options) {
    root->score = 0;
    root->visits = 1;
    if (!num_deals) {
        return;
    }
    root->deals = malloc(sizeof(DealNode));
    root->num_deals = 1;
    root->deals->probability = 1;
    root->deals->content = deals[0];

    root->deals->num_choices = 0;
    root->deals->choices = calloc(NUM_CHOICES, sizeof(ChoiceNode));
    for (int i = 0; i < NUM_CHOICES; ++i) {
        void *copy = options.copy(state);
        double score;
        int valid = options.step(copy, root->deals->content, CHOICES[i], &score);
        if (!valid) {
            options.delete(copy);
            continue;
        }
        ChoiceNode *choice = root->deals->choices + root->deals->num_choices++;
        choice->target.parent = root;
        choice->content = CHOICES[i];
        mc_init_target(copy, &choice->target, deals + 1, num_deals - 1, options);
        options.delete(copy);
    }
    if (!root->deals->num_choices) {
        root->deals->choices[0].target.score = 0;
        root->deals->choices[0].target.visits = 1000000;
        root->deals->choices[0].target.parent = root;
        root->deals->choices[0].content = CHOICE_PASS;
        root->deals->num_choices = 1;
    }
    root->deals->choices = realloc(root->deals->choices, root->deals->num_choices * sizeof(ChoiceNode));
}

TreeNode* mc_init(void *state, content_t *deals, int num_deals, McOptions options) {
    TreeNode *root = calloc(1, sizeof(TreeNode));
    mc_init_target(state, root, deals, num_deals, options);
    return root;
}

void mc_iterate(void *state, TreeNode *root, size_t iterations, McOptions options) {
    for (size_t i = 0; i < iterations; ++i) {
        void *copy = options.copy(state);
        mc_single_round(copy, root, options);
        options.delete(copy);
    }
}

content_t mc_choose(TreeNode *root) {
    assert(root->num_deals == 1);
    assert(root->deals->num_choices);
    McOptions options;
    options.exploration = 0;
    ChoiceNode *node = mc_tree_choose(root->deals, options);
    return node->content;
}

McOptions get_mc_options(policy_fun policy) {
    McOptions options;
    options.exploration = 1000;
    options.score_threshold = 90;
    options.delete = free;
    options.copy = copy_state;
    options.step = step_state;
    options.policy = policy;
    return options;
}

void mc_free_(TreeNode *root) {
    for (int i = 0; i < root->num_deals; ++i) {
        for (int j = 0; j < root->deals[i].num_choices; ++j) {
            mc_free_(&root->deals[i].choices[j].target);
        }
        free(root->deals[i].choices);
    }
    free(root->deals);
}

void mc_free(TreeNode *root) {
    mc_free_(root);
    free(root);
}
