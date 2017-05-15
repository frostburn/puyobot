typedef content_t (*policy_fun)(state *s, content_t*, size_t);

content_t random_policy(state *s, content_t *deals, size_t num_deals) {
    return CHOICES[rand() % NUM_CHOICES];
}

content_t euler_policy(state *s, content_t *deals, size_t num_deals) {
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
    double p = drand() * total_weight;
    for (int i = 0; i < NUM_CHOICES; ++i) {
        p -= weights[i];
        if (p <= 0) {
            return CHOICES[i];
        }
    }
    return 0;
}

content_t clump_policy(state *s, content_t *deals, size_t num_deals) {
    content_t color1 = deal_color1(deals[0]);
    content_t color2 = deal_color2(deals[0]);
    int color1_score = 0;
    int color2_score = 0;
    for (size_t i = 1; i < num_deals; ++i) {
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
        int total_groups1 = num_groups(c->floors[0][color1]) + num_groups(c->floors[1][color2]);  // XXX: Obviously incorrect
        int total_groups2 = num_groups(c->floors[0][color1]) + num_groups(c->floors[1][color2]);  // XXX: Obviously incorrect
        weights[i] = exp(-total_groups1 * color1_score - total_groups2 * color2_score);
        total_weight += weights[i];
        free(c);
    }
    double p = drand() * total_weight;
    for (int i = 0; i < NUM_CHOICES; ++i) {
        p -= weights[i];
        if (p <= 0) {
            return CHOICES[i];
        }
    }
    return 0;
}

content_t frog_stacking_policy(state *s, content_t *deals, size_t num_deals) {
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
        choice = CHOICES[rand() % NUM_CHOICES];
        free(c);
    }
    return choice;
}
