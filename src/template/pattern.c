#include "puyobot/template/pattern.h"

const puyos_t TETROMINOES[NUM_TETROMINOES] = {
    195,  // O
    387, 4290,  // Z
    198, 8385,  // S
    15, 266305,  // I
    71, 452, 8323, 12353,  // L
    135, 450, 4289, 8386,  // T
    263, 449, 4163, 12418,  // J
};

const int TETROMINO_DIMS[NUM_TETROMINOES][2] = {
    {2, 2},
    {3, 2}, {2, 3},
    {3, 2}, {2, 3},
    {4, 1}, {1, 4},
    {3, 2}, {3, 2}, {2, 3}, {2, 3},
    {3, 2}, {3, 2}, {2, 3}, {2, 3},
    {3, 2}, {3, 2}, {2, 3}, {2, 3},
};

puyos_t TOP_TETROMINOES[NUM_TOP_TETROMINOES];
puyos_t TRANSLATED_TETROMINOES[NUM_TRANSLATED_TETROMINOES];
puyos_t TRANSLATED_TETROMINOES_2[2 * NUM_TRANSLATED_TETROMINOES_2];

int TETROMINOES_INITIALIZED = 0;

void init_tetrominoes() {
    int n = 0;
    int n_top = 0;
    for (int i = 0; i < NUM_TETROMINOES; ++i) {
        for (int j = 0; j <= WIDTH - TETROMINO_DIMS[i][0]; ++j) {
            for (int k = 0; k <= HEIGHT - TETROMINO_DIMS[i][1]; ++k) {
                TRANSLATED_TETROMINOES[n++] = TETROMINOES[i] << (j + k * V_SHIFT);
            }
            if (i == 0 || i == 5 || i == 7 || i == 9 || i == 11 || i == 15 || i == 17) {
                TOP_TETROMINOES[n_top++] = TETROMINOES[i] << j;
            }
        }
    }
    n = 0;
    for (int i = 0; i < NUM_TETROMINOES; ++i) {
        for (int j = 0; j <= WIDTH - TETROMINO_DIMS[i][0]; ++j) {
            for (int k = 0; k <= LIFE_HEIGHT - TETROMINO_DIMS[i][1]; ++k) {
                TRANSLATED_TETROMINOES_2[2*n] = TETROMINOES[i];
                TRANSLATED_TETROMINOES_2[2*n + 1] = 0;
                translate_2(TRANSLATED_TETROMINOES_2 + 2*n, j, k + GHOST_Y);
                ++n;
            }
        }
    }
    TETROMINOES_INITIALIZED = 1;
}
