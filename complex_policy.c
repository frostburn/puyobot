content_t gcn_game_policy(game *g, int player_index) {
    practice_game *pg = game_as_practice(g, player_index);
    if (!pg) {
        return CHOICE_PASS;
    }
    content_t choice = gcn_practice_policy(pg, pg->deals, pg->num_deals);
    free(pg);
    return choice;
}

content_t mc_game_policy(game *g, int player_index) {
    practice_game *pg = game_as_practice(g, player_index);
    if (!pg) {
        return CHOICE_PASS;
    }
    mc_options options = simple_mc_options(10000, random_policy);
    options.step = step_practice;
    options.copy = copy_practice;
    content_t choice = iterate_mc(pg, pg->deals, pg->num_deals, options);
    free(pg);
    return choice;
}

content_t gcnk_game_policy(game *g, int player_index) {
    practice_game *pg = game_as_practice(g, player_index);
    if (!pg) {
        return CHOICE_PASS;
    }
    pg->time = 0;
    pg->player.total_score = 0;
    knockout_context *context = new_knockout(g, 1 - player_index);
    tree_options options;
    content_t choice = CHOICE_PASS;

    double eval_early(void *pg) {
        return eval_knockout_prototype(pg, context);
    }
    options = simple_tree_options(eval_early, 0, 1);
    options.step = step_practice;
    options.copy = copy_practice;

    for (int num_deals = 1; num_deals <= g->num_deals; ++num_deals) {
        value_node *root = make_full_tree(num_deals, options.depth);
        solve_tree(pg, root, pg->deals, num_deals, options);
        if (root->value > 1e10) {
            choice = choose(root)->content;
        }
        free_tree(root);
    }
    if (choice != CHOICE_PASS) {
        free(pg);
        free(context);
        return choice;
    }

    double eval(void *_pg) {
        practice_game *pg = _pg;
        state *s = &pg->player.state;
        double groups = eval_groups(s);
        double chains = eval_groups(s);
        double nuisance = popcount(s->floors[0][GARBAGE]) + popcount(s->floors[1][GARBAGE]);
        double knockout = eval_knockout_prototype(pg, context);
        double dead = eval_dead(pg);
        return 
            1 * groups +
            20 * chains + 
            (-0.96) * nuisance +
            (-4.0 - 1.0 / (pg->delay + 2)) * pg->incoming +
            0.00001 * knockout +
            dead;
    }

    options = simple_tree_options(eval, 1, 0.4);
    options.step = step_practice;
    options.copy = copy_practice;
    choice = solve(pg, pg->deals, pg->num_deals, options);
    free(pg);
    free(context);
    return choice;
}
