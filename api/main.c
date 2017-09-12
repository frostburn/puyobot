#include <assert.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jkiss/jkiss.h"

#include "puyobot/solver/game_policy.h"
#include "puyobot/template/pattern.h"

void write_game(Game *g, FILE *f) {
    fwrite(g, sizeof(Game), 1, f);
    fwrite(g->players, sizeof(Player), g->num_players, f);
    fwrite(g->deals, sizeof(content_t), g->total_num_deals, f);
}

Game* read_game(FILE *f) {
    Game *g = malloc(sizeof(Game));
    assert(fread(g, sizeof(Game), 1, f));
    g->players = malloc(sizeof(Player) * g->num_players);
    g->deals = malloc(sizeof(content_t) * g->total_num_deals);
    assert(fread(g->players, sizeof(Player), g->num_players, f));
    assert(fread(g->deals, sizeof(content_t), g->total_num_deals, f));
    return g;
}

void init_all() {
    init_tetrominoes();
    jkiss_init();
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        return 1;
    }
    init_all();

    int player_index = atoi(argv[1]);

    FILE *f = fopen("in.bin", "rb");
    Game *g = read_game(f);
    fclose(f);

    print_player(g->players);
    print_player(g->players + 1);
    content_t choice = gcn_game_policy(g, player_index);
    just_apply_deal_and_choice(&g->players[player_index].state, g->deals[g->players[player_index].deal_index], choice);

    f = fopen("out.bin", "wb");
    write_game(g, f);
    fclose(f);

    free_game(g);
    return 0;
}
