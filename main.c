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

static char* COLOR_NAMES[NUM_COLORS] = {"red", "green", "yellow", "blue", "purple", "garbage"};

#include "jkiss.c"
#include "util.c"
#include "bitboard.c"
#include "scoring.c"
#include "state.c"

#include "deal.c"
#include "eval.c"
#include "tree.c"
#include "template.c"
#include "template_gen.c"
#include "multiplayer.c"
#include "demo.c"
#include "test.c"
#include "benchmark.c"
#include "info.c"

void init_all() {
    jkiss_init();
    init_tetrominoes();
}

int main(int argc, char *argv[]) {
    init_all();
    for (int i = 0; i < 100; ++i) {
        printf("\n");
    }
    printf("Frostbot v0.2-alpha\n\n");

    FILE *groups_out = fopen("groups.txt", "w");
    FILE *chains_out = fopen("chains.txt", "w");
    FILE *value_out = fopen("value.txt", "w");

    size_t iteration = 0;
    content_t policy(void *s, content_t *deals, int num_deals) {
        float factor = 0.0017;
        float pc = state_popcount(s);
        if (pc > 50) {
            factor = 0.03;
        }
        if (pc > 68) {
            factor = 0.1;
        }
        tree_options options = simple_tree_options(_eval_groups_chains, 1, factor);
        value_node *root = solve_tree(s, deals, num_deals, options);
        float value = root->value;
        choice_branch *choice = choose(root);
        content_t action = choice->content;
        free_tree(root);

        ++iteration;
        fprintf(value_out, "----Turn %zu----\n", iteration);
        fprintf(groups_out, "----Turn %zu----\n", iteration);
        fprintf(chains_out, "----Turn %zu----\n", iteration);

        fprintf(value_out, "Tree value=%f\n", value);
        eval_groups_info(s, groups_out);
        eval_chains_info(s, chains_out);
        fflush(value_out);
        fflush(groups_out);
        fflush(chains_out);

        return action;
    }

    state* s = calloc(1, sizeof(state));
    policy_demo(s, 1, 10000, policy);

    fclose(value_out);
    fclose(groups_out);
    fclose(chains_out);
    return 0;
}
