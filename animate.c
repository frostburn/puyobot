#define ANIM_DELAY (20000)

void animate_gravity(state *s, void (*callback)(state*)) {
    assert(NUM_FLOORS == 2);
    puyos_t all[2];
    for (int i = 0; i < NUM_FLOORS; ++i) {
        all[i] = 0;
        for (int j = 0; j < NUM_COLORS; ++j) {
            all[i] |= s->floors[i][j];
        }
    }

    puyos_t temp[2];
    do {
        temp[0] = all[0];
        temp[1] = all[1];
        puyos_t bellow, falling;
        bellow = (all[1] >> V_SHIFT) | BOTTOM;
        all[1] = 0;
        for (int i = 0; i < NUM_COLORS; ++i) {
            falling = s->floors[1][i] & ~bellow;
            s->floors[1][i] = (falling << V_SHIFT) | (s->floors[1][i] & ~falling);
            all[1] |= s->floors[1][i];
        }

        bellow = (all[0] >> V_SHIFT) | ((all[1] & TOP) << (V_SHIFT * (HEIGHT - 1)));
        all[0] = 0;
        for (int i = 0; i < NUM_COLORS; ++i) {
            falling = s->floors[0][i] & ~bellow;

            s->floors[1][i] |= (falling & BOTTOM) >> (V_SHIFT * (HEIGHT - 1));

            s->floors[0][i] = (falling << V_SHIFT) | (s->floors[0][i] & ~falling);
            all[0] |= s->floors[0][i];
        }
        callback(s);
        usleep(ANIM_DELAY);
    } while (temp[0] != all[0] || temp[1] != all[1]);
}

int animate(state *s, void (*callback)(state*)) {
    int chain = -1;
    int total_score = 0;
    while(1) {
        ++chain;
        animate_gravity(s, callback);
        kill_puyos(s);
        int score = clear_groups(s, chain);
        if (!score) {
            break;
        }
        usleep(50 * ANIM_DELAY);
        total_score += score;
    }
    return chain;
}
