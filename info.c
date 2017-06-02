void eval_groups_info(state *s, FILE *stream) {
    int counts[CLEAR_THRESHOLD] = {0};
    puyos_t groups[2*MAX_GROUPS];
    double score = 0;
    for (int i = 0; i < NUM_COLORS - 1; ++i) {
        puyos_t puyos[2] = {s->floors[0][i], s->floors[1][i]};
        int num = num_groups_2(puyos, groups);
        for (int j = 0; j < num; ++j) {
            int size = popcount(groups[2*j]) + popcount(groups[2*j + 1]);
            score += size*size;
            counts[size]++;
        }
    }
    for (int i = 1; i < CLEAR_THRESHOLD; ++i) {
        fprintf(stream, "%d groups of size %d\n", counts[i], i);
    }
    fprintf(stream, "Total score %f\n", score);
}

void eval_chains_info(state *s, FILE *stream) {
    int original_pc = state_popcount(s);
    puyos_t all[2];
    get_state_mask(s, all);
    double score = 0;
    state *c = malloc(sizeof(state));
    for (int i = 0; i < NUM_COLORS - 1; ++i) {
        puyos_t handled[2] = {0};
        for (int j = 0; j < WIDTH; ++j) {
            // Resolution is expensive so we try to short-circuit.
            // We also filter out chains that we checked previously.
            // There are some corner cases where the filtering is overaggressive, but they are ignored for now.
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
            } else if ((p[0] & handled[0]) || (p[1] & handled[1])) {
                continue;
            } else {
                handled[0] |= p[0];
                handled[1] |= p[1];
            }
            // Do the actual thing.
            memcpy(c, s, sizeof(state));
            c->floors[0][i] |= 1ULL << j;
            int chain;
            resolve(c, &chain);
            if (chain > 1) {
                double pc = state_popcount(c);
                double count = original_pc + 1 - pc;
                score += chain * chain * chain / count;
                double efficiency = chain * CLEAR_THRESHOLD / count;
                fprintf(stream, "A %s %d-chain consisting of %d puyos. Efficiency=%d%%\n", COLOR_NAMES[i], chain, (int)count, (int)(100 * efficiency));
            }
        }
    }
    free(c);
    if (!score) {
        fprintf(stream, "No chains\n");
    }
    fprintf(stream, "Total score %f\n", score);
}
