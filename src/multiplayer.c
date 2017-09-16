#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "puyobot/multiplayer.h"

void print_player(Player *p) {
    print_state(&p->state);
    double sending = (p->chain_score + p->leftover_score) / (double) TARGET_SCORE;
    printf(
        "score=%d chain=%d sending=%.2f receiving=%d all clear=%d/%d game_overs=%d\n",
        p->total_score, p->chain, sending, p->pending_nuisance, p->chain_all_clear_bonus, p->all_clear_bonus, p->game_overs
    );
}

void print_practice(PracticeGame *pg) {
    print_player(&pg->player);
    printf(
        "incoming=%d/%d\n",
        pg->incoming, pg->delay
    );
}

Game* new_game(int num_players, int num_deals) {
    Game *g = calloc(1, sizeof(Game));
    g->num_players = num_players;
    g->players = calloc(num_players, sizeof(Player));
    g->num_deals = num_deals;
    g->total_num_deals = 1 << 4;
    while (g->total_num_deals < num_deals) {
        g->total_num_deals <<= 1;
    }
    g->deals = malloc(g->total_num_deals * sizeof(content_t));
    for (int i = 0; i < g->total_num_deals; ++i) {
        g->deals[i] = rand_piece();
    }
    return g;
}

void free_game(void *game) {
    Game *g = game;
    free(g->players);
    free(g->deals);
    free(g);
}

void clear_player(Player *p) {
    clear_state(&p->state);
    p->chain = 0;
    p->chain_score = 0;
    p->total_score = 0;
    p->all_clear_bonus = 0;
    p->chain_all_clear_bonus = 0;
    p->pending_nuisance = 0;
    p->leftover_score = 0;
    p->nuisance_x = 0;
}

int send_nuisance(Player *p) {
    // Send garbage
    int nuisance_sent = (p->chain_score + p->leftover_score) / TARGET_SCORE;
    p->leftover_score = (p->chain_score + p->leftover_score) % TARGET_SCORE;
    p->chain_score = 0;
    // Offset garbage
    if (p->pending_nuisance >= nuisance_sent) {
        p->pending_nuisance -= nuisance_sent;
        nuisance_sent = 0;
    } else {
        nuisance_sent -= p->pending_nuisance;
        p->pending_nuisance = 0;
    }
    return nuisance_sent;
}

void receive_nuisance(Player *p) {
    // Receive garbage
    int nuisance_received = 0;
    if (p->pending_nuisance) {
        nuisance_received = p->pending_nuisance;
        if (nuisance_received > MAX_NUISANCE_ROWS * WIDTH) {
            nuisance_received = MAX_NUISANCE_ROWS * WIDTH;
        }
        p->pending_nuisance -= nuisance_received;
        int row = 0;
        while (nuisance_received) {
            int nuisance_placed = nuisance_received;
            if (nuisance_placed > WIDTH) {
                nuisance_placed = WIDTH;
            }
            nuisance_received -= nuisance_placed;
            while(nuisance_placed) {
                puyos_t n = 1ULL << (p->nuisance_x + row * V_SHIFT);
                p->state.floors[0][GARBAGE] |= n;
                p->nuisance_x = (p->nuisance_x + 1) % WIDTH;
                --nuisance_placed;
            }
            ++row;
        }
        handle_gravity(&p->state);
        kill_puyos(&p->state);
    }
}

int step_player(Player *p) {
    handle_gravity(&p->state);
    kill_puyos(&p->state);
    int score = clear_groups(&p->state, p->chain);
    if (score) {
        ++p->chain;
        p->chain_score += score;
        p->total_score += score;
        int all_clear_bonus = 1;
        for (int i = 0; i < NUM_COLORS; ++i) {
            if (p->state.floors[NUM_FLOORS-1][i]) {
                all_clear_bonus = 0;
                break;
            }
        }
        if (all_clear_bonus) {
            p->chain_all_clear_bonus = 1;
        }
    } else {
        if (p->chain) {
            if (p->all_clear_bonus) {
                p->chain_score += MAX_NUISANCE_ROWS * WIDTH * TARGET_SCORE;
                p->total_score += MAX_NUISANCE_ROWS * WIDTH * TARGET_SCORE;
            }
            p->all_clear_bonus = p->chain_all_clear_bonus;
            p->chain_all_clear_bonus = 0;
        }
        p->chain = 0;
    }
    return score;
}

double step_game(void *game, content_t *choices) {
    Game *g = game;
    int nuisance_receivers[MAX_PLAYERS] = {0};
    int has_stepped[MAX_PLAYERS] = {0};
    int game_over = 0;
    double result = NAN;
    for (int i = 0; i < g->num_players; ++i) {
        Player *p = g->players + i;
        if (p->chain) {
            step_player(p);
            has_stepped[i] = 1;
            if (!p->chain) {
                int nuisance_sent = send_nuisance(p);
                for (int j = 0; j < g->num_players; ++j) {
                    if (j != i) {
                        g->players[j].pending_nuisance += nuisance_sent;
                    }
                }
                nuisance_receivers[i] = 1;
            }
            if (choices[i] != CHOICE_PASS) {
                game_over = 1;
                ++p->game_overs;
                if (!isnan(result)) {
                    result = 0;
                } else {
                    result = 2 * i - 1;
                }
            }
        }
    }
    // Separate loop to mitigate iteration order issues.
    for (int i = 0; i < g->num_players; ++i) {
        if (has_stepped[i]) {
            continue;
        }
        Player *p = g->players + i;
        int valid = apply_deal_and_choice(&p->state, g->deals[p->deal_index], choices[i]);
        p->chain_score += DROP_SCORE;
        p->total_score += DROP_SCORE;
        handle_gravity(&p->state);
        ++p->deal_index;
        step_player(p);
        if (!p->chain) {
            receive_nuisance(p);
        }
        if (!valid) {
            game_over = 1;
            ++p->game_overs;
            if (!isnan(result)) {
                result = 0;
            } else {
                result = 2 * i - 1;
            }
        }
        // Just some memory management.
        if (p->deal_index + g->num_deals >= g->total_num_deals) {
            g->deals = realloc(g->deals, (g->total_num_deals << 1) * sizeof(content_t));
            for (int i = 0; i < g->total_num_deals; ++i) {
                g->deals[g->total_num_deals + i] = rand_piece();
            }
            g->total_num_deals <<= 1;
        }
    }
    // Separate loop to mitigate iteration order issues.
    for (int i = 0; i < g->num_players; ++i) {
        if (nuisance_receivers[i]) {
            receive_nuisance(g->players + i);
        }
    }
    if (game_over) {
        int max_index = 0;
        for (int i = 0; i < g->num_players; ++i) {
            clear_player(g->players + i);
            if (g->players[i].deal_index > max_index) {
                max_index = g->players[i].deal_index;
            }
        }
        for (int i = 0; i < g->num_players; ++i) {
            g->players[i].deal_index = max_index;
        }
    }
    ++g->time;
    return result;
}

void* copy_game(void *g) {
    Game *game = g;
    Game *c = malloc(sizeof(Game));
    memcpy(c, game, sizeof(Game));
    c->players = malloc(c->num_players * sizeof(Player));
    c->deals = malloc(c->total_num_deals * sizeof(content_t));
    memcpy(c->players, game->players, c->num_players * sizeof(Player));
    memcpy(c->deals, game->deals, c->total_num_deals * sizeof(content_t));
    return c;
}

PracticeGame* game_as_practice(Game *g, int player_index) {
    // If the player is stuck chaining there is no practice state the reflects that.
    if (g->players[player_index].chain) {
        return NULL;
    }
    // Multiple opponents would require multiple incomming channels for practice.
    assert(g->num_players == 2);  // Limit to two for now.
    PracticeGame *pg = calloc(1, sizeof(PracticeGame));
    pg->time = g->time;
    pg->num_deals = g->num_deals;
    pg->player = g->players[player_index];
    for (int j = 0; j < g->num_deals; ++j) {
        pg->deals[j] = g->deals[j + g->players[player_index].deal_index];
    }
    for (int j = g->num_deals; j < MAX_DEALS; ++j) {
        pg->deals[j] = rand_piece();
    }
    handle_gravity(&pg->player.state);
    for (int i = 0; i < g->num_players; ++i) {
        if (i == player_index) {
            continue;
        }
        Player opponent = g->players[i];
        if (opponent.chain) {
            int delay = 0;
            do {
                step_player(&opponent);
                ++delay;
            } while(opponent.chain);
            if (opponent.all_clear_bonus) {
                opponent.chain_score += MAX_NUISANCE_ROWS * WIDTH * TARGET_SCORE;
            }
            // All clear reset omitted.
            // opponent.all_clear_bonus = opponent.chain_all_clear_bonus;
            // opponent.chain_all_clear_bonus = 0;
            int nuisance = send_nuisance(&opponent);
            pg->delay = delay;
            pg->incoming = nuisance;
        }
        return pg;
    }
    assert(0);
}

void* copy_practice(void *pg) {
    void *c = malloc(sizeof(PracticeGame));
    memcpy(c, pg, sizeof(PracticeGame));
    return c;
}

void append_practice_deal(PracticeGame *pg, content_t deal) {
    for (int i = 0; i < MAX_DEALS - 1; ++i) {
        pg->deals[i] = pg->deals[i + 1];
    }
    pg->deals[MAX_DEALS - 1] = deal;
}

int step_practice(void *_pg, content_t deal, content_t choice, double *score) {
    PracticeGame *pg = _pg;
    State *s = &pg->player.state;
    if (pg->delay == 0) {
        pg->player.pending_nuisance += pg->incoming;
        pg->incoming = 0;
    }
    // Assume deal is pg->deals[0] for now.
    if (!apply_deal_and_choice(s, deal, choice)) {
        return 0;
    }
    do {
        step_player(&pg->player);
        ++pg->time;
        --pg->delay;
    } while (pg->player.chain);
    int outgoing = send_nuisance(&pg->player);
    receive_nuisance(&pg->player);
    handle_gravity(s);
    pg->incoming -= outgoing;
    if (pg->incoming < 0) {
        pg->incoming = 0;
        pg->delay = 0;
    }
    outgoing -= pg->incoming;
    append_practice_deal(pg, rand_piece());
    if (pg->delay < 0) {
        pg->delay = 0;
    }
    *score = outgoing;
    return 1;
}
