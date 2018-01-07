#include <assert.h>
#include <stdio.h>

#include "jkiss/jkiss.h"

#include "puyobot/multiplayer.h"
#include "puyobot/solver/policy.h"
#include "puyobot/solver/game_policy.h"

#define NUM_PLAYERS (2)
#define NUM_DEALS (3)

#define NUM_BOTS (4)

content_t random_but_alive_game_policy(Game *game, int player_index) {
    Player *player = game->players + player_index;
    if (player->chain) {
        return CHOICE_PASS;
    }
    State *state = &player->state;
    return random_but_alive_policy(state, game->deals + player->deal_index, game->num_deals);
}

void init_all() {
    jkiss_init();
    // init_tetrominoes();
}

int main(int argc, char *argv[]) {
    init_all();

    for (int i = 0; i < 100; ++i) {
        printf("\n");
    }

    game_policy_fun bots[NUM_BOTS] = {
        random_but_alive_game_policy,
        simple_search_game_policy,
        monte_carlo_game_policy,
        gcn_game_policy,
    };

    content_t choices[NUM_PLAYERS];
    long long int points[NUM_BOTS * NUM_BOTS] = {0};

    size_t iterations = 0;
    while (1) {
        printf("Round %zu, fight!\n", iterations++);
        for (int i = 0; i < NUM_BOTS; ++i) {
        for (int j = i; j < NUM_BOTS; ++j) {
            Game *game = new_game(NUM_PLAYERS, NUM_DEALS);

            while(!(game->players[0].game_overs || game->players[1].game_overs)) {
                choices[0] = bots[i](game, 0);
                choices[1] = bots[j](game, 1);

                step_game(game, choices);
                // print_player(game->players + 0);
                // print_player(game->players + 1);
            }

            points[i + NUM_BOTS * j] += game->players[1].game_overs - game->players[0].game_overs;
            printf("Result %d vs. %d game overs: %d / %d\n", i, j, game->players[0].game_overs, game->players[1].game_overs);
            printf("Tally so far %d vs. %d: %lld\n", i, j, points[i + NUM_BOTS * j]);
            free_game(game);
        }}
    }

    return 0;
}
