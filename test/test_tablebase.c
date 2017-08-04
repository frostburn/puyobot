#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "puyobot/bottom.h"
#include "puyobot/tablebase.h"

void test_table_position() {
    TablePosition *position = malloc(sizeof(TablePosition));
    position->deals[0] = make_piece(1, 2);
    position->deals[1] = make_piece(2, 3);
    position->deals[2] = make_piece(0, 0);
    position->num_deals = 3;
    position->floor[0] = 1 << 1;
    position->floor[1] = 1 << 3;
    print_bottom(position->floor, NUM_DEAL_COLORS);
    print_deals(position->deals, position->num_deals);
    canonize_table_position(position);
    print_bottom(position->floor, NUM_DEAL_COLORS);
    print_deals(position->deals, position->num_deals);
    content_t last_color = deal_color1(position->deals[2]);
    assert(deal_color1(position->deals[0]) != last_color);
    assert(deal_color2(position->deals[0]) != last_color);
    assert(deal_color1(position->deals[1]) != last_color);
    assert(deal_color2(position->deals[1]) != last_color);
}

int main() {
    test_table_position();
    printf("\n");
    content_t deals[3] = {make_piece(3, 1), make_piece(2, 0), make_piece(0, 2)};
    print_deals(deals, 3);
    canonize_deals(deals, 3);
    print_deals(deals, 3);
    assert(deal_color1(deals[0]) == 0);
    assert(deal_color1(deals[1]) == deal_color1(deals[2]));
}
