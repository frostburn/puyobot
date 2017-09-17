#include <assert.h>
#include <stdio.h>

#include "jkiss/jkiss.h"

#include "puyobot/solver/simul_tree.h"

#define NUM_DEALS (2)

void init_all() {
    jkiss_init();
    // init_tetrominoes();
}

int main(int argc, char *argv[]) {
    init_all();

    for (int i = 0; i < 100; ++i) {
        printf("\n");
    }

    content_t choices[NUM_PLAYERS];

    Game *game = new_game(NUM_PLAYERS, NUM_DEALS);

    for (int i = 0; i < 100; ++i) {
        SimulNode *root = simul_mc_init(game);
        SimulMcOptions options = get_simul_mc_options();
        options.policy = multi_random_alive_policy;
        options.exploration = 0;

        simul_mc_iterate(game, root, 1000, options);

        print_simul_node(root, 0);

        simul_mc_choose(root, choices);
        simul_mc_free(root);

        step_game(game, choices);
        print_player(game->players + 0);
        print_player(game->players + 1);
    }

    return 0;
}
