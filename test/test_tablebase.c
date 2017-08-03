#include <assert.h>

#include "puyobot/tablebase.h"

int main() {
    content_t deals[3] = {make_piece(3, 1), make_piece(2, 4), make_piece(4, 2)};
    print_deals(deals, 3);
    canonize_deals(deals, 3);
    print_deals(deals, 3);
    assert(deal_color1(deals[0]) == 0);
    assert(deal_color1(deals[1]) == deal_color1(deals[2]));
}
