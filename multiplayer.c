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

int handle_nuisance(player *p, int score) {
    // Send garbage
    double nuisance = score / (double) TARGET_SCORE + p->leftover_nuisance;
    int nuisance_sent = floor(nuisance);
    p->leftover_nuisance = nuisance - nuisance_sent;
    // Offset garbage
    p->pending_nuisance -= nuisance_sent;
    if (p->pending_nuisance < 0) {
        p->pending_nuisance = 0;
    }
    nuisance_sent -= p->pending_nuisance;
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
