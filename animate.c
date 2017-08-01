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
                    any = 1;
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
    puyos_t all[NUM_FLOORS];
    get_state_mask(s, all);

    puyos_t temp[2];
    do {
        temp[0] = all[0];
        temp[1] = all[1];
        puyos_t bellow, falling;
        bellow = (all[1] >> V_SHIFT) | BOTTOM;
        all[1] = 0;
        for (int i = 0; i < NUM_COLORS; ++i) {
            falling = s->floors[1][i] & ~bellow;
            s->floors[1][i] = (falling << V_SHIFT) | (s->floors[1][i] & bellow);
            all[1] |= s->floors[1][i];
        }

        bellow = (all[0] >> V_SHIFT) | ((all[1] & TOP) << TOP_TO_BOTTOM);
        all[0] = 0;
        for (int i = 0; i < NUM_COLORS; ++i) {
            falling = s->floors[0][i] & ~bellow;

            s->floors[1][i] |= (falling & BOTTOM) >> TOP_TO_BOTTOM;

            s->floors[0][i] = (falling << V_SHIFT) | (s->floors[0][i] & bellow);
            all[0] |= s->floors[0][i];
        }
        callback(s);
        usleep(ANIM_DELAY);
    } while (temp[0] != all[0] || temp[1] != all[1]);
}

void animate(state *s, void (*callback)(state*)) {
    while(1) {
        animate_gravity(s, callback);
        kill_puyos(s);
        if (!clear_groups(s, 1)) {
            break;
        }
        usleep(50 * ANIM_DELAY);
    }
}

void redraw_bottom_spam(puyos_t *floor, int num_links, int num_colors) {
    for (int i = 0; i <= HEIGHT; ++i) {
        printf("\033[A");
    }
    printf("\n");
    for (int i = 0; i < HEIGHT * WIDTH; ++i) {
        if (i % V_SHIFT == 0) {
            printf("%d", i / V_SHIFT);
        }
        puyos_t p = (1ULL << i);
        int any = 0;
        for (int k = 0; k < num_colors; ++k) {
            if (p & floor[k]) {
                printf("\x1b[3%d;1m", k % 6 + 1);
                if (k < num_links) {
                    printf(" %c", color_label(k));
                } else {
                    printf(" @");
                }
                any  = 1;
                break;
            }
        }
        printf("\x1b[0m");
        if (!any) {
            printf("  ");
        }
        if (i % V_SHIFT == V_SHIFT - 1) {
            printf(" \n");
        }
    }
}

int animate_bottom_gravity(puyos_t *floor, int num_links, int num_colors, void (*callback)(puyos_t*, int, int)) {
    puyos_t all;
    all = 0;
    for (int j = 0; j < num_colors; ++j) {
        all |= floor[j];
    }

    int iterations = 0;
    puyos_t temp;
    do {
        temp = all;
        puyos_t bellow, falling;
        bellow = (all >> V_SHIFT) | BOTTOM;
        all = 0;
        for (int i = 0; i < num_colors; ++i) {
            falling = floor[i] & ~bellow;
            floor[i] = (falling << V_SHIFT) | (floor[i] & bellow);
            all |= floor[i];
        }
        ++iterations;
        callback(floor, num_links, num_colors);
        usleep(ANIM_DELAY);
    } while (temp != all);
    return iterations;
}

void animate_bottom(puyos_t *floor, int num_links, int num_colors, void (*callback)(puyos_t*, int, int)) {
    while(1) {
        animate_bottom_gravity(floor, num_links, num_colors, callback);
        int dummy;
        if (!clear_bottom_groups(floor, num_colors, &dummy)) {
            break;
        }
        usleep(50 * ANIM_DELAY);
    }
}
