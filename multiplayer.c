#define TARGET_SCORE (70)
#define MAX_NUISANCE_ROWS (5)
#define MAX_PLAYERS (16)

typedef struct player
{
    state state;
    int deal_index;
    int chain;
    int chain_score;
    int total_score;
    int all_clear_bonus;
    int game_overs;
    int pending_nuisance;
    int leftover_score;
    int nuisance_x;
} player;

typedef struct practice_game
{
    player player;
    int delay;
    int incoming;
    int num_deals;
    content_t deals[MAX_DEALS];
} practice_game;

typedef struct game
{
    int num_players;
    player *players;
    int num_deals;
    int total_num_deals;
    content_t *deals;
} game;

void print_player(player *p) {
    print_state(&p->state);
    double sending = (p->chain_score + p->leftover_score) / (double) TARGET_SCORE;
    printf(
        "score=%d chain=%d sending=%.2f receiving=%d all clear=%d game_overs=%d\n",
        p->total_score, p->chain, sending, p->pending_nuisance, p->all_clear_bonus, p->game_overs
    );
}

void print_practice(practice_game *pg) {
    print_player(&pg->player);
    printf(
        "incoming=%d/%d\n",
        pg->incoming, pg->delay
    );
}

game* new_game(int num_players, int num_deals) {
    game *g = calloc(1, sizeof(game));
    g->num_players = num_players;
    g->players = calloc(num_players, sizeof(player));
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

void free_game(game *g) {
    free(g->players);
    free(g->deals);
    free(g);
}

void clear_player(player *p) {
    clear_state(&p->state);
    p->chain = 0;
    p->chain_score = 0;
    p->total_score = 0;
    p->all_clear_bonus = 0;
    p->pending_nuisance = 0;
    p->leftover_score = 0;
    p->nuisance_x = 0;
}

int send_nuisance(player *p, int score) {
    // Send garbage
    int nuisance_sent = (score + p->leftover_score) / TARGET_SCORE;
    p->leftover_score = (score + p->leftover_score) % TARGET_SCORE;
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

int receive_nuisance(player *p) {
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
    return nuisance_received;
}

int step_player(player *p) {
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
            }
        }
        if (all_clear_bonus) {
            p->all_clear_bonus = 1;
        }
    } else {
        p->chain = 0;
    }
    return score;
}

void step_game(game *g, content_t *choices) {
    int nuisance_receivers[MAX_PLAYERS] = {0};
    for (int i = 0; i < g->num_players; ++i) {
        player *p = g->players + i;
        if (p->chain) {
            step_player(p);
            if (!p->chain) {
                if (p->all_clear_bonus) {
                    p->chain_score += MAX_NUISANCE_ROWS * WIDTH * TARGET_SCORE;
                }
                int nuisance_sent = send_nuisance(p, p->chain_score);
                p->chain_score = 0;
                p->all_clear_bonus = 0;
                for (int j = 0; j < g->num_players; ++j) {
                    if (j != i) {
                        g->players[j].pending_nuisance += nuisance_sent;
                    }
                }
                nuisance_receivers[i] = 1;
            }
        } else {
            int valid = apply_deal_and_choice(&p->state, g->deals[p->deal_index], choices[i]);
            handle_gravity(&p->state);
            ++p->deal_index;
            step_player(p);
            if (!p->chain) {
                receive_nuisance(p);
            }
            if (!valid) {
                clear_player(p);
                ++p->game_overs;
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
    }
    // Separate loop to mitigate iteration order issues.
    for (int i = 0; i < g->num_players; ++i) {
        if (nuisance_receivers[i]) {
            receive_nuisance(g->players + i);
        }
    }
}

practice_game* game_as_practice(game *g, int player_index) {
    // If the player is stuck chaining there is no practice state the reflects that.
    if (g->players[player_index].chain) {
        return NULL;
    }
    // Multiple opponents would require multiple incomming channels for practice.
    assert(g->num_players == 2);  // Limit to two for now.
    practice_game *pg = calloc(1, sizeof(practice_game));
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
        player opponent = g->players[i];
        if (opponent.chain) {
            int delay = 0;
            do {
                step_player(&opponent);
                ++delay;
            } while(opponent.chain);
            int nuisance = send_nuisance(&opponent, opponent.chain_score);
            pg->delay = delay;
            pg->incoming = nuisance;
        }
        return pg;
    }
    assert(0);
}

void* copy_practice(void *pg) {
    void *c = malloc(sizeof(practice_game));
    memcpy(c, pg, sizeof(practice_game));
    return c;
}

void append_practice_deal(practice_game *pg, content_t deal) {
    for (int i = 0; i < MAX_DEALS - 1; ++i) {
        pg->deals[i] = pg->deals[i + 1];
    }
    pg->deals[MAX_DEALS - 1] = deal;
}

double step_practice(void *_pg, content_t deal, content_t choice) {
    practice_game *pg = _pg;
    double score = 0;
    state *s = &pg->player.state;
    if (pg->delay == 0) {
        pg->player.pending_nuisance += pg->incoming;
        pg->incoming = 0;
    }
    // Assume deal is pg->deals[0] for now.
    int valid = apply_deal_and_choice(s, deal, choice);
    int chain_score = resolve(s, NULL);
    score += chain_score;
    pg->player.total_score += chain_score;
    int outgoing = send_nuisance(&pg->player, chain_score);
    receive_nuisance(&pg->player);
    handle_gravity(s);
    pg->incoming -= outgoing;
    if (pg->incoming < 0) {
        pg->incoming = 0;
        pg->delay = 0;
    }
    if (!valid) {
        clear_player(&pg->player);
        ++pg->player.game_overs;
        score = -DEATH_SCORE;
    }
    append_practice_deal(pg, rand_piece());
    --pg->delay;
    if (pg->delay < 0) {
        pg->delay = 0;
    }
    return score;
}
