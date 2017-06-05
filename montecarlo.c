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
    copy_fun copy;
    step_fun step;
    policy_fun policy;
    eval_fun eval;
    size_t iterations;
    size_t num_policy_steps;
    float exploration;
    float tree_factor;
    float policy_factor;
} mc_options;

mc_options simple_mc_options(size_t iterations, policy_fun policy) {
    return (mc_options) {
        .copy = state_copy,
        .step = state_step,
        .policy = policy,
        .eval = eval_zero,
        .iterations = iterations,
        .num_policy_steps = DEFAULT_STEPS,
        .exploration = DEFAULT_EXPLORATION,
        .tree_factor = DEFAULT_TREE_FACTOR,
        .policy_factor = DEFAULT_POLICY_FACTOR,
    };
}

choice_branch* tree_policy(void *s, value_node *root, mc_options options) {
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
    double score = options.step(s, root->deals[i].content, best_branch->content);
    best_branch->destination->value += score * options.tree_factor;
    return best_branch;
}

int find_path(void *s, value_node *root, mc_options options, choice_branch **path) {
    int path_len = 0;
    while (1) {
        choice_branch *leaf = tree_policy(s, root, options);
        if (leaf) {
            root = leaf->destination;
            path[path_len++] = leaf;
        } else {
            break;
        }
    }
    return path_len;
}

double rollout(void *s, int num_deals, mc_options options) {
    double score = 0;
    content_t deals[MAX_STEPS];
    for (int i = 0; i < MAX_STEPS; ++i) {
        deals[i] = rand_piece();
    }
    for (int i = 0; i < options.num_policy_steps; ++i) {
        content_t choice = options.policy(s, deals + i, num_deals);
        double step_score = options.step(s, deals[i], choice);
        if (step_score < -MC_GAME_OVER) {
            score -= MC_GAME_OVER * options.policy_factor;
            break;
        }
        score += step_score * options.policy_factor;
    }
    score += options.eval(s);
    return score;
}

choice_branch* eval_mc(void *s, value_node *root, int num_deals, mc_options options) {
    assert(options.num_policy_steps + num_deals < MAX_STEPS);
    void *c = options.copy(s);
    choice_branch *path[MAX_DEPTH];
    int path_len = find_path(c, root, options, path);

    double score = rollout(c, num_deals, options);

    for (int i = 0; i < path_len; ++i) {
        path[i]->visits++;
        path[i]->destination->value += score;
    }
    free(c);
    // Root value not updated.
    return path[path_len - 1];
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

content_t iterate_mc(void *s, content_t *deals, size_t num_deals, mc_options options) {
    value_node *root = calloc(1, sizeof(value_node));
    append_deals(root, deals, num_deals);
    for (size_t i = 0; i < options.iterations; ++i) {
        choice_branch *leaf = eval_mc(s, root, num_deals, options);
        if (leaf->visits > 3) {
            expand(leaf->destination);
        }
    }
    content_t choice = greedy_choice(s, root);
    free_tree(root);
    return choice;
}

#ifdef _OPENMP
    content_t omp_vote_mc(void *s, content_t *deals, size_t num_deals, mc_options options) {
        int votes[256] = {0};
        #pragma omp parallel
        {
            content_t choice = iterate_mc(s, deals, num_deals, options);
            #pragma omp critical
            ++votes[choice];
        }
        content_t best_choice = 0;
        int most_votes = -1;
        puyos_t indices[256];
        for (int i = 0; i < 256; ++i) {
            indices[i] = i;
        }
        shuffle(indices, 256);
        for (int i = 0; i < 256; ++i) {
            content_t j = indices[i];
            if (votes[j] > most_votes) {
                best_choice = j;
                most_votes = votes[j];
            }
        }
        return best_choice;
    }
#endif
