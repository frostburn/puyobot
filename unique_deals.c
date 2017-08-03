#include <stdlib.h>
#include <stdio.h>

#include "full-dict/full.h"

#include "puyobot/tablebase.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Please input the number of deals\n");
        return 1;
    }
    int num_deals = atoi(argv[1]);
    content_t deals[num_deals];
    keys_t num_all = 1;
    for (int i = 0; i < num_deals; ++i) {
        num_all *= NUM_DEAL_COLORS;
        num_all *= NUM_DEAL_COLORS;
    }
    num_all /= NUM_DEAL_COLORS; // Known symmetry reduction

    puyos_t *floor = calloc(NUM_DEAL_COLORS, sizeof(puyos_t));

    FullDict *dict = full_dict_new(sizeof(keys_t), compare_keys);
    for (keys_t i = 0; i < num_all; ++i) {
        if (i % 10000 == 0) {
            printf("%.1f %%\n", 100.0 * i / num_all);
        }
        deals_from_key(deals, num_deals, i);
        canonize_deals(deals, num_deals);
        keys_t key = deals_key(deals, num_deals);
        full_dict_append(dict, &key);
    }
    free(floor);
    full_dict_finalize(dict);

    keys_t *keys = dict->keys;
    for (size_t i = 0; i < dict->num_keys; ++i) {
        keys_t key = keys[i];
        deals_from_key(deals, num_deals, key);
        print_deals(deals, num_deals);
    }
    printf("Number of unique sequences with %d pieces of %d colors = %zu\n", num_deals, NUM_DEAL_COLORS, dict->num_keys);
    full_dict_delete(dict);

    return 0;
}

// Number of unique sequences with 1 pieces of 4 colors = 2
// Number of unique sequences with 2 pieces of 4 colors = 9
// Number of unique sequences with 3 pieces of 4 colors = 59
// Number of unique sequences with 4 pieces of 4 colors = 483
// Number of unique sequences with 5 pieces of 4 colors = 4427
// Number of unique sequences with 6 pieces of 4 colors = 42699
// Number of unique sequences with 7 pieces of 4 colors = 420779
