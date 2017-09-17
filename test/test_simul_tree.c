#include <assert.h>
#include <stdlib.h>

#include "jkiss/jkiss.h"

#include "puyobot/solver/simul_tree.h"
#include "puyobot/multiplayer.h"

void test_iterate() {
    Game *game = new_game(NUM_PLAYERS, 1);

    SimulNode *root = simul_mc_init(game);
    SimulMcOptions options = get_simul_mc_options();
    simul_mc_iterate(game, root, 1, options);
    simul_mc_free(root);
}

int main() {
    jkiss_init();

    test_iterate();
}
