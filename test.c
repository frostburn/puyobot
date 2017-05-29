void reference_gravity(state *s) {
    assert(NUM_FLOORS == 2);
    puyos_t all[NUM_FLOORS];
    get_state_mask(s, all);
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
    puyos_t result = 0;
    for (int i = 0; i < 10000; ++i) {
        result |= jlrand();
    }
    assert(result == ~0);
}

void test_gravity() {
    state *s = calloc(1, sizeof(state));
    state *ref = calloc(1, sizeof(state));
    int puyo_count = 0;
    for (int j = 0; j < NUM_FLOORS; j++) {
        puyos_t allowed = FULL;
        for (int i = 0; i < NUM_COLORS; ++i) {
            puyos_t candidates = jlrand() & jlrand() & jlrand();
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
    for (int i = 0; i < WIDTH * HEIGHT; ++i) {
        for (int j = 0; j < NUM_FLOORS; ++j) {
            int color = jrand() % NUM_COLORS;
            s->floors[j][color] |= 1ULL << i;
        }
    }
    for (test_color = 0; test_color < NUM_COLORS - 1; ++test_color) {
        for (int i = 0; i < WIDTH * HEIGHT; ++i) {
            test_group = flood(1ULL << i, s->floors[1][test_color]);
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
    clear_groups(s, 0);
    print_state(s);
    if (test_group) {
        assert(!(s->floors[1][test_color] & test_group));
        assert(!(s->floors[1][GARBAGE] & cross(test_group)));
    }
}

void test_clear_with_shift() {
    puyos_t noise = 3833423454597982578ULL & FULL;
    for (int i = GHOST_Y + 1; i < HEIGHT + 1; ++i) {
        state *s = calloc(1, sizeof(state));
        s->floors[0][0] = noise;
        for (int j = 0; j < i; ++j) {
            shift_down(s);
        }
        print_state(s);
        printf("%d, %d\n", popcount(s->floors[0][0]), popcount(s->floors[1][0]));
        assert(popcount(s->floors[0][0]) + popcount(s->floors[1][0]) == 29);
        clear_groups(s, 0);
        print_state(s);
        assert(popcount(s->floors[0][0]) + popcount(s->floors[1][0]) == 12);
        free(s);
    }
}

void test_ghost_chain() {
    state *s = calloc(1, sizeof(state));
    s->floors[1][GARBAGE] = FULL;
    s->floors[0][GARBAGE] = ~DEATH_BLOCK;
    s->floors[0][RED] = 15ULL << (V_SHIFT * (GHOST_Y + 1));
    s->floors[0][RED] |= 3ULL << (2 + V_SHIFT * (GHOST_Y));
    s->floors[0][RED] |= 3ULL << (4 + V_SHIFT * (GHOST_Y + 2));
    s->floors[0][GARBAGE] &= ~s->floors[0][RED];
    print_state(s);
    int chain = 0;
    resolve(s, &chain);
    print_state(s);
    assert(chain == 2);
    assert(!s->floors[0][RED]);
}

void test_euler() {
    puyos_t noise = jlrand() & FULL & ~(TOP | BOTTOM | LEFT_WALL | RIGHT_WALL);
    int e = euler(noise);
    print_puyos(noise);
    printf("e=%d\n", e);
    assert(e == euler(noise >> H_SHIFT));
    assert(e == euler(noise << H_SHIFT));
    assert(e == euler(noise >> V_SHIFT));
    assert(e == euler(noise >> V_SHIFT));
}

void test_state_euler() {
    state *s = calloc(1, sizeof(state));
    s->floors[0][0] = jlrand() & FULL & ~(LEFT_WALL | RIGHT_WALL);
    s->floors[1][0] = jlrand() & FULL & ~(BOTTOM | LEFT_WALL | RIGHT_WALL);
    int e = state_euler(s);
    print_state(s);
    printf("e=%d\n", e);
    s->floors[0][0] <<= 1;
    s->floors[1][0] <<= 1;
    assert(e == state_euler(s));
    s->floors[0][0] >>= 2;
    s->floors[1][0] >>= 2;
    assert(e == state_euler(s));
    shift_down(s);
    assert(e == state_euler(s));
    s->floors[0][0] <<= 2;
    s->floors[1][0] <<= 2;
    assert(e == state_euler(s));
}

void test_chainify() {
    state s = (state){{{0ull, 0ull, 0ull, 0ull, 0ull, 0ull}, {0ull, 361484238840659968ull, 0ull, 18014398509481984ull, 4503599627370496ull, 0ull}}};
    chainify(&s, SHOT_PATIENCE, CHAIN_PATIENCE);
    print_state(&s);
    clear_state(&s);
    chainify(&s, SHOT_PATIENCE, CHAIN_PATIENCE);
    print_state(&s);
}

void test_extend_chain() {
    state *s = calloc(1, sizeof(state));
    s->floors[0][RED] = 1;
    handle_gravity(s);
    for (int i = 0; i < CLEAR_THRESHOLD; ++i) {
        s->floors[0][GREEN] = 1;
        handle_gravity(s);
    }
    for (int i = 0; i < CLEAR_THRESHOLD - 1; ++i) {
        s->floors[0][RED] = 1;
        handle_gravity(s);
    }
    puyos_t fixed[2] = {s->floors[0][RED], s->floors[1][RED]};
    assert(!extend_chain(s, fixed));
}

void test_flood_2() {
    puyos_t noise[2] = {234987234897892347ULL & FULL, 2348972342394729384ULL & FULL};
    int clears [] = {11, 5, 4, 6, 1, 1, 1, 1, 1, 19};
    int num_clears = 10;
    print_puyos_2(noise);
    int j = 0;
    for (int i = 0; i < WIDTH * HEIGHT; i += 2) {
        puyos_t source[2] = {3ULL << i, 0};
        flood_2(source, noise);
        if (source[0] || source[1]) {
            noise[0] ^= source[0];
            noise[1] ^= source[1];
            print_puyos_2(noise);
            int cleared = popcount(source[0]) + popcount(source[1]);
            printf("cleared=%d\n", cleared);
            assert(clears[j++] == cleared);
        }
    }
    assert(j == num_clears);
    noise[0] = 234987234897892347ULL & FULL;
    noise[1] = 2348972342394729384ULL & FULL;
    int clears2[] = {19, 1, 11, 1, 1};
    num_clears = 5;
    j = 0;
    for (int i = 0; i < WIDTH * HEIGHT; i += 2) {
        puyos_t source[2] = {0, 3ULL << i};
        flood_2(source, noise);
        if (source[0] || source[1]) {
            noise[0] ^= source[0];
            noise[1] ^= source[1];
            print_puyos_2(noise);
            int cleared = popcount(source[0]) + popcount(source[1]);
            printf("cleared=%d\n", cleared);
            assert(clears2[j++] == cleared);
        }
    }
    assert(j == num_clears);
}

void test_all() {
    test_lrand();
    test_gravity();
    test_clear();
    test_clear_with_shift();
    test_ghost_chain();
    test_euler();
    test_state_euler();
    test_chainify();
    test_extend_chain();
    test_flood_2();
}
