#include <assert.h>
#include <stdlib.h>

#include "jkiss/jkiss.h"

#include "puyobot/solver/simul_tree.h"
#include "puyobot/multiplayer.h"

void test_iterate() {
    Game *game = new_game(NUM_PLAYERS, 1);

    SimulNode *root = mc_init(game);
    McOptions options = get_mc_options();
    mc_iterate(game, root, 1, options);
    mc_free(root);
}

int main() {
    jkiss_init();

    test_iterate();
}
