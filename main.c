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

#include "jkiss.c"
#include "util.c"
#include "bitboard.c"
#include "scoring.c"
#include "state.c"

#include "tree.c"
#include "template.c"
#include "template_gen.c"
#include "demo.c"
#include "test.c"
#include "benchmark.c"

void init_all() {
    jkiss_init();
    init_tetrominoes();
}

int main() {
    init_all();

    int num_templates = 16;
    bottom_template **templates = malloc(num_templates * sizeof(bottom_template*));
    for (int i = 0; i < num_templates; ++i) {
        bottom_template *t = any_good_chain();
        prepare_bottom_template(t);
        print_bottom(t->floor, t->num_colors);
        templates[i] = t;
    }

    // bottom_match_result r_debug;
    float eval(state *s) {
        float best = -2;
        float good = 0;
        float bad = 0;
        int num_good = 0;
        for (int i = 0; i < num_templates; ++i) {
            bottom_match_result r = match_bottom(s, templates[i]);
            float score = bottom_match_score(templates[i], r);
            if (score > -1) {
                if (score > best) {
                    best = score;
                    // r_debug = r;
                }
                good += score;
                ++num_good;
            } else {
                bad += score;
            }
        }
        if (num_good) {
            return best + 0.05 * (good / num_good);
        } else {
            return (good + bad) / num_templates;
        }
    }

    content_t policy(state *s, content_t *deals, size_t num_deals) {
        value_node *root = solve_tree(s, deals, num_deals, 0, eval, 0);
        float value = root->value;
        choice_branch *choice = choose(root);
        content_t action = choice->content;
        float score = eval(s);
        // print_bottom_match_result(r_debug);
        fprintf(stderr, "score=%f -> %f\n", score, value);
        return action;
    }
    state *s = calloc(1, sizeof(state));
    policy_demo(s, 0, 1000, policy);
}
