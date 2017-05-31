#define TARGET_SCORE (70)
#define MAX_NUISANCE_ROWS (5)

typedef struct player
{
    state state;
    int deal_index;
    int chain;
    int chain_score;
    int total_score;
    int all_clear_bonus;
    int pending_nuisance;
    double leftover_nuisance;
} player;

typedef struct practice_game
{
    player player;
    int delay;
    int incoming;
    int game_overs;
    int num_deals;
    content_t deals[MAX_DEALS];
} practice_game;

typedef struct game
{
    int num_players;
    player *players;
    int num_deals;
    content_t *deals;
} game;

void print_player(player *p) {
    print_state(&p->state);
    int sending = floor(p->chain_score / (double) TARGET_SCORE + p->leftover_nuisance);
    printf(
        "score=%d chain=%d sending=%d receiving=%d\n",
        p->total_score, p->chain, sending, p->pending_nuisance
    );
}

void print_practice(practice_game *pg) {
    print_player(&pg->player);
    printf(
        "incoming=%d/%d game overs=%d\n",
        pg->incoming, pg->delay, pg->game_overs
    );
}

int handle_nuisance(player *p, int score) {
    // Send garbage
    double nuisance = score / (double) TARGET_SCORE + p->leftover_nuisance;
    int nuisance_sent = floor(nuisance);
    p->leftover_nuisance = nuisance - nuisance_sent;
    // Offset garbage
    if (p->pending_nuisance >= nuisance_sent) {
        p->pending_nuisance -= nuisance_sent;
        nuisance_sent = 0;
    } else {
        nuisance_sent -= p->pending_nuisance;
        p->pending_nuisance = 0;
    }
    // Receive garbage
    if (p->pending_nuisance) {
        int nuisance_reveiced = p->pending_nuisance;
        if (nuisance_reveiced > MAX_NUISANCE_ROWS * WIDTH) {
            nuisance_reveiced = MAX_NUISANCE_ROWS * WIDTH;
        }
        p->pending_nuisance -= nuisance_reveiced;
        int row = 0;
        while (nuisance_reveiced) {
            int nuisance_placed = nuisance_reveiced;
            if (nuisance_placed > WIDTH) {
                nuisance_placed = WIDTH;
            }
            nuisance_reveiced -= nuisance_placed;
            while(nuisance_placed) {
                puyos_t n = 1ULL << (jrand() % WIDTH + row * V_SHIFT);
                if (!(p->state.floors[0][GARBAGE] & n)) {
                    p->state.floors[0][GARBAGE] |= n;
                    --nuisance_placed;
                }
            }
            ++row;
        }
    }
    return nuisance_sent;
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
    for (int i = 0; i < g->num_players; ++i) {
        player *p = g->players + i;
        if (p->chain) {
            step_player(p);
        } else {
            int valid = apply_deal_and_choice(&p->state, g->deals[p->deal_index], choices[i]);
            assert(valid);  // TODO: Game over
            int nuisance = handle_nuisance(p, p->chain_score);
            handle_gravity(&p->state);
            if (p->all_clear_bonus) {
                nuisance += MAX_NUISANCE_ROWS * WIDTH;
                p->all_clear_bonus = 0;
            }
            p->chain_score = 0;
            for (int j = 0; j < g->num_players; ++j) {
                if (j != i) {
                    g->players[i].pending_nuisance += nuisance;
                }
            }
        }
    }
}

void* copy_practice(void *pg) {
    void *c = malloc(sizeof(practice_game));
    memcpy(c, pg, sizeof(practice_game));
    return c;
}

void append_practice_deal(practice_game *pg, content_t deal) {
    for (int i = 0; i < MAX_DEALS - 1; ++i) {
        pg->deals[i] = pg->deals[i + i];
    }
    pg->deals[0] = deal;
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
    int outgoing = handle_nuisance(&pg->player, chain_score);
    handle_gravity(s);
    pg->incoming -= outgoing;
    if (pg->incoming < 0) {
        pg->incoming = 0;
        pg->delay = 0;
    }
    if (!valid) {
        ++pg->game_overs;
        clear_state(&pg->player.state);
        score = -DEATH_SCORE;
    }
    append_practice_deal(pg, rand_piece());
    --pg->delay;
    if (pg->delay < 0) {
        pg->delay = 0;
    }
    return score;
}
