#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

#include "constants.c"
#include "jkiss/include/jkiss/jkiss.h"
#include "jkiss/src/jkiss.c"
#include "util.c"
#include "bitboard.c"
#include "scoring.c"
#include "state.c"

#include "deal.c"
#include "eval.c"
#include "multiplayer.c"
#include "tree.c"
#include "template.c"
#include "template_gen.c"
#include "todo.c"
#include "harassment.c"
#include "complex_policy.c"
#include "demo.c"
#include "test.c"
#include "benchmark.c"
#include "info.c"

void init_all() {
    jkiss_init();
    init_tetrominoes();
}

int main(int argc, char *argv[]) {
    init_all();
    test_all();

    for (int i = 0; i < 100; ++i) {
        printf("\n");
    }

    game *g = new_game(2, 3);
    for (int i = 0; i < 10000; ++i) {
        content_t choices[2] = {
            gcn_game_policy(g, 0),
            gcnk_game_policy(g, 1),
        };
        step_game(g, choices);
        print_player(g->players);
        print_deals(g->deals + g->players[0].deal_index, g->num_deals);
        print_player(g->players + 1);
        print_deals(g->deals + g->players[1].deal_index, g->num_deals);
        //usleep(50000);
    }
    return 0;
}
