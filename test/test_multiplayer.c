#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jkiss/jkiss.h"

#include "puyobot/multiplayer.h"

#define TEST_GAME_ITERATIONS (5000)

void test_mirror_game() {
    Game *g = new_game(2, 3);
    for (int i = 0; i < TEST_GAME_ITERATIONS; ++i) {
        content_t choices[2];
        choices[0] = CHOICES[jrand() % NUM_CHOICES];
        choices[1] = choices[0];
        step_game(g, choices);
        assert(!g->players[0].pending_nuisance);
        assert(!g->players[1].pending_nuisance);
        assert(!g->players[0].state.floors[0][GARBAGE]);
        assert(!g->players[0].state.floors[1][GARBAGE]);
        assert(!g->players[1].state.floors[0][GARBAGE]);
        assert(!g->players[1].state.floors[1][GARBAGE]);
    }
    free_game(g);
}

void test_game_symmetry() {
    Game *g = new_game(2, 3);
    content_t choices[2*TEST_GAME_ITERATIONS];
    for (int i = 0; i < TEST_GAME_ITERATIONS; ++i) {
        choices[2*i] = CHOICES[jrand() % NUM_CHOICES];
        choices[2*i + 1] = CHOICES[jrand() % NUM_CHOICES];
        step_game(g, choices + 2*i);
    }
    int scores[2] = {
        g->players[0].total_score,
        g->players[1].total_score,
    };
    int game_overs[2] = {
        g->players[0].game_overs,
        g->players[1].game_overs
    };
    int indices[2] = {
        g->players[0].deal_index,
        g->players[1].deal_index
    };
    print_player(g->players);
    print_player(g->players + 1);
    int total_num_deals = g->total_num_deals;
    content_t *deals = malloc(total_num_deals * sizeof(content_t));
    memcpy(deals, g->deals, total_num_deals * sizeof(content_t));
    free_game(g);

    g = new_game(2, 3);
    g->deals = realloc(g->deals, total_num_deals * sizeof(content_t));
    g->total_num_deals = total_num_deals;
    memcpy(g->deals, deals, total_num_deals * sizeof(content_t));
    free(deals);
    for (int i = 0; i < TEST_GAME_ITERATIONS; ++i) {
        content_t rev_choices[2] = {choices[2*i + 1], choices[2*i]};
        step_game(g, rev_choices);
    }
    print_player(g->players + 1);
    print_player(g->players);
    assert(scores[0] == g->players[1].total_score);
    assert(scores[1] == g->players[0].total_score);
    assert(game_overs[1] == g->players[0].game_overs);
    assert(game_overs[0] == g->players[1].game_overs);
    assert(indices[0] == g->players[1].deal_index);
    assert(indices[1] == g->players[0].deal_index);
    free_game(g);
}

void test_game_all_clear() {
    Game *g = new_game(2, 6);
    int x = 3;
    int o = 0;
    content_t choices[2] = {0, make_choice(&x, &o)};
    g->deals[0] = 0;
    g->deals[1] = 0;
    g->deals[2] = make_piece(1, 2);
    g->deals[3] = make_piece(3, 3);
    g->deals[4] = make_piece(3, 3);
    g->deals[5] = make_piece(4, 1);

    step_game(g, choices);
    choices[1] = 0;
    step_game(g, choices);
    x = 5;
    choices[1] = make_choice(&x, &o);
    step_game(g, choices);

    print_player(g->players);
    print_player(g->players + 1);

    assert(g->players[0].all_clear_bonus);
    assert(!g->players[1].state.floors[NUM_FLOORS - 1][GARBAGE]);

    PracticeGame *pg = game_as_practice(g, 1);
    assert(!pg->incoming);
    print_practice(pg);
    free(pg);

    step_game(g, choices);
    choices[1] = 0;
    step_game(g, choices);
    step_game(g, choices);

    pg = game_as_practice(g, 1);
    assert(pg->incoming > 1);
    print_practice(pg);
    free(pg);

    step_game(g, choices);
    print_player(g->players);
    print_player(g->players + 1);

    assert(!g->players[0].all_clear_bonus);
    assert(g->players[1].state.floors[NUM_FLOORS - 1][GARBAGE]);

    free_game(g);
}

void test_practice_time_and_clear() {
    PracticeGame *pg = calloc(1, sizeof(PracticeGame));
    pg->num_deals = 3;
    pg->delay = 100;
    pg->incoming = 100;

    double score;
    step_practice(pg, 0, 0, &score);
    print_practice(pg);
    assert(pg->delay == 99);
    step_practice(pg, 0, 0, &score);
    assert(pg->delay == 97);
    assert(pg->player.all_clear_bonus);
    print_practice(pg);
    step_practice(pg, 0, 0, &score);
    print_practice(pg);
    step_practice(pg, 0, 0, &score);
    print_practice(pg);
    print_deals(pg->deals, pg->num_deals);
    assert(pg->delay == 94);
    assert(pg->incoming < 80);
}

int main() {
    test_game_symmetry();
    test_mirror_game();
    test_practice_time_and_clear();
}
