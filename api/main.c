#include <assert.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../constants.c"
#include "..jkiss/include/jkiss/jkiss.h"
#include "..jkiss/src/jkiss.c"  // TODO: Use CMake
#include "../util.c"
#include "../bitboard.c"
#include "../scoring.c"
#include "../state.c"
#include "../deal.c"
#include "../multiplayer.c"

#include "../eval.c"
#include "../tree.c"
#include "../template.c"
#include "../template_gen.c"
#include "../todo.c"
#include "../harassment.c"
#include "../complex_policy.c"

void write_game(game *g, FILE *f) {
    fwrite(g, sizeof(game), 1, f);
    fwrite(g->players, sizeof(player), g->num_players, f);
    fwrite(g->deals, sizeof(content_t), g->total_num_deals, f);
}

game* read_game(FILE *f) {
    game *g = malloc(sizeof(game));
    assert(fread(g, sizeof(game), 1, f));
    g->players = malloc(sizeof(player) * g->num_players);
    g->deals = malloc(sizeof(content_t) * g->total_num_deals);
    assert(fread(g->players, sizeof(player), g->num_players, f));
    assert(fread(g->deals, sizeof(content_t), g->total_num_deals, f));
    return g;
}

void init_all() {
    jkiss_init();
    init_tetrominoes();
}

int main(int argc, char *argv[]) {
    init_all();

    int player_index = atoi(argv[1]);

    FILE *f = fopen("in.bin", "rb");
    game *g = read_game(f);
    fclose(f);

    print_player(g->players);
    print_player(g->players + 1);
    content_t choice = gcn_game_policy(g, player_index);
    apply_deal_and_choice(&g->players[player_index].state, g->deals[g->players[player_index].deal_index], choice);

    f = fopen("out.bin", "wb");
    write_game(g, f);
    fclose(f);

    free_game(g);
    return 0;
}
