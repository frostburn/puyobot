#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "puyobot/solver/game_policy.h"
#include "puyobot/solver/policy.h"
#include "puyobot/solver/eval.h"

double _eval_gcn_practice(void *_pg) {
    PracticeGame *pg = _pg;
    State *s = &pg->player.state;
    double groups = eval_groups(s);
    double chains = eval_chains(s);
    double nuisance = popcount(s->floors[0][GARBAGE]) + popcount(s->floors[1][GARBAGE]);

    return 30 * groups + 20 * chains - nuisance * 0.96 - pg->incoming / (0.3 + 0.01 * pg->delay);
}

content_t gcn_practice_policy(void *_pg, content_t *deals, int num_deals) {
    PracticeGame *pg = _pg;
    SearchOptions options = simple_search_options(_eval_gcn_practice, 1, 50);
    options.step = step_practice;
    options.copy = copy_practice;
    options.choice_sets = malloc((num_deals + 1) * sizeof(choice_set_t));
    for (int i = 0; i < num_deals + 1; ++i) {
        options.choice_sets[i] = CHOICE_SET_ALL;
    }
    if (!pg->incoming && state_popcount(&pg->player.state) + 24 < TOTAL_SPACE) {
        options.choice_sets[0] = filter_chains(&pg->player.state, deals[0], 0);
        if (!options.choice_sets[0]) {
            options.choice_sets[0] = CHOICE_SET_ALL;
        }
    }
    return rand_choice(solve(pg, deals, num_deals, options));
}

content_t gcn_game_policy(Game *g, int player_index) {
    PracticeGame *pg = game_as_practice(g, player_index);
    if (!pg) {
        return CHOICE_PASS;
    }
    content_t choice = gcn_practice_policy(pg, pg->deals, pg->num_deals);
    free(pg);
    return choice;
}

void multi_random_policy(void *g, content_t *choices) {
    choices[0] = CHOICES[jrand() % NUM_CHOICES];
    choices[1] = CHOICES[jrand() % NUM_CHOICES];
}

void multi_random_alive_policy(void *g, content_t *choices) {
    Game *game = g;
    for (int i = 0; i < game->num_players; ++i) {
        choices[i] = random_but_alive_policy(&game->players[i].state, game->deals + game->players[i].deal_index, game->num_deals);
    }
}
