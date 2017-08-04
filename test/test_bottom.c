#include <assert.h>

#include "jkiss/jkiss.h"

#include "puyobot/bottom.h"

void playout() {
    puyos_t floor[NUM_DEAL_COLORS] = {0};
    for (int i = 0; i < 30; ++i) {
        bottom_deal_and_choice(floor, rand_piece(), rand_choice(0, WIDTH));
        resolve_bottom(floor, NUM_DEAL_COLORS, NULL);
        print_bottom(floor, NUM_DEAL_COLORS);
    }
}

int main() {
    jkiss_init();

    puyos_t floor[] = {1, 2, 7, 1 << V_SHIFT};
    print_bottom(floor, 4);
    resolve_bottom(floor, 4, NULL);
    print_bottom(floor, 4);
    assert(floor[0] == 1ULL << (V_SHIFT * (HEIGHT - 2)));
    mirror_bottom(floor, 4);
    print_bottom(floor, 4);
    assert(floor[0] == 1ULL << (WIDTH - 1 + V_SHIFT * (HEIGHT - 2)));

    playout();
}
