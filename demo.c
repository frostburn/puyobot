#include "animate.c"

void print_deals(content_t *deals, int num_deals) {
    printf("                   \033[A\033[A\033[A");
    for (int i = 1; i < num_deals; ++i) {
        content_t color1 = deals[i] & COLOR1_MASK;
        content_t color2 = deals[i] >> COLOR2_SHIFT;
        printf("\x1b[3%d;1m ●", color1 + 1);
        printf("\x1b[3%d;1m ●", color2 + 1);
        printf("  ");
    }
    printf("\x1b[0m\033[B\033[B\n");
}

void demo() {
    srand(time(NULL));
    state *s = calloc(1, sizeof(state));
    content_t deals[3];

    for (int i = 0; i < 3; ++i) {
        deals[i] = rand_piece();
    }

    for (int i = 0; i < 10000; ++i) {
        content_t choice = solve(s, deals, 3, 0, &eval_fun_weighted);
        if (!apply_deal_and_choice(s, deals[0], choice)) {
            printf("Game Over\n");
            return;
        }
        print_state(s);
        printf("score=%d\n", resolve(s, NULL));
        for (int j = 0; j < 2; ++j) {
            deals[j] = deals[j + 1];
        }
        deals[2] = rand_piece();
    }
}

void mc_demo() {
    srand(time(NULL));
    state *s = calloc(1, sizeof(state));
    content_t deals[3];
    int max_chain = 0;
    int total_score = 0;

    for (int i = 0; i < 3; ++i) {
        deals[i] = rand_piece();
    }

    for (int i = 0; i < 10000; ++i) {
        content_t choice = iterate_mc(s, deals, 3, 10000);
        if (!apply_deal_and_choice(s, deals[0], choice)) {
            print_state(s);
            printf("Game Over\n");
            return;
        }

        void cb(state *s) {
            print_state(s);
            print_deals(deals, 3);
            printf("score=%d, max chain=%d\n", total_score, max_chain);
        }

        state *c = copy_state(s);
        animate(c, cb);
        free(c);
        int chain = 0;
        int score = resolve(s, &chain);
        total_score += score;
        if (chain > max_chain) {
            max_chain = chain;
        }
        for (int j = 0; j < 2; ++j) {
            deals[j] = deals[j + 1];
        }
        deals[2] = rand_piece();
    }
}
