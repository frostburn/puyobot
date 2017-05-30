#include "policy.c"

#define DEFAULT_EXPLORATION (5555.55)
#define DEFAULT_TREE_FACTOR (0.1)
#define DEFAULT_POLICY_FACTOR (1.0)
#define DEFAULT_STEPS (30)
#define MC_GAME_OVER (3.14)
#define MAX_DEPTH (255)
#define MAX_STEPS (40)

typedef struct mc_options
{
    size_t iterations;
    size_t num_policy_steps;
    float exploration;
    float tree_factor;
    float policy_factor;
    policy_fun policy;
    eval_fun eval;
} mc_options;

mc_options simple_mc_options(size_t iterations, policy_fun policy) {
    return (mc_options) {
        .iterations = iterations,
        .policy = policy,
        .num_policy_steps = DEFAULT_STEPS,
        .exploration = DEFAULT_EXPLORATION,
        .tree_factor = DEFAULT_TREE_FACTOR,
        .policy_factor = DEFAULT_POLICY_FACTOR,
        .eval = eval_zero,
    };
}

choice_branch* tree_policy(state *s, value_node *root, mc_options options) {
    if (!root->num_deals) {
        return NULL;
    }
    // Assumes uniform deals. XXX: Incorrect under symmetry reduction.
    int i = jrand() % root->num_deals;
    size_t total_visits = 0;
    for (int j = 0; j < root->deals[i].num_choices; ++j) {
        total_visits += root->deals[i].choices[j].visits;
    }
    total_visits += 1;
    double best_value = -INFINITY;
    choice_branch *best_branch = root->deals[i].choices;
    for (int j = 0; j < root->deals[i].num_choices; ++j) {
        double value = root->deals[i].choices[j].destination->value;
        double visits = root->deals[i].choices[j].visits + 1;
        value = value / visits + options.exploration * sqrt(log(total_visits) / visits);
        if (value > best_value) {
            best_value = value;
            best_branch = root->deals[i].choices + j;
        }
    }
    if (!apply_deal_and_choice(s, root->deals[i].content, best_branch->content)) {
        best_branch->destination->value -= DEATH_SCORE;
    }
    best_branch->destination->value += resolve(s, NULL) * options.tree_factor;
    return best_branch;
}

void eval_mc(state *s, value_node *root, size_t num_deals, mc_options options) {
    assert(options.num_policy_steps + num_deals < MAX_STEPS);
    choice_branch *path[MAX_DEPTH];
    int path_len = 0;
    choice_branch *leaf = NULL;
    do {
        value_node *n = root;
        if (leaf) {
            n = leaf->destination;
        }
        leaf = tree_policy(s, n, options);
        path[path_len++] = leaf;
    } while (leaf);
    path_len--;

    leaf = path[path_len - 1];
    if (leaf->visits > 3) {
        expand(leaf->destination);
    }

    double score = 0;
    content_t deals[MAX_STEPS];
    for (int i = 0; i < MAX_STEPS; ++i) {
        deals[i] = rand_piece();
    }
    for (int i = 0; i < options.num_policy_steps; ++i) {
        content_t choice = options.policy(s, deals + i, num_deals);
        if (!apply_deal_and_choice(s, deals[i], choice)) {
            score -= MC_GAME_OVER * options.policy_factor;
            break;
        }
        score += resolve(s, NULL) * options.policy_factor;
    }
    score += options.eval(s);

    for (int i = 0; i < path_len; ++i) {
        path[i]->visits++;
        path[i]->destination->value += score;
    }
    // Root value not updated.
}

content_t greedy_choice(state *s, value_node *root) {
    if (root->num_deals != 1) {
        return 0;
    }
    content_t best_action = 0;
    float best_value = -INFINITY;
    for (num_t i = 0; i < root->deals->num_choices; ++i) {
        float value = root->deals->choices[i].destination->value / (root->deals->choices[i].visits + 3);

        if (value > best_value) {
            best_value = value;
            best_action = root->deals->choices[i].content;
        }
    }
    return best_action;
}

content_t iterate_mc(state *s, content_t *deals, size_t num_deals, mc_options options) {
    value_node *root = calloc(1, sizeof(value_node));
    append_deals(root, deals, num_deals);
    for (size_t i = 0; i < options.iterations; ++i) {
        state *c = copy_state(s);
        eval_mc(c, root, num_deals, options);
        free(c);
    }
    content_t choice = greedy_choice(s, root);
    free_tree(root);
    return choice;
}
