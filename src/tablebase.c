#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "puyobot/tablebase.h"
#include "puyobot/bottom.h"

int has_clear_potential(content_t *deals, int num_deals) {
    int counts[NUM_DEAL_COLORS] = {0};
    for (int i = 0; i < num_deals; ++i) {
        counts[deal_color1(deals[i])]++;
        counts[deal_color2(deals[i])]++;
        int enough = 1;
        for (int j = 0; j < NUM_DEAL_COLORS; ++j) {
            if (counts[j] && counts[j] < CLEAR_THRESHOLD) {
                enough = 0;
            }
        }
        if (enough) {
            return 1;
        }
    }
    return 0;
}

int search_for_clears(puyos_t *floor, content_t *deals, int num_deals) {
    if (!num_deals) {
        return 0;
    }
    puyos_t *child = malloc(NUM_DEAL_COLORS * sizeof(puyos_t));
    int result = 0;
    for (int i = 0; i < NUM_CHOICES; ++i) {
        memcpy(child, floor, NUM_DEAL_COLORS * sizeof(puyos_t));
        bottom_deal_and_choice(child, deals[0], CHOICES[i]);
        resolve_bottom(child, NUM_DEAL_COLORS, NULL);
        int any = 0;
        for (int j = 0; j < NUM_DEAL_COLORS; ++j) {
            any = (any || child[j]);
        }
        if (!any || search_for_clears(child, deals + 1, num_deals - 1)) {
            result = 1;
            break;
        }
    }
    free(child);
    return result;
}

keys_t deals_key(content_t *deals, int num_deals) {
    keys_t key = 0;
    for (int i = 0; i < num_deals; ++i) {
        key *= NUM_DEAL_COLORS;
        key += deal_color1(deals[i]);
        key *= NUM_DEAL_COLORS;
        key += deal_color2(deals[i]);
    }
    return key;
}

void deals_from_key(content_t *deals, int num_deals, keys_t key) {
    content_t color;
    for (int i = num_deals - 1; i >= 0; --i) {
        content_t color2 = key % NUM_DEAL_COLORS;
        key /= NUM_DEAL_COLORS;
        content_t color1 = key % NUM_DEAL_COLORS;
        key /= NUM_DEAL_COLORS;
        deals[i] = make_piece(color1, color2);
    }
}

// TODO: Lehmer codes or something
content_t COLOR_PERMUTATIONS[24 * NUM_DEAL_COLORS] = {
    0, 1, 2, 3,
    0, 1, 3, 2,
    0, 2, 1, 3,
    0, 2, 3, 1,
    0, 3, 1, 2,
    0, 3, 2, 1,
    1, 0, 2, 3,
    1, 0, 3, 2,
    1, 2, 0, 3,
    1, 2, 3, 0,
    1, 3, 0, 2,
    1, 3, 2, 0,
    2, 0, 1, 3,
    2, 0, 3, 1,
    2, 1, 0, 3,
    2, 1, 3, 0,
    2, 3, 0, 1,
    2, 3, 1, 0,
    3, 0, 1, 2,
    3, 0, 2, 1,
    3, 1, 0, 2,
    3, 1, 2, 0,
    3, 2, 0, 1,
    3, 2, 1, 0,
};

keys_t flip_deals(content_t *deals, int num_deals, int index) {
    if (index == num_deals) {
        return deals_key(deals, num_deals);
    }
    content_t deal = deals[index];
    content_t color1 = deal_color1(deal);
    content_t color2 = deal_color2(deal);

    keys_t smallest = flip_deals(deals, num_deals, index + 1);
    if (color1 != color2) {
        deals[index] = make_piece(color2, color1);
        keys_t flipped = flip_deals(deals, num_deals, index + 1);
        deals[index] = deal;
        if (flipped < smallest) {
            return flipped;
        }
    }
    return smallest;
}

void canonize_deals(content_t *deals, int num_deals) {
    assert(NUM_DEAL_COLORS == 4);
    assert(num_deals > 0);

    content_t *perm_deals = malloc(num_deals * sizeof(content_t));
    // Known symmetry reduction
    content_t zero_color = deal_color1(deals[0]);
    content_t one_color = deal_color2(deals[0]);

    keys_t smallest = ~0ULL;
    for (int perm = 0; perm < 24; perm++) {
        // Known symmetry reduction
        if (zero_color == one_color && COLOR_PERMUTATIONS[NUM_DEAL_COLORS * perm + zero_color] > 0) {
            continue;
        }
        if (COLOR_PERMUTATIONS[NUM_DEAL_COLORS * perm + zero_color] > 1) {
            continue;
        }
        if (COLOR_PERMUTATIONS[NUM_DEAL_COLORS * perm + one_color] > 1) {
            continue;
        }

        for (int i = 0; i < num_deals; ++i) {
            content_t deal = deals[i];
            content_t color1 = deal_color1(deal);
            content_t color2 = deal_color2(deal);
            perm_deals[i] = make_piece(
                COLOR_PERMUTATIONS[NUM_DEAL_COLORS * perm + color1],
                COLOR_PERMUTATIONS[NUM_DEAL_COLORS * perm + color2]
            );
        }
        keys_t key = flip_deals(perm_deals, num_deals, 0);
        if (key < smallest) {
            smallest = key;
        }
    }
    free(perm_deals);
    deals_from_key(deals, num_deals, smallest);
}
