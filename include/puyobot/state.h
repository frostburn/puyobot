#ifndef PUYOBOT_STATE_H_GUARD
#define PUYOBOT_STATE_H_GUARD

#include "puyobot/constants.h"
#include "puyobot/bitboard.h"
#include "puyobot/deal.h"
#include "puyobot/solver/search.h"

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

extern const int COLOR_BONUS[];
extern const int GROUP_BONUS[];
extern const int CHAIN_POWERS[];

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

void blast_state(State *state, int num_colors, int num_shots);

int apply_deal_and_choice(State *state, content_t deal, content_t choice);

void just_apply_deal_and_choice(State *state, content_t deal, content_t choice);

void clear_deal_and_choice(State *state);

int step_state(void *s, content_t deal, content_t choice, double *score);

SearchOptions simple_search_options(eval_fun eval, int depth, double tree_factor);

#endif
