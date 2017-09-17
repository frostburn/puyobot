#ifndef PUYOBOT_SOLVER_SIMUL_TREE_H_GUARD
#define PUYOBOT_SOLVER_SIMUL_TREE_H_GUARD

#include "puyobot/deal.h"
#include "puyobot/multiplayer.h"
#include "puyobot/solver/game_policy.h"
#include "puyobot/solver/tree.h"

#define NUM_PLAYERS (2)

typedef double (*multi_step_fun)(void *s, content_t *choices);

typedef struct SimulNode
{
    count_t wins;
    count_t visits;
    struct SimulDealNode *deals;
    char num_deals;
    struct SimulNode *parent;
} SimulNode;

typedef struct SimulDealNode
{
    content_t deal[NUM_PLAYERS];
    float probability;
    struct SimulChoiceNode *choices;
    short int num_choices;
} SimulDealNode;

typedef struct SimulChoiceNode
{
    char choice[NUM_PLAYERS];
    SimulNode target;
} SimulChoiceNode;

typedef struct SimulMcOptions
{
    copy_fun copy;
    delete_fun delete;
    multi_step_fun step;
    multi_policy_fun policy;
    double exploration;
} SimulMcOptions;

void print_simul_node(SimulNode *root, int show_children);

SimulNode* simul_mc_init(Game *game);

void simul_mc_iterate(void *state, SimulNode *root, size_t iterations, SimulMcOptions options);

void simul_mc_choose(SimulNode *root, content_t *choices);

void simul_mc_free(SimulNode *root);

SimulMcOptions get_simul_mc_options();

#endif
