void reference_gravity(state *s) {
    assert(NUM_FLOORS == 2);
    puyos_t all[2];
    for (int i = 0; i < NUM_FLOORS; ++i) {
        all[i] = 0;
        for (int j = 0; j < NUM_COLORS; ++j) {
            all[i] |= s->floors[i][j];
        }
    }
    for (int i = 0; i < WIDTH; ++i) {
        int pile_size = 0;
        for (int j = 0; j < TOTAL_HEIGHT; ++j) {
            int scanner_floor = 1;
            int scanner_y = HEIGHT - 1 - j;
            if (j >= HEIGHT) {
                scanner_floor = 0;
                scanner_y = 2 * HEIGHT - 1 - j;
            }
            puyos_t scanner = 1ULL << (scanner_y * WIDTH + i);
            if (!(scanner & all[scanner_floor])) {
                continue;
            }
            for (int k = 0; k < NUM_COLORS; ++k) {
                if (scanner & s->floors[scanner_floor][k]) {
                    s->floors[scanner_floor][k] ^= scanner;
                    int pile_floor = 1;
                    int pile_y = HEIGHT - 1 - pile_size;
                    if (pile_size >= HEIGHT) {
                        pile_floor = 0;
                        pile_y = 2 * HEIGHT - 1 - pile_size;
                    }
                    s->floors[pile_floor][k] |= 1ULL << (pile_y * WIDTH + i);
                    ++pile_size;
                    break;
                }
            }
        }
    }
}

void test_lrand() {
    srand(time(NULL));
    puyos_t result = 0;
    for (int i = 0; i < 10000; ++i) {
        result |= lrand();
    }
    assert(result == ~0);
}

void test_gravity() {
    state *s = calloc(1, sizeof(state));
    state *ref = calloc(1, sizeof(state));
    srand(time(NULL));
    int puyo_count = 0;
    for (int j = 0; j < NUM_FLOORS; j++) {
        puyos_t allowed = FULL;
        for (int i = 0; i < NUM_COLORS; ++i) {
            puyos_t candidates = lrand() & lrand() & lrand();
            candidates &= allowed;
            allowed ^= candidates;
            s->floors[j][i] = candidates;
            puyo_count += popcount(candidates);
            ref->floors[j][i] = s->floors[j][i];
        }
    }
    print_state(s);
    handle_gravity(s);
    reference_gravity(ref);
    print_state(s);
    print_state(ref);
    int new_puyo_count = 0;
    for (int j = 0; j < NUM_FLOORS; j++) {
        for (int i = 0; i < NUM_COLORS; ++i) {
            new_puyo_count += popcount(s->floors[j][i]);
            assert(ref->floors[j][i] == s->floors[j][i]);
        }
    }
    assert(puyo_count == new_puyo_count);
}

void test_clear() {
    int test_color;
    puyos_t test_group;
    state *s = calloc(1, sizeof(state));
    srand(time(NULL));
    for (int i = 0; i < WIDTH * HEIGHT; ++i) {
        for (int j = 0; j < NUM_FLOORS; ++j) {
            int color = rand() % NUM_COLORS;
            s->floors[j][color] |= 1ULL << i;
        }
    }
    for (test_color = 0; test_color < NUM_COLORS - 1; ++test_color) {
        for (int i = 0; i < WIDTH * HEIGHT; ++i) {
            test_group = flood(1ULL << i, s->floors[0][test_color]);
            if (popcount(test_group) >= CLEAR_THRESHOLD) {
                break;
            }
            else {
                test_group = 0;
            }
        }
        if (popcount(test_group) >= CLEAR_THRESHOLD) {
            break;
        }
    }
    print_state(s);
    print_puyos(test_group);
    clear_groups(s);
    print_state(s);
    if (test_group) {
        assert(!(s->floors[0][test_color] & test_group));
        assert(!(s->floors[0][GARBAGE] & cross(test_group)));
    }
}

void test_clear_with_shift() {
    puyos_t noise = 3833423454597982578ULL & FULL;
    for (int i = 0; i < HEIGHT + 1; ++i) {
        state *s = calloc(1, sizeof(state));
        s->floors[0][0] = noise;
        for (int j = 0; j < i; ++j) {
            shift_down(s);
        }
        print_state(s);
        printf("%d, %d\n", popcount(s->floors[0][0]), popcount(s->floors[1][0]));
        assert(popcount(s->floors[0][0]) + popcount(s->floors[1][0]) == 29);
        clear_groups(s);
        print_state(s);
        assert(popcount(s->floors[0][0]) + popcount(s->floors[1][0]) == 12);
        free(s);
    }
}
