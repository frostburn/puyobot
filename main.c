#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

#define WIDTH (6)
#define HEIGHT (10)
#define H_SHIFT (1)
#define V_SHIFT (6)
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

#define GHOST_Y (7)
#define DEATH_BLOCK (0x3ffffffffffULL)
#define GHOST_LINE (0xfc0000000000ULL)
#define LIFE_BLOCK (0xfff000000000000ULL)
#define LIFE_HEIGHT (12)

#define TOTAL_SPACE (WIDTH * (LIFE_HEIGHT + 1))

#define NUM_FLOORS (2)
#define TOTAL_HEIGHT (20)
#define NUM_COLORS (6)
#define RED (0)
#define GREEN (1)
#define YELLOW (2)
#define BLUE (3)
#define PURPLE (4)
#define GARBAGE (5)
#define CLEAR_THRESHOLD (4)

#define MAX_GROUPS (WIDTH * LIFE_HEIGHT / 2)  // Assuming even width and height

#define MAX_DEALS (5)

#include "jkiss.c"
#include "util.c"
#include "bitboard.c"
#include "scoring.c"
#include "state.c"

#include "tree.c"
#include "template.c"
#include "template_gen.c"
#include "multiplayer.c"
#include "demo.c"
#include "test.c"
#include "benchmark.c"

void init_all() {
    jkiss_init();
    init_tetrominoes();
}

int main(int argc, char *argv[]) {
    init_all();
    int min_links = 0;
    int use_extensions = 1;
    int use_tailing = 1;
    if (argc > 1) {
        min_links = atoi(argv[1]);
    }
    if (argc > 2) {
        use_extensions = atoi(argv[2]);
    }
    if (argc > 3) {
        use_tailing = atoi(argv[3]);
    }
    show_chain(min_links, use_extensions, use_tailing);
}
