#define MAX_DELAY (20)
#define MAX_HARASSMENT (512)

#ifdef _OPENMP
    #include <omp.h>
#endif

typedef struct knockout_context
{
    practice_game pg;
    double scores[MAX_DELAY + 1][MAX_HARASSMENT + 1];
} knockout_context;

void print_knockout(knockout_context *context) {
    print_practice(&context->pg);
    int num_empty = 0;
    for (int j = 0; j < MAX_HARASSMENT; ++j) {
        int any = 0;
        for (int i = 0; i < MAX_DELAY; ++i) {
            if (!isnan(context->scores[i][MAX_HARASSMENT - 1 - j])) {
                any = 1;
            }
        }
        if (any) {
            break;
        }
        ++num_empty;
    }
    for (int j = 0; j < MAX_HARASSMENT - num_empty; ++j) {
        for (int i = 0; i < MAX_DELAY; ++i) {
            if (isnan(context->scores[i][j])) {
                printf("  #  ");
            } else if (context->scores[i][j] <= -1000) {
                printf("dead ");
            } else {
                printf("%4.0f ", context->scores[i][j] / 100);
            }
        }
        printf("\n");
    }
    if (num_empty) {
        printf("...\n");
    }
}

int can_clear_something(state *s, content_t deal) {
    state *c = malloc(sizeof(state));
    for (int i = 0; i < NUM_CHOICES; ++i) {
        memcpy(c, s, sizeof(state));
        if (apply_deal_and_choice(c, deal, CHOICES[i]) && resolve(c, NULL)) {
            free(c);
            return 1;
        }
    }
    free(c);
    return 0;
}

int max_dip(state *s, int extra) {
    puyos_t all[2];
    get_state_mask(s, all);
    int best_dip = 0;
    int previous = depth_2(all, 0);
    for (int i = 1; i < WIDTH; ++i) {
        int temp, dip;
        int current = depth_2(all, i);
        if (current <= previous) {
            temp = current - extra;
            if (temp < GHOST_Y) {
                temp = GHOST_Y;
            }
            dip = previous - temp;
        } else {
            temp = previous - extra;
            if (temp < GHOST_Y) {
                temp = GHOST_Y;
            }
            dip = current - temp;
        }
        if (dip > best_dip) {
            best_dip = dip;
        }
    }
    return best_dip;
}

int game_basically_over(practice_game *pg) {
    player p = pg->player;
    if (state_is_full(&p.state)) {
        return 1;
    }
    if (max_dip(&p.state, 2) > 5) {
        return 0;
    }
    if (can_clear_something(&p.state, pg->deals[0])) {
        return 0;
    }
    do {
        receive_nuisance(&p);
    } while(p.pending_nuisance >= MAX_NUISANCE_ROWS * WIDTH);
    if (state_is_full(&p.state)) {
        return 1;
    }
    return 0;
}

double eval_dead(void *pg) {
    if (game_basically_over(pg)) {
        return -DEATH_SCORE;
    }
    return 0;
}

double knockout_score(int incoming, int delay, knockout_context *context) {
    assert(incoming >= 0);
    assert(delay >= 0);
    if (incoming >= MAX_HARASSMENT) {
        incoming = MAX_HARASSMENT - 1;
    }
    if (delay >= MAX_DELAY) {
        delay = MAX_DELAY - 1;
    }

    double score;
    #ifdef _OPENMP
    #pragma omp critical(knockout_update)
    #endif
    {
        score = context->scores[delay][incoming];
    }
    if (!isnan(score)) {
        return score;
    }
    score = max_score(&context->pg.player.state, context->pg.deals, context->pg.num_deals, delay);
    // TODO: Take playing area shape and finite nuisance limits into account.
    if (context->pg.player.pending_nuisance + incoming - (score / TARGET_SCORE) >= WIDTH * LIFE_HEIGHT - 3) {
        score -= DEATH_SCORE;
    }
    if (delay > 3 || score < 0) {
        #ifdef _OPENMP
        #pragma omp critical(knockout_update)
        #endif
        {
            context->scores[delay][incoming] = score;
        }
        return score;
    }
    practice_game pg = context->pg;
    pg.delay += delay;
    pg.incoming += incoming;
    pg.num_deals = 4;
    tree_options options = {
        .copy = copy_practice,
        .step = step_practice,
        .eval = eval_dead,
        .depth = 0,
        .tree_factor = 1,
    };
    value_node *root = make_full_tree(pg.num_deals, options.depth);
    solve_tree(&pg, root, pg.deals, pg.num_deals, options);
    score = root->value;
    free_tree(root);
    #ifdef _OPENMP
    #pragma omp critical(knockout_update)
    #endif
    {
        context->scores[delay][incoming] = score;
    }
    return score;
}

double random_knockout_score(int iterations, int turns, int incoming, int delay, knockout_context *context) {
    assert(incoming >= 0);
    int double_checking = 0;
    if (delay <= 0) {
        delay = -delay;
        double_checking = 1;
    }
    if (incoming >= MAX_HARASSMENT) {
        incoming = MAX_HARASSMENT - 1;
    }
    if (delay >= MAX_DELAY) {
        delay = MAX_DELAY - 1;
    }

    double score;
    #ifdef _OPENMP
    #pragma omp critical(knockout_update)
    #endif
    {
        score = context->scores[delay][incoming];
    }
    if (!isnan(score)) {
        return score;
    }

    practice_game pg = context->pg;
    pg.delay += delay;
    pg.incoming += incoming;
    score = best_random_score(&context->pg, iterations, turns);
    // Death is a serious thing. Make sure.
    if (score < 0) {
        double check_score = best_random_score(&context->pg, iterations, turns);
        if (check_score > score) {
            score = check_score;
        }
    }
    #ifdef _OPENMP
    #pragma omp critical(knockout_update)
    #endif
    {
        context->scores[delay][incoming] = score;
    }
    if (score < 0 && !double_checking && incoming) {
        double check_score = random_knockout_score(iterations, turns, incoming - 1, -delay, context);
        if (check_score >= 0) {
            return 0;
        }
    }
    return score;
}

// Assumes that the practice game was started with time=0 and score=0
double eval_knockout_prototype(practice_game *pg, knockout_context *context) {
    int delay = pg->time;
    int score = pg->player.total_score + pg->player.leftover_score;  // No exactly precise but close enough.
    int outgoing = score / TARGET_SCORE;
    return -knockout_score(outgoing, delay, context);
}

double eval_random_knockout_prototype(practice_game *pg, int iterations, int turns, knockout_context *context) {
    int delay = pg->time;
    int score = pg->player.total_score + pg->player.leftover_score;  // No exactly precise but close enough.
    int outgoing = score / TARGET_SCORE;

    return -random_knockout_score(iterations, turns, outgoing, delay, context);
}

knockout_context* new_knockout(game *g, int opponent_index) {
    knockout_context *context = malloc(sizeof(knockout_context));
    for (int i = 0; i < MAX_DELAY; ++i) {
        for (int j = 0; j < MAX_HARASSMENT; ++j) {
            context->scores[i][j] = NAN;
        }
    }
    practice_game pg = {0};
    pg.player = g->players[opponent_index];
    int time = 0;
    while (pg.player.chain) {
        step_player(&pg.player);
        ++time;
    }
    pg.time = time;
    pg.delay = -time;
    int outgoing = (pg.player.chain_score + pg.player.leftover_score) / TARGET_SCORE;
    if (outgoing >= pg.player.pending_nuisance) {
        outgoing -= pg.player.pending_nuisance;
        pg.player.pending_nuisance = 0;
    } else {
        pg.player.pending_nuisance -= outgoing;
        outgoing = 0;
    }
    pg.incoming = -outgoing;
    pg.player.leftover_score = (pg.player.chain_score + pg.player.leftover_score) % TARGET_SCORE;
    pg.player.chain_score = 0;
    pg.player.total_score = 0;
    pg.num_deals = g->num_deals;
    for (int j = 0; j < g->num_deals; ++j) {
        pg.deals[j] = g->deals[j + g->players[opponent_index].deal_index];
    }
    for (int j = g->num_deals; j < MAX_DEALS; ++j) {
        pg.deals[j] = rand_piece();
    }
    context->pg = pg;
    return context;
}
