void benchmark_resolve(unsigned long iterations) {
    state *s = malloc(sizeof(state));
    srand(time(NULL));

    unsigned long total_chain = 0;
    for (unsigned long k = 0; k < iterations; ++k) {
        for (int j = 0; j < NUM_FLOORS; j++) {
            for (int i = 0; i < NUM_COLORS; ++i) {
                s->floors[j][i] = 0;
            }
        }
        for (int i = 0; i < WIDTH * HEIGHT; ++i) {
            for (int j = 0; j < NUM_FLOORS; ++j) {
                int color = rand() % NUM_COLORS;
                s->floors[j][color] |= 1ULL << i;
            }
        }
        total_chain += resolve(s, NULL);
    }
    printf("%lu\n", total_chain);
}
