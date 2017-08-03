#ifndef PUYOBOT_TABLEBASE_H_GUARD
#define PUYOBOT_TABLEBASE_H_GUARD

#include "full-dict/types.h"

#include "puyobot/bitboard.h"
#include "puyobot/deal.h"

int has_clear_potential(content_t *deals, int num_deals);

int search_for_clears(puyos_t *floor, content_t *deals, int num_deals);

keys_t deals_key(content_t *deals, int num_deals);

void deals_from_key(content_t *deals, int num_deals, keys_t key);

void canonize_deals(content_t *deals, int num_deals);

#endif
