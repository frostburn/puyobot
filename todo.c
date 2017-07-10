// Calculate the maximum score attainable from a state in num turns
// May overestimate
// XXX: Sometimes underestimates. Fix that!
int max_score_simple(state *s, int num_turns) {
    int bins[NUM_COLORS - 1];

    for (int k = 0; k < NUM_COLORS - 1; ++k) {
        puyos_t component[2] = {s->floors[0][k], s->floors[1][k]};
        int count = popcount_2(component);
        // TODO: Improve this heuristic and make max_score respect it
        // if (count == CLEAR_THRESHOLD && gap_size_2(component)) {
        //     count -= 1;
        // }
        bins[k] = count;
    }

    while (num_turns) {
        int blocks = 2;
        while (blocks) {
            for (int k = 0; k < NUM_COLORS - 1; ++k) {
                int overflow = bins[k] % CLEAR_THRESHOLD;
                if (overflow == 3) {
                    bins[k] += 1;
                    blocks -= 1;
                    if (!blocks) {
                        break;
                    }
                }
            }
            if (!blocks) {
                break;
            }
            for (int k = 0; k < NUM_COLORS - 1; ++k) {
                int overflow = bins[k] % CLEAR_THRESHOLD;
                if (overflow == 2) {
                    bins[k] += blocks;
                    break;
                }
            }
            if (!blocks) {
                break;
            }
            for (int k = 0; k < NUM_COLORS - 1; ++k) {
                int overflow = bins[k] % CLEAR_THRESHOLD;
                if (overflow == 1) {
                    bins[k] += blocks;
                    break;
                }
            }
            if (!blocks) {
                break;
            }
            bins[0] += blocks;
            break;
        }
        num_turns -= 1;
    }
    int num_cleared = 0;
    int group_bonus = 0;
    int num_colors = 0;
    for (int k = 0; k < NUM_COLORS - 1; ++k) {
        if (bins[k] >= CLEAR_THRESHOLD) {
            int group_size = bins[k] - CLEAR_THRESHOLD;
            if (group_size >= NUM_GROUP_BONUS) {
                group_size = NUM_GROUP_BONUS - 1;
            }
            group_bonus += GROUP_BONUS[group_size];
            num_cleared += bins[k];
            ++num_colors;
        }
    }
    int combo_score = (10 * num_cleared) * (group_bonus + COLOR_BONUS[num_colors]);

    int score = 0;
    int chain_number = 0;
    for (int k = 0; k < NUM_COLORS - 1; ++k) {
        while (bins[k] >= CLEAR_THRESHOLD * 2) {
            int clear_bonus = GROUP_BONUS[0] + CHAIN_POWERS[chain_number];
            if (clear_bonus < 1) {
                clear_bonus = 1;
            } else if (clear_bonus > MAX_CLEAR_BONUS) {
                clear_bonus = MAX_CLEAR_BONUS;
            }
            score += (10 * CLEAR_THRESHOLD) * clear_bonus;
            bins[k] -= CLEAR_THRESHOLD;
            if (chain_number < NUM_CHAIN_POWERS - 1) {
                ++chain_number;
            }
        }
    }
    qsort(bins, NUM_COLORS - 1, sizeof(int), cmp_int);
    for (int k = 0; k < NUM_COLORS - 1; ++k) {
        if (bins[k] >= CLEAR_THRESHOLD) {
            int group_size = bins[k] - CLEAR_THRESHOLD;
            if (group_size >= NUM_GROUP_BONUS) {
                group_size = NUM_GROUP_BONUS - 1;
            }
            int clear_bonus = GROUP_BONUS[group_size] + CHAIN_POWERS[chain_number];
            if (clear_bonus < 1) {
                clear_bonus = 1;
            } else if (clear_bonus > MAX_CLEAR_BONUS) {
                clear_bonus = MAX_CLEAR_BONUS;
            }
            score += (10 * bins[k]) * clear_bonus;
            if (chain_number < NUM_CHAIN_POWERS - 1) {
                ++chain_number;
            }
        }
    }
    if (combo_score > score) {
        return combo_score;
    }
    return score;
}

int max_score(state *s, content_t *deals, int num_deals, int num_turns) {
    state *c = copy_state(s);
    if (num_turns < num_deals) {
        num_deals = num_turns;
    }
    for (int i = 0; i < num_deals; ++i) {
        c->floors[0][deal_color1(deals[i])] |= 1 << (2*i);
        c->floors[0][deal_color2(deals[i])] |= 1 << (2*i + 1);
    }
    num_turns -= num_deals;
    int score = max_score_simple(c, num_turns);
    free(c);
    return score;
}
