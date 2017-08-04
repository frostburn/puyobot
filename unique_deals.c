#include <stdlib.h>
#include <stdio.h>

#include "full-dict/full.h"

#include "puyobot/tablebase.h"
#include "puyobot/util.h"

void print_results(FullDict *dict, int num_deals) {
    content_t deals[num_deals];
    keys_t *keys = dict->keys;
    for (size_t i = 0; i < dict->num_keys; ++i) {
        keys_t key = keys[i];
        deals_from_key(deals, num_deals, key);
        print_deals(deals, num_deals);
    }
    printf("Number of unique sequences with %d pieces of %d colors = %zu\n", num_deals, NUM_DEAL_COLORS, dict->num_keys);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Please input the number of deals\n");
        return 1;
    }
    int num_deals = atoi(argv[1]);
    FullDict *dict;

    if (num_deals <= 0) {
        if (argc < 3) {
            printf("Missing filename to load\n");
            return 1;
        }
        dict = malloc(sizeof(FullDict));
        char *buffer = file_to_buffer(argv[2]);
        char *buffer_start = buffer;
        num_deals = *((int*)buffer);
        buffer += sizeof(int);
        full_dict_associate(dict, compare_keys, buffer);
        print_results(dict, num_deals);
        free(dict);
        free(buffer_start);
        return 1;
    }

    content_t deals[num_deals];
    keys_t num_all = 1;
    for (int i = 0; i < num_deals; ++i) {
        num_all *= NUM_DEAL_COLORS;
        num_all *= NUM_DEAL_COLORS;
    }
    num_all /= NUM_DEAL_COLORS; // Known symmetry reduction

    puyos_t *floor = calloc(NUM_DEAL_COLORS, sizeof(puyos_t));

    dict = full_dict_new(sizeof(keys_t), compare_keys);
    for (keys_t i = 0; i < num_all; ++i) {
        if (i % 10000 == 0) {
            printf("%.1f %%\n", 100.0 * i / num_all);
        }
        if ((i + 1) % 1000000 == 0) {
            printf("Writing a backup just in case...\n");
            full_dict_finalize(dict);
            FILE *f = fopen(argv[2], "w");
            fwrite((void*) &num_deals, sizeof(int), 1, f);
            full_dict_write(dict, f);
            fclose(f);
        }
        deals_from_key(deals, num_deals, i);
        canonize_deals(deals, num_deals);
        keys_t key = deals_key(deals, num_deals);
        full_dict_append(dict, &key);
    }
    free(floor);
    full_dict_finalize(dict);

    print_results(dict, num_deals);

    if (argc > 2) {
        printf("Saving the result...\n");
        FILE *f = fopen(argv[2], "w");
        fwrite((void*) &num_deals, sizeof(int), 1, f);
        full_dict_write(dict, f);
        fclose(f);
    }

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
// Number of unique sequences with 8 pieces of 4 colors = 4183083
