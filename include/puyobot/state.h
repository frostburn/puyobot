#ifndef PUYOBOT_STATE_H_GUARD
#define PUYOBOT_STATE_H_GUARD

#include "puyobot/constants.h"
#include "puyobot/bitboard.h"
#include "puyobot/deal.h"

#define NUM_COLORS (6)
#define RED (0)
#define GREEN (1)
#define YELLOW (2)
#define BLUE (3)
#define PURPLE (4)
#define GARBAGE (5)

// Classic scoring
#define NUM_GROUP_BONUS (8)
#define NUM_CHAIN_POWERS (24)
#define MAX_CLEAR_BONUS (999)

static int COLOR_BONUS[NUM_COLORS] = {0, 0, 3, 6, 12, 24};
static int GROUP_BONUS[NUM_GROUP_BONUS] = {0, 2, 3, 4, 5, 6, 7, 10};
static int CHAIN_POWERS[NUM_CHAIN_POWERS] = {
    0, 8, 16, 32, 64, 96, 128, 160, 192, 224, 256, 288,
    320, 352, 384, 416, 448, 480, 512, 544, 576, 608, 640, 672
};

typedef struct State
{
    puyos_t floors[NUM_FLOORS][NUM_COLORS];
} State;

State* copy_state(State *state);

void clear_state(State *state);

int state_is_clear(State *state);

void shift_down(State *state);

void print_state(State *state);

void repr_state(State *state);

int state_euler(State *state);

int state_popcount(State *state);

int clear_groups(State *state, int chain_number);

int handle_gravity(State *state);

void kill_puyos(State *state);

int resolve(State *state, int *chain_out);

void get_state_mask(State *state, puyos_t *out);

int state_is_full(State *state);

void assert_sanity(State *state);

void blast_state(State *state, int num_shots);

#endif
