#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

#include "constants.c"
#include "jkiss.c"
#include "util.c"
#include "bitboard.c"
#include "scoring.c"
#include "state.c"

#include "deal.c"
#include "todo.c"
#include "eval.c"
#include "multiplayer.c"
#include "tree.c"
#include "template.c"
#include "template_gen.c"
#include "demo.c"
#include "test.c"
#include "benchmark.c"
#include "info.c"

void init_all() {
    jkiss_init();
    init_tetrominoes();
}

content_t gcn_game_policy(game *g, int player_index) {
    practice_game *pg = game_as_practice(g, player_index);
    if (!pg) {
        return CHOICE_PASS;
    }
    content_t choice = gcn_practice_policy(pg, pg->deals, pg->num_deals);
    free(pg);
    return choice;
}

content_t mc_game_policy(game *g, int player_index) {
    practice_game *pg = game_as_practice(g, player_index);
    if (!pg) {
        return CHOICE_PASS;
    }
    mc_options options = simple_mc_options(10000, random_policy);
    options.step = step_practice;
    options.copy = copy_practice;
    content_t choice = iterate_mc(pg, pg->deals, pg->num_deals, options);
    free(pg);
    return choice;
}

int main(int argc, char *argv[]) {
    init_all();
    test_all();

    for (int i = 0; i < 100; ++i) {
        printf("\n");
    }

    game *g = new_game(2, 3);
    for (int i = 0; i < 1000; ++i) {
        content_t choices[2] = {
            gcn_game_policy(g, 0),
            mc_game_policy(g, 1)
        };
        step_game(g, choices);
        print_player(g->players);
        print_player(g->players + 1);
        usleep(50000);
    }
    return 0;
}
