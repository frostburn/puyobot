#ifndef PUYOBOT_SOLVER_TREE_H_GUARD
#define PUYOBOT_SOLVER_TREE_H_GUARD

#include "puyobot/deal.h"
#include "puyobot/solver/policy.h"
#include "puyobot/solver/search.h"

#define NUM_REDUCED_DEALS (NUM_DEAL_COLORS + (NUM_DEAL_COLORS * (NUM_DEAL_COLORS - 1)) / 2)

typedef unsigned long long int count_t;

typedef struct TreeNode
{
    double score;
    count_t visits;
    struct DealNode *deals;
    char num_deals;
    struct TreeNode *parent;
} TreeNode;

typedef struct DealNode
{
    content_t content;
    float probability;
    struct ChoiceNode *choices;
    char num_choices;
} DealNode;

typedef struct ChoiceNode
{
    content_t content;
    TreeNode target;
} ChoiceNode;

typedef struct McOptions
{
    copy_fun copy;
    delete_fun delete;
    step_fun step;
    policy_fun policy;
    double exploration;
    double score_threshold;
} McOptions;

void print_tree_node(TreeNode *root, int show_children);

TreeNode* mc_init(void *state, content_t *deals, int num_deals, McOptions options);

void mc_iterate(void *state, TreeNode *root, size_t iterations, McOptions options);

content_t mc_choose(TreeNode *root);

McOptions get_mc_options(policy_fun policy);

void mc_free(TreeNode *root);

#endif
