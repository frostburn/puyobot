#ifndef PUYOBOT_MULTIPLAYER_H_GUARD
#define PUYOBOT_MULTIPLAYER_H_GUARD

#include "puyobot/state.h"

#define MAX_DEALS (5)
#define TARGET_SCORE (70)
#define DROP_SCORE (10)
#define MAX_NUISANCE_ROWS (5)
#define MAX_PLAYERS (16)

typedef struct Player
{
    State state;
    int deal_index;
    int chain;
    int chain_score;
    int total_score;
    int chain_all_clear_bonus;
    int all_clear_bonus;
    int game_overs;
    int pending_nuisance;
    int leftover_score;
    int nuisance_x;
} Player;

typedef struct PracticeGame
{
    Player player;
    int time;
    int delay;
    int incoming;
    int num_deals;
    content_t deals[MAX_DEALS];
} PracticeGame;

typedef struct Game
{
    Player *players;
    content_t *deals;
    int time;
    int num_players;
    int num_deals;
    int total_num_deals;
} Game;

void print_player(Player *p);

void print_practice(PracticeGame *pg);

Game* new_game(int num_players, int num_deals);

void free_game(Game *g);

void clear_player(Player *p);

int send_nuisance(Player *p);

void receive_nuisance(Player *p);

int step_player(Player *p);

void step_game(Game *g, content_t *choices);

PracticeGame* game_as_practice(Game *g, int player_index);

void* copy_practice(void *pg);

void append_practice_deal(PracticeGame *pg, content_t deal);

int step_practice(void *_pg, content_t deal, content_t choice, double *score);

#endif
