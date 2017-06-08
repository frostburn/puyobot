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

int main(int argc, char *argv[]) {
    init_all();
    for (int i = 0; i < 100; ++i) {
        printf("\n");
    }

    practice_game *pg = calloc(1, sizeof(practice_game));
    int num_deals = 3;
    for (int i = 0; i < num_deals; ++i) {
        append_practice_deal(pg, rand_piece());
    }
    pg->incoming = WIDTH * 5;
    pg->delay = 20;

    for (int i = 0; i < 1000; ++i) {
        content_t choice = gcn_practice_policy(pg, pg->deals, num_deals);
        step_practice(pg, pg->deals[0], choice);
        print_practice(pg);
        if (i % 30 == 29) {
            pg->incoming = WIDTH * 5;
            pg->delay = 28;
        }
    }
    return 0;
}
