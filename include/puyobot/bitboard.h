#ifndef PUYOBOT_BITBOARD_H_GUARD
#define PUYOBOT_BITBOARD_H_GUARD

#include <stdio.h>

#include "puyobot/constants.h"

#define H_SHIFT (1)
#define V_SHIFT (WIDTH)
#define FULL (0xfffffffffffffffULL)
#define TOP (0x3fULL)
#define BOTTOM (0xfc0000000000000ULL)
#define LEFT_WALL (0x41041041041041ULL)
#define RIGHT_BLOCK (0xfbefbefbefbefbeULL)
#define RIGHT_WALL (0x820820820820820ULL)
#define LEFT_BLOCK (0x7df7df7df7df7dfULL)
#define LEFT_SIDE (0x1c71c71c71c71c7ULL)
#define RIGHT_SIDE (0xe38e38e38e38e38ULL)
#define TOP_TO_BOTTOM (V_SHIFT * (HEIGHT - 1))

#define DEATH_BLOCK (0x3ffffffffffULL)
#define GHOST_LINE (0xfc0000000000ULL)
#define LIFE_BLOCK (0xfff000000000000ULL)

#define MIRROR_DELTA_LEFT (LEFT_WALL | (LEFT_WALL << 3))
#define MIRROR_DELTA_CENTER ((LEFT_WALL << 1) | (LEFT_WALL << 4))

#define MAX_GROUPS (WIDTH * LIFE_HEIGHT / 2)  // Assuming even width and height

typedef unsigned long long int puyos_t;

void fprint_puyos(FILE *f, puyos_t puyos);

void print_puyos(puyos_t puyos);

void fprint_puyos_2(FILE *f, puyos_t *puyos);

void print_puyos_2(puyos_t *puyos);

int popcount(puyos_t puyos);

puyos_t left(puyos_t puyos);

puyos_t right(puyos_t puyos);

puyos_t up(puyos_t puyos);

puyos_t down(puyos_t puyos);

puyos_t cross(puyos_t puyos);

puyos_t blob(puyos_t puyos);

puyos_t beam_up(puyos_t puyos);

puyos_t beam_down(puyos_t puyos);

int popcount_2(puyos_t *puyos);

void left_2(puyos_t *puyos);

void right_2(puyos_t *puyos);

void up_2(puyos_t *puyos);

void down_2(puyos_t *puyos);

void cross_2(puyos_t *puyos);

void beam_up_2(puyos_t *puyos);

void beam_down_2(puyos_t *puyos);

// Positive direction only and no clips here. So be safe.
void translate_2(puyos_t *puyos, int x, int y);

void point_2(puyos_t *puyos, int x, int y);

puyos_t flood(register puyos_t source, register puyos_t target);

void flood_2(puyos_t *source, puyos_t *target);

int depth_2(puyos_t *puyos, int column);

// Number of (diagonally connected) objects minus the number of holes
int euler(puyos_t puyos);

int num_groups(puyos_t puyos);

int num_groups_2(puyos_t *puyos, puyos_t *groups);

int gap_size(puyos_t puyos);

int gap_size_2(puyos_t * puyos);

puyos_t mirror_puyos(puyos_t puyos);

#endif /* !PUYOBOT_BITBOARD_H_GUARD */
