content_t random_policy(state *s, content_t *deals, size_t num_deals) {
    return CHOICES[rand() % NUM_CHOICES];
}

content_t euler_policy(state *s, content_t *deals, size_t num_deals) {
    double weights[NUM_CHOICES];
    double total_weight = 0;
    for (int i = 0; i < NUM_CHOICES; ++i) {
        state *c = copy_state(s);
        apply_deal_and_choice(c, deals[0], CHOICES[i]);
        double weight = exp(-state_euler(c));
        weights[i] = weight;
        total_weight += weight;
        free(c);
    }
    double p = drand() * total_weight;
    for (int i = 0; i < NUM_CHOICES; ++i) {
        p -= weights[i];
        if (p <= 0) {
            return CHOICES[i];
        }
    }
    return 0;
}
