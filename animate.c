#define ANIM_DELAY (20000)

void redraw_state(state *s) {
    for (int i = 0; i < TOTAL_HEIGHT; ++i) {
        printf("\033[A");
    }
    for (int j = 0; j < NUM_FLOORS; ++j) {
        for (int i = 0; i < HEIGHT * WIDTH; ++i) {
            if (i % V_SHIFT == 0) {
                int l = i / V_SHIFT + j * HEIGHT;
                if (l < 10) {
                    printf("%d", l);
                } else {
                    printf("%c", 'a' + l - 10);
                }
            }
            puyos_t p = (1ULL << i);
            int any = 0;
            for (int k = 0; k < NUM_COLORS; ++k) {
                if (p & s->floors[j][k]) {
                    if (j == 0 && i / WIDTH == GHOST_Y) {
                        printf("\x1b[3%dm", k + 1);
                    } else {
                        printf("\x1b[3%d;1m", k + 1);
                    }
                    if (k == GARBAGE) {
                        printf(" ◎");
                    } else {
                        printf(" ●");
                    }
                    any  = 1;
                    break;
                }
            }
            printf("\x1b[0m");
            if (!any) {
                printf("  ");
            }
            if (i % V_SHIFT == V_SHIFT - 1){
                printf(" \n");
            }
        }
    }
}

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

        bellow = (all[0] >> V_SHIFT) | ((all[1] & TOP) << TOP_TO_BOTTOM);
        all[0] = 0;
        for (int i = 0; i < NUM_COLORS; ++i) {
            falling = s->floors[0][i] & ~bellow;

            s->floors[1][i] |= (falling & BOTTOM) >> TOP_TO_BOTTOM;

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
