#ifndef PUYOBOT_TEMPLATE_PATTERN_H_GUARD
#define PUYOBOT_TEMPLATE_PATTERN_H_GUARD

#include "puyobot/bitboard.h"

#define NUM_TETROMINOES (19)
#define NUM_TOP_TETROMINOES (30)
#define NUM_TRANSLATED_TETROMINOES (725)
#define NUM_TRANSLATED_TETROMINOES_2 (897)

extern const puyos_t TETROMINOES[];

extern const int TETROMINO_DIMS[][2];

extern puyos_t TOP_TETROMINOES[];
extern puyos_t TRANSLATED_TETROMINOES[];
extern puyos_t TRANSLATED_TETROMINOES_2[];

extern int TETROMINOES_INITIALIZED;

void init_tetrominoes();

#endif
