#ifndef PUYOBOT_TABLEBASE_H_GUARD
#define PUYOBOT_TABLEBASE_H_GUARD

#include "full-dict/types.h"

#include "puyobot/bitboard.h"
#include "puyobot/deal.h"

#define MAX_TABLE_DEALS (16)

typedef struct TablePosition {
    int num_deals;
    content_t deals[MAX_TABLE_DEALS];
    puyos_t floor[NUM_DEAL_COLORS];
} TablePosition;

int has_clear_potential(TablePosition position);

int search_for_clears(puyos_t *floor, content_t *deals, int num_deals);

keys_t deals_key(const content_t *deals, int num_deals);

void deals_from_key(content_t *deals, int num_deals, keys_t key);

unsigned int canonize_deals(content_t *deals, int num_deals);

int compare_bottom(const void *a, const void *b);

int compare_table_position(const void *a, const void *b);

unsigned int canonize_table_position(TablePosition *position);

#endif
