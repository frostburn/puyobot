#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "puyobot/animate.h"
#include "puyobot/solver/policy.h"
#include "puyobot/solver/tree.h"

#define NUM_DEALS (3)

void print_deals_on_the_side(content_t *deals, int num_deals) {
    printf("                   \033[A\033[A\033[A");
    for (int i = 0; i < num_deals; ++i) {
        content_t color1 = deal_color1(deals[i]);
        content_t color2 = deal_color2(deals[i]);
        printf("\x1b[3%d;1m ●", color1 + 1);
        printf("\x1b[3%d;1m ●", color2 + 1);
        printf("  ");
    }
    printf("\x1b[0m\033[B\033[B\n");
}

void policy_demo(State *s, int do_animation, size_t iterations, policy_fun policy) {
    content_t deals[NUM_DEALS];
    int total_score = 0;
    double puyos_played = 0;
    int game_overs = 0;
    int max_chain = 0;

    if (do_animation >= 0) {
        print_state(s);
    }
    void cb(State *s) {
        redraw_state(s);
        print_deals_on_the_side(deals + 1, 2);
        float efficiency = 0;
        if (puyos_played) {
            efficiency = total_score / puyos_played;
        }
        printf("score=%d, max chain=%d, efficiency=%f, game_overs=%d           \n\033[A", total_score, max_chain, efficiency, game_overs);
    }

    for (int i = 0; i < NUM_DEALS; ++i) {
        deals[i] = rand_piece();
    }

    for (size_t i = 0; i < iterations; ++i) {
        content_t choice = policy(s, deals, NUM_DEALS);
        if (!apply_deal_and_choice(s, deals[0], choice)) {
            game_overs++;
            clear_state(s);
        }
        State *c = copy_state(s);
        if (do_animation >= 1) {
            animate(c, cb);
        } else if (do_animation == 0) {
            cb(c);
        }
        free(c);
        int chain = 0;
        int score = resolve(s, &chain);
        if (chain) {
            printf("Last chain = %d                                                  \n\n", chain);
            print_state(s);
            cb(s);
        }
        total_score += score;
        puyos_played += 2;
        if (chain > max_chain) {
            max_chain = chain;
        }
        for (int j = 0; j < NUM_DEALS - 1; ++j) {
            deals[j] = deals[j + 1];
        }
        deals[NUM_DEALS - 1] = rand_piece();
        if (do_animation >= 2) {
            c = copy_state(s);
            apply_deal_and_choice(c, deals[0], 2 | CHOICE_0);
            cb(c);
            free(c);
            printf("\033[A");
            getchar();
        }
    }
    printf("\nDone\n");
}

void eval_demo(int do_animation, size_t iterations, eval_fun eval, int depth, float tree_factor) {
    content_t policy(void *s, content_t *deals, int num_deals) {
        SearchOptions options = simple_search_options(eval, depth, tree_factor);
        return rand_choice(solve(s, deals, num_deals, options));
    }
    State *s = calloc(1, sizeof(State));
    policy_demo(s, do_animation, iterations, policy);
}

/*
size_t show_chain(int min_links, int use_extensions, int use_tailing) {
    State *s = calloc(1, sizeof(State));
    size_t iter = 0;
    int chain;
    while (1) {
        ++iter;
        clear_state(s);
        chain = 0;
        if (use_extensions) {
            while(extend_chain(s, 0)) ++chain;
        }
        if (use_tailing) {
            while(tail_chain(s)) ++chain;
        }
        if (chain >= min_links) {
            break;
        }
    }
    if (iter == 1) {
        printf("It took 1 iteration to find this %d-chain\n", chain);
    } else {
        printf("It took %zu iterations to find this %d-chain\n", iter, chain);
    }
    print_state(s);
    print_state(s);
    animate(s, redraw_state);
    return iter;
}
*/

int main() {
    jkiss_init();

    State *state = calloc(1, sizeof(State));
    policy_demo(state, 0, 1000, gcs_policy);

    return 0;
}
