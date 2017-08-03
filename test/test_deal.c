#include <assert.h>

#include "puyobot/constants.h"
#include "puyobot/deal.h"

int main() {
    content_t deal = rand_piece();
    assert(deal_color1(deal) < NUM_DEAL_COLORS);
    int x = 10;
    int orientation = 5;
    make_choice(&x, &orientation);
    assert(x < WIDTH);
    assert(orientation == 1);

    return 0;
}
