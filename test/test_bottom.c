#include <assert.h>

#include "puyobot/bottom.h"

int main() {
    puyos_t floors[] = {1, 2, 7, 1 << V_SHIFT};
    print_bottom(floors, 4);
    resolve_bottom(floors, 4, NULL);
    print_bottom(floors, 4);
    assert(floors[0] == 1ULL << (V_SHIFT * (HEIGHT - 2)));
}
