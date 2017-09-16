#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "jkiss/util.h"

#include "puyobot/solver/simul_tree.h"

void print_simul_node(SimulNode *root, int show_children) {
    printf("SimulNode visits=%llu wins=%llu\n", root->visits, root->wins);
    if (!show_children) {
        return;
    }
    for (int i = 0; i < root->num_deals; ++i) {
        print_deals(root->deals[i].deal, NUM_PLAYERS);
        for (int j = 0; j < root->deals[i].num_choices; ++j) {
            ChoiceNode *c = root->deals[i].choices + j;
            printf(" %d,%d visits=%llu wins=%llu\n", c->choice[0], c->choice[1], c->target.visits, c->target.wins);
        }
    }
}

ChoiceNode* mc_tree_choose(DealtNode *node, McOptions options) {
    count_t wins[NUM_PLAYERS][NUM_CHOICES] = {0};
    count_t visits[NUM_PLAYERS][NUM_CHOICES] = {0};
    double total = 0;
    for (int i = 0; i < node->num_choices; ++i) {
        ChoiceNode *c = node->choices + i;
        wins[0][c->choice[0]] += c->target.wins;
        wins[1][c->choice[1]] += c->target.visits - c->target.wins;
        visits[0][c->choice[0]] += c->target.visits;
        visits[1][c->choice[1]] += c->target.visits;
        total += c->target.visits;
    }
    total = sqrt(log(total)) * options.exploration;
    double best[NUM_PLAYERS] = {-INFINITY, -INFINITY};
    char best_choice[NUM_PLAYERS];
    for (int i = 0; i < node->num_choices; ++i) {
        ChoiceNode *c = node->choices + i;
        for (int j = 0; j < NUM_PLAYERS; ++j) {
            double v = visits[j][c->choice[j]];
            assert(v);
            double value = wins[j][c->choice[j]] / v + total / sqrt(v);
            if (value > best[j]) {
                best[j] = value;
                best_choice[j] = c->choice[j];
            }
        }
    }
    #ifdef SIMUL_DEBUG
        printf("value=%g,%g choice=%d,%d out of %d\n", best[0], best[1], best_choice[0], best_choice[1], node->num_choices);
    #endif
    // TODO: Weighted choice
    for (int i = 0; i < node->num_choices; ++i) {
        ChoiceNode *c = node->choices + i;
        if (c->choice[0] == best_choice[0] && c->choice[1] == best_choice[1]) {
            return c;
        }
    }
    assert(0);
    return NULL;
}

SimulNode* mc_tree_policy(void *state, SimulNode *root, McOptions options, double *result) {
    if (!root->num_deals) {
        return root;
    }
    double weights[NUM_REDUCED_DEALS * NUM_REDUCED_DEALS];
    for (int i = 0; i < root->num_deals; ++i) {
        weights[i] = root->deals[i].probability;
    }
    size_t index = choose_weighted(weights, root->num_deals);
    ChoiceNode *choice = mc_tree_choose(root->deals + index, options);

    content_t choices[NUM_PLAYERS] = {CHOICES[choice->choice[0]], CHOICES[choice->choice[1]]};
    double score = options.step(state, choices);
    if (!isnan(score)) {
        *result = score;
        return root;
    }
    return mc_tree_policy(state, &choice->target, options, result);
}

double mc_playout(void *state, McOptions options) {
    content_t choices[NUM_PLAYERS];
    double result = NAN;
    while (isnan(result)) {
        options.policy(state, choices);
        result = options.step(state, choices);
    }
    return result;
}

void mc_single_round(void *state, SimulNode *root, McOptions options) {
    double result;
    SimulNode *leaf = mc_tree_policy(state, root, options, &result);

    if (!isnan(result)) {
        result = mc_playout(state, options);
    }
    SimulNode *node = leaf;
    do {
        if (result > 0) {
            node->wins += result;
        } else if (result == 0) {
            node->wins += jrand() % 2;
        }
        ++node->visits;
        node = node->parent;
    } while(node);
}

void mc_init_terminal(Game *game, ChoiceNode *node) {
    node->target.visits = 10000;
    for (int i = 0; i < NUM_CHOICES + 1; ++i) {
        for (int j = 0; j < NUM_CHOICES + 1; ++j) {
            Game *copy = copy_game(game);
            content_t choices[NUM_PLAYERS] = {CHOICES[i], CHOICES[j]};
            double result = step_game(copy, choices);
            free_game(copy);
            if (result != 0) {
                node->choice[0] = i;
                node->choice[1] = j;
                if (result > 0) {
                    node->target.wins = 10000;
                } else {
                    node->target.wins = 0;
                }
                return;
            }
        }
    }
    node->target.wins = 5000;
}

void mc_init_target(Game *game, SimulNode *root, int depth) {
    // Initialize to 50-50 winrate.
    root->wins = 1;
    root->visits = 2;
    if (!depth) {
        return;
    }
    root->deals = malloc(sizeof(DealtNode));
    root->num_deals = 1;
    root->deals->probability = 1;
    root->deals->deal[0] = game->deals[game->players[0].deal_index];
    root->deals->deal[1] = game->deals[game->players[1].deal_index];

    root->deals->num_choices = 0;
    root->deals->choices = calloc(NUM_CHOICES * NUM_CHOICES, sizeof(ChoiceNode));
    for (int i = 0; i < NUM_CHOICES + 1; ++i) {
        for (int j = 0; j < NUM_CHOICES + 1; ++j) {
            Game *copy = copy_game(game);
            content_t choices[NUM_PLAYERS] = {CHOICES[i], CHOICES[j]};
            double result = step_game(copy, choices);
            if (!isnan(result)) {
                free_game(copy);
                continue;
            }
            ChoiceNode *choice = root->deals->choices + root->deals->num_choices++;
            choice->target.parent = root;
            choice->choice[0] = i;
            choice->choice[1] = j;
            mc_init_target(copy, &choice->target, depth - 1);
            free_game(copy);
        }
    }
    if (!root->deals->num_choices) {
        mc_init_terminal(game, root->deals->choices);
        root->deals->num_choices = 1;
    }
    root->deals->choices = realloc(root->deals->choices, root->deals->num_choices * sizeof(ChoiceNode));
}

SimulNode* mc_init(Game *game) {
    SimulNode *root = calloc(1, sizeof(SimulNode));
    root->parent = NULL;
    mc_init_target(game, root, game->num_deals);
    return root;
}

void mc_iterate(void *state, SimulNode *root, size_t iterations, McOptions options) {
    for (size_t i = 0; i < iterations; ++i) {
        void *copy = options.copy(state);
        mc_single_round(copy, root, options);
        options.delete(copy);
    }
}

void mc_choose(SimulNode *root, content_t *choices) {
    assert(root->num_deals == 1);
    assert(root->deals->num_choices);
    McOptions options;
    options.exploration = 0;
    ChoiceNode *node = mc_tree_choose(root->deals, options);
    choices[0] = CHOICES[node->choice[0]];
    choices[1] = CHOICES[node->choice[1]];
}

void mc_free_(SimulNode *root) {
    for (int i = 0; i < root->num_deals; ++i) {
        for (int j = 0; j < root->deals[i].num_choices; ++j) {
            mc_free_(&root->deals[i].choices[j].target);
        }
        free(root->deals[i].choices);
    }
    free(root->deals);
}

void mc_free(SimulNode *root) {
    mc_free_(root);
    free(root);
}

McOptions get_mc_options() {
    McOptions options;
    options.exploration = 0.5;
    options.delete = free_game;
    options.copy = copy_game;
    options.step = step_game;
    options.policy = multi_random_policy;
    return options;
}
