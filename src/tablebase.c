#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "puyobot/tablebase.h"
#include "puyobot/bottom.h"

void print_table_position(TablePosition position) {
    print_bottom(position.floor, NUM_DEAL_COLORS);
    assert(position.num_deals <= MAX_TABLE_DEALS);
    if (position.num_deals > 0) {
        print_deals(position.deals, position.num_deals);
    } else if (position.num_deals) {
        printf("(terminal)\n");
    } else {
        printf("(invalid)\n");
    }
}

int has_clear_potential(TablePosition position) {
    int counts[NUM_DEAL_COLORS] = {0};
    for (int i = 0; i < NUM_DEAL_COLORS; ++i) {
        counts[i] += popcount(position.floor[i]);
    }
    for (int i = 0; i < position.num_deals; ++i) {
        counts[deal_color1(position.deals[i])]++;
        counts[deal_color2(position.deals[i])]++;
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

TablePosition table_position_apply_choice(TablePosition position, content_t choice) {
    TablePosition child = position;
    if (position.num_deals <= 0) {
        child.num_deals = TABLE_POSITION_INVALID;
        return child;
    }
    if (bottom_deal_and_choice(child.floor, position.deals[0], choice)) {
        resolve_bottom(child.floor, NUM_DEAL_COLORS, NULL);
        for (int j = 1; j < child.num_deals; ++j) {
            child.deals[j - 1] = position.deals[j];
        }
        child.num_deals = position.num_deals - 1;
    } else {
        child.num_deals = TABLE_POSITION_INVALID;
    }
    return child;
}

int can_clear(TablePosition position) {
    int any = 0;
    for (int j = 0; j < NUM_DEAL_COLORS; ++j) {
        if (position.floor[j]) {
            any = 1;
            break;
        }
    }
    if (!any) {
        return 1;
    }
    if (!position.num_deals) {
        return 0;
    }
    for (int i = 0; i < NUM_CHOICES; ++i) {
        TablePosition child = table_position_apply_choice(position, CHOICES[i]);
        if (child.num_deals != TABLE_POSITION_INVALID && can_clear(child)) {
            return 1;
        }
    }
    return 0;
}

keys_t deals_key(const content_t *deals, int num_deals) {
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

unsigned int canonize_deals(content_t *deals, int num_deals) {
    assert(NUM_DEAL_COLORS == 4);
    assert(num_deals > 0);

    content_t *perm_deals = malloc(num_deals * sizeof(content_t));
    // Known symmetry reduction
    content_t zero_color = deal_color1(deals[0]);
    content_t one_color = deal_color2(deals[0]);

    unsigned int valid_permutations = 0;
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
            valid_permutations = 1 << perm;
            smallest = key;
        } else if (key == smallest) {
            valid_permutations |= 1 << perm;
        }
    }
    free(perm_deals);
    deals_from_key(deals, num_deals, smallest);
    return valid_permutations;
}

int compare_bottom(const void *a, const void *b) {
    const puyos_t *x = a;
    const puyos_t *y = b;
    for (int i = 0; i < NUM_DEAL_COLORS; ++i) {
        if (x[i] < y[i]) {
            return -1;
        }
        if (x[i] > y[i]) {
            return 1;
        }
    }
    return 0;
}

int compare_table_position(const void *a, const void *b) {
    const TablePosition *x = a;
    const TablePosition *y = b;
    if (x->num_deals > y->num_deals) {
        return -1;
    }
    if (x->num_deals < y->num_deals) {
        return 1;
    }
    keys_t key_x = deals_key(x->deals, x->num_deals);
    keys_t key_y = deals_key(y->deals, y->num_deals);
    if (key_x < key_y) {
        return -1;
    }
    if (key_x > key_y) {
        return 1;
    }
    return compare_bottom(x->floor, y->floor);
}

unsigned int canonize_table_position(TablePosition *position) {
    unsigned int color_permutations = canonize_deals(position->deals, position->num_deals);
    puyos_t temp[NUM_DEAL_COLORS];
    puyos_t smallest[NUM_DEAL_COLORS];
    int first = 1;
    int cmp;
    unsigned int valid_permutations = 0;
    for (int perm = 0; perm < 24; ++perm) {
        if (!(color_permutations & (1 << perm))) {
            continue;
        }
        for (int i = 0; i < NUM_DEAL_COLORS; ++i) {
            temp[COLOR_PERMUTATIONS[NUM_DEAL_COLORS * perm + i]] = position->floor[i];
        }
        cmp = compare_bottom(temp, smallest);
        if (cmp < 0 || first) {
            first = 0;
            memcpy(smallest, temp, NUM_DEAL_COLORS * sizeof(puyos_t));
            valid_permutations = 1 << perm;
        } else if (cmp == 0) {
            valid_permutations |= 1 << perm;
        }
        mirror_bottom(temp, NUM_DEAL_COLORS);
        cmp = compare_bottom(temp, smallest);
        if (cmp < 0) {
            memcpy(smallest, temp, NUM_DEAL_COLORS * sizeof(puyos_t));
            valid_permutations = 1 << perm;
        } else if (cmp == 0) {
            valid_permutations |= 1 << perm;
        }
    }
    memcpy(position->floor, smallest, NUM_DEAL_COLORS * sizeof(puyos_t));
    return valid_permutations;
}
