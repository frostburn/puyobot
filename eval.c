typedef double (*eval_fun)(void *s);

double eval_zero(void *s) {
    return 0;
}

double eval_random(void *s) {
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

double eval_weighted(void *s) {
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

// groups and chains inspired by https://github.com/mbrown1413/Puyo-AI

double eval_groups(void *_s) {
    state *s = _s;
    puyos_t groups[2*MAX_GROUPS];
    double score = 0;
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

double eval_chains(void *_s) {
    state *s = _s;
    int original_pc = state_popcount(s);
    puyos_t all[2];
    get_state_mask(s, all);
    double score = 0;
    state *c = malloc(sizeof(state));
    for (int i = 0; i < NUM_COLORS - 1; ++i) {
        for (int j = 0; j < WIDTH; ++j) {
            // Resolution is expensive so we try to short-circuit.
            int depth = depth_2(all, j);
            puyos_t p[2];
            point_2(p, j, depth - 1);
            puyos_t target[2] = {
                (s->floors[0][i] & LIFE_BLOCK) | p[0],
                s->floors[1][i] | p [1]
            };
            flood_2(p, target);
            if (popcount_2(p) < CLEAR_THRESHOLD) {
                continue;
            }
            // Do the actual thing.
            memcpy(c, s, sizeof(state));
            c->floors[0][i] |= 1ULL << j;
            int chain;
            resolve(c, &chain);
            if (chain > 1) {
                double pc = state_popcount(c);
                score += chain * chain * chain / (original_pc - pc);
            }
        }
    }
    free(c);
    return score;
}

// XXX: Needs more work
double eval_groups_plus(void *_s) {
    state *s = _s;
    puyos_t groups[2*MAX_GROUPS];
    double score = 0;
    for (int i = 0; i < NUM_COLORS - 1; ++i) {
        puyos_t puyos[2] = {s->floors[0][i], s->floors[1][i]};
        int num = num_groups_2(puyos, groups);
        puyos_t threes[2] = {0, 0};
        for (int j = 0; j < num; ++j) {
            int size = popcount(groups[2*j]) + popcount(groups[2*j + 1]);
            if (size == 3) {
                threes[0] |= groups[2*j];
                threes[1] |= groups[2*j + 1];
             }
        }
        for (int j = 0; j < num; ++j) {
            int size = popcount(groups[2*j]) + popcount(groups[2*j + 1]);
            score += pow(size, 2.5);
            if (size == 1) {
                puyos_t group_l[2] = {groups[2*j], groups[2*j + 1]};
                down_2(group_l);
                puyos_t group_r[2] = {group_l[0], group_l[1]};
                left_2(group_l);
                right_2(group_r);
                int left_connects = (threes[0] & group_l[0]) || (threes[1] & group_l[1]);
                int right_connects = (threes[0] & group_r[0]) || (threes[1] & group_r[1]);
                if (left_connects ^ right_connects) {
                    score += 0.77;
                }
            }
        }
    }
    return score;
}
