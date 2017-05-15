#include "animate.c"

void print_deals(content_t *deals, int num_deals) {
    printf("                   \033[A\033[A\033[A");
    for (int i = 1; i < num_deals; ++i) {
        content_t color1 = deal_color1(deals[i]);
        content_t color2 = deal_color2(deals[i]);
        printf("\x1b[3%d;1m ●", color1 + 1);
        printf("\x1b[3%d;1m ●", color2 + 1);
        printf("  ");
    }
    printf("\x1b[0m\033[B\033[B\n");
}

void policy_demo(policy_fun policy) {
    srand(time(NULL));
    state *s = calloc(1, sizeof(state));
    content_t deals[3];
    int total_score = 0;
    double puyos_played = 0;
    int game_overs = 0;

    for (int i = 0; i < 3; ++i) {
        deals[i] = rand_piece();
    }

    for (int i = 0; i < 3000; ++i) {
        content_t choice = policy(s, deals, 3);
        if (!apply_deal_and_choice(s, deals[0], choice)) {
            printf("Game Over\n");
            game_overs++;
            clear_state(s);
        }
        print_state(s);
        int score = resolve(s, NULL);
        total_score += score;
        puyos_played += 2;
        printf("score=%d, efficiency=%f, game overs=%d\n", score, total_score / puyos_played, game_overs);
        for (int j = 0; j < 2; ++j) {
            deals[j] = deals[j + 1];
        }
        deals[2] = rand_piece();
    }
}

void demo() {
    content_t policy(state *s, content_t *deals, size_t num_deals) {
        return solve(s, deals, num_deals, 0, &eval_fun_weighted);
    }
    policy_demo(policy);
}

void mc_demo(int do_animation, size_t iterations, policy_fun policy) {
    srand(time(NULL));
    state *s = calloc(1, sizeof(state));
    content_t deals[3];
    int max_chain = 0;
    int total_score = 0;
    double puyos_played = 0;

    for (int i = 0; i < 3; ++i) {
        deals[i] = rand_piece();
    }

    for (int i = 0; i < 10000; ++i) {
        content_t choice = iterate_mc(s, deals, 3, iterations, policy);
        if (!apply_deal_and_choice(s, deals[0], choice)) {
            print_state(s);
            printf("Game Over\n");
            return;
        }

        void cb(state *s) {
            print_state(s);
            print_deals(deals, 3);
            printf("score=%d, max chain=%d, efficiency=%f\n", total_score, max_chain, total_score / puyos_played);
        }

        state *c = copy_state(s);
        if (do_animation) {
            animate(c, cb);
        } else {
            cb(c);
        }
        free(c);
        int chain = 0;
        int score = resolve(s, &chain);
        total_score += score;
        puyos_played += 2;
        if (chain > max_chain) {
            max_chain = chain;
        }
        for (int j = 0; j < 2; ++j) {
            deals[j] = deals[j + 1];
        }
        deals[2] = rand_piece();
    }
}
