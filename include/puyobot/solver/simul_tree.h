#include "puyobot/deal.h"
#include "puyobot/multiplayer.h"
#include "puyobot/solver/game_policy.h"

#define NUM_REDUCED_DEALS (NUM_DEAL_COLORS + (NUM_DEAL_COLORS * (NUM_DEAL_COLORS - 1)) / 2)
#define NUM_PLAYERS (2)

typedef double (*multi_step_fun)(void *s, content_t *choices);

typedef unsigned long long int count_t;

typedef struct SimulNode
{
    count_t wins;
    count_t visits;
    struct DealtNode *deals;
    char num_deals;
    struct SimulNode *parent;
} SimulNode;

typedef struct DealtNode
{
    content_t deal[NUM_PLAYERS];
    float probability;
    struct ChoiceNode *choices;
    short int num_choices;
} DealtNode;

typedef struct ChoiceNode
{
    char choice[NUM_PLAYERS];
    SimulNode target;
} ChoiceNode;

typedef struct McOptions
{
    copy_fun copy;
    delete_fun delete;
    multi_step_fun step;
    multi_policy_fun policy;
    double exploration;
} McOptions;

void print_simul_node(SimulNode *root, int show_children);

SimulNode* mc_init(Game *game);

void mc_iterate(void *state, SimulNode *root, size_t iterations, McOptions options);

void mc_choose(SimulNode *root, content_t *choices);

void mc_free(SimulNode *root);

McOptions get_mc_options();
