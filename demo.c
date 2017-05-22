#include "animate.c"
#define NUM_DEALS (3)

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

void policy_demo(int do_animation, size_t iterations, policy_fun policy) {
    state *s = calloc(1, sizeof(state));
    content_t deals[NUM_DEALS];
    int total_score = 0;
    double puyos_played = 0;
    int game_overs = 0;
    int max_chain = 0;

    print_state(s);
    void cb(state *s) {
        redraw_state(s);
        print_deals(deals, 3);
        printf("score=%d, max chain=%d, efficiency=%f, game_overs=%d                       \n\033[A", total_score, max_chain, total_score / puyos_played, game_overs);
    }

    for (int i = 0; i < NUM_DEALS; ++i) {
        deals[i] = rand_piece();
    }

    for (int i = 0; i < iterations; ++i) {
        content_t choice = policy(s, deals, NUM_DEALS);
        if (!apply_deal_and_choice(s, deals[0], choice)) {
            game_overs++;
            clear_state(s);
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
        for (int j = 0; j < NUM_DEALS - 1; ++j) {
            deals[j] = deals[j + 1];
        }
        deals[NUM_DEALS - 1] = rand_piece();
    }
    printf("\nDone\n");
}

void demo(int do_animation, size_t iterations, eval_fun f, float tree_value_multiplier) {
    content_t policy(state *s, content_t *deals, size_t num_deals) {
        return solve(s, deals, num_deals, 0, f, tree_value_multiplier);
    }
    policy_demo(do_animation, iterations, policy);
}

// TODO: Convert into a policy for the policy demo.
void mc_demo(int do_animation, size_t iterations, policy_fun policy) {
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
