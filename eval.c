typedef float (*eval_fun)(state *s);

float eval_fun_zero(state *s) {
    return 0;
}

float eval_fun_random(state *s) {
    state *c = malloc(sizeof(state));
    int total_score = 0;
    for (int i = 0; i < 20; ++i) {
        memcpy(c, s, sizeof(state));
        for (int j = 0; j < 25; ++j) {
            if(!apply_deal_and_choice(c, rand_piece(), CHOICES[jrand() % NUM_CHOICES])) {
                break;
            }
            total_score += resolve(c, NULL);
        }
    }
    free(c);
    return total_score * 0.05;
}

float eval_fun_weighted(state *s) {
    state *c;
    double total_score = 0;
    double total_weight = 0;
    for (int i = 0; i < 10; ++i) {
        c = copy_state(s);
        for (int j = 0; j < 25; ++j) {
            if(!apply_deal_and_choice(c, rand_piece(), CHOICES[jrand() % NUM_CHOICES])) {
                break;
            }
            double score = resolve(c, NULL);
            int num_remaining = 0;
            for (int j = 0; j < NUM_FLOORS; ++j) {
                for (int i = 0; i < NUM_COLORS; ++i) {
                    num_remaining += popcount(c->floors[j][i]);
                }
            }
            score += 1.5 * exp(-0.3 * num_remaining);
            double weight = exp(-state_euler(c));
            total_weight += weight;
            total_score += score * weight;
        }
        free(c);
    }
    return total_score / total_weight;
}

float eval_fun_groups(state *s) {
    puyos_t groups[2*MAX_GROUPS];
    float score = 0;
    for (int i = 0; i < NUM_COLORS - 1; ++i) {
        puyos_t puyos[2] = {s->floors[0][i], s->floors[1][i]};
        int num = num_groups_2(puyos, groups);
        for (int j = 0; j < num; ++j) {
            int size = popcount(groups[2*j]) + popcount(groups[2*j + 1]);
            score += size*size;
        }
    }
    return score;
}
