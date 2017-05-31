typedef content_t (*policy_fun)(void *s, content_t*, int);

content_t random_policy(void *s, content_t *deals, int  num_deals) {
    return CHOICES[jrand() % NUM_CHOICES];
}

content_t euler_policy(void *s, content_t *deals, int  num_deals) {
    double weights[NUM_CHOICES];
    double total_weight = 0;
    for (int i = 0; i < NUM_CHOICES; ++i) {
        state *c = copy_state(s);
        apply_deal_and_choice(c, deals[0], CHOICES[i]);
        handle_gravity(c);
        double weight = exp(-state_euler(c));
        weights[i] = weight;
        total_weight += weight;
        free(c);
    }
    double p = jdrand() * total_weight;
    for (int i = 0; i < NUM_CHOICES; ++i) {
        p -= weights[i];
        if (p <= 0) {
            return CHOICES[i];
        }
    }
    return 0;
}

content_t clump_policy(void *s, content_t *deals, int  num_deals) {
    content_t color1 = deal_color1(deals[0]);
    content_t color2 = deal_color2(deals[0]);
    int color1_score = 1;
    int color2_score = 1;
    for (int  i = 1; i < num_deals; ++i) {
        if (deal_color1(deals[i]) == color1) {
            color1_score++;
        }
        if (deal_color2(deals[i]) == color1) {
            color1_score++;
        }
        if (deal_color1(deals[i]) == color2) {
            color2_score++;
        }
        if (deal_color2(deals[i]) == color2) {
            color2_score++;
        }
    }
    double weights[NUM_CHOICES];
    double total_weight = 0;
    for (int i = 0; i < NUM_CHOICES; ++i) {
        state *c = copy_state(s);
        apply_deal_and_choice(c, deals[0], CHOICES[i]);
        handle_gravity(c);
        puyos_t puyos[2] = {c->floors[0][color1], c->floors[1][color1]};
        int total_groups1 = num_groups_2(puyos, NULL);
        puyos[0] = c->floors[0][color2];
        puyos[1] = c->floors[1][color2];
        int total_groups2 = num_groups_2(puyos, NULL);
        weights[i] = exp(-total_groups1 * color1_score - total_groups2 * color2_score);
        total_weight += weights[i];
        free(c);
    }
    double p = jdrand() * total_weight;
    for (int i = 0; i < NUM_CHOICES; ++i) {
        p -= weights[i];
        if (p <= 0) {
            return CHOICES[i];
        }
    }
    return 0;
}

content_t frog_stacking_policy(void *_s, content_t *deals, int  num_deals) {
    state *s = (state*) _s;
    int total_left = 0;
    int total_right = 0;
    for (int i = 0; i < NUM_FLOORS; ++i) {
        for (int j = 0; j < NUM_COLORS; ++j) {
            total_left += popcount(s->floors[i][j] & LEFT_SIDE);
            total_right += popcount(s->floors[i][j] & RIGHT_SIDE);
        }
    }
    content_t choice;
    if (total_left > total_right) {
        choice = rand_choice(0, 2);
    } else {
        choice = rand_choice(3, 5);
    }
    for (int i = 0; i < 10; ++i) {
        state *c = copy_state(s);
        if (apply_deal_and_choice(s, deals[0], choice)) {
            free(c);
            break;
        }
        choice = CHOICES[jrand() % NUM_CHOICES];
        free(c);
    }
    return choice;
}

content_t group_policy(void *s, content_t *deals, int  num_deals) {
    return solve(s, deals, num_deals, 0, eval_groups, 0.015);
}


double _eval_groups_chains(void *s) {
    float groups = eval_groups(s);
    float chains = eval_chains(s);
    return groups + 20 * chains;
}

content_t group_chain_policy(void *s, content_t *deals, int  num_deals) {
    float factor = 0.0017;
    float pc = state_popcount(s);
    if (pc > 50) {
        factor = 0.03;
    }
    if (pc > 68) {
        factor = 0.1;
    }
    return solve(s, deals, num_deals, 0, _eval_groups_chains, factor);
}
