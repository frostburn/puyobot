#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "full-dict/full.h"

#include "puyobot/bottom.h"
#include "puyobot/tablebase.h"
#include "puyobot/util.h"

int append_children(FullDict *dict, TablePosition position, int clears_only) {
    if (!position.num_deals) {
        return 0;
    }
    if (clears_only && !has_clear_potential(position)) {
        return 0;
    }
    canonize_table_position(&position);
    if (dict->num_sorted + 2200 < dict->num_keys) {
        full_dict_finalize(dict);
    }
    if (full_dict_contains(dict, &position)) {
        return 0;
    }
    if (clears_only && !can_clear(position)) {
        return 0;
    }
    full_dict_append(dict, &position);
    for (int i = 0; i < NUM_CHOICES; ++i) {
        TablePosition child = table_position_apply_choice(position, CHOICES[i]);
        if (child.num_deals != TABLE_POSITION_INVALID) {
            append_children(dict, child, clears_only);
        }
    }
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Please enter a file containing the deals\n");
        return 0;
    }
    int clears_only = 0;
    if (argc > 3) {
        printf("Using only deals with clear potential\n");
        clears_only = 1;
    }
    FullDict *deals_dict = malloc(sizeof(FullDict));
    char *buffer = file_to_buffer(argv[1]);
    char *buffer_start = buffer;
    int num_deals = *((int*)buffer);
    buffer += sizeof(int);
    full_dict_associate(deals_dict, compare_keys, buffer);
    keys_t *unique_deals = deals_dict->keys;

    FullDict *dict = full_dict_new(sizeof(TablePosition), compare_table_position);
    for (size_t j = 0; j < deals_dict->num_keys; ++j) {
        if (argc > 2 && (j + 1) % 250 == 0) {
            full_dict_finalize(dict);
            printf("Saving a backup...\n");
            FILE *f = fopen(argv[2], "w");
            full_dict_write(dict, f);
            fclose(f);
        }
        printf("Handling deals %zu of %zu...", j, deals_dict->num_keys);
        TablePosition position;
        position.num_deals = num_deals;
        deals_from_key(position.deals, position.num_deals, unique_deals[j]);
        memset(position.floor, 0, NUM_DEAL_COLORS * sizeof(puyos_t));
        if (clears_only && !has_clear_potential(position)) {
            printf("skipped\n");
            continue;
        } else {
            printf("\n");
        }
        append_children(dict, position, clears_only);
    }
    full_dict_finalize(dict);
    #ifdef OPENING_DEBUG
        TablePosition *keys = dict->keys;
        for (size_t i = 0; i < dict->num_keys; ++i) {
            print_table_position(keys[i]);
        }
    #endif
    printf("Number of unique positions = %zu\n", dict->num_keys);
    if (argc > 2) {
        printf("Saving the result...\n");
        FILE *f = fopen(argv[2], "w");
        full_dict_write(dict, f);
        fclose(f);
    }
    full_dict_delete(dict);
    free(deals_dict);
    free(buffer_start);
    return 0;
}

// 2: Number of unique positions = 73
// 3: Number of unique positions = 4278
// 4: Number of unique positions = 235089
// 5: Number of unique positions = 10944303

// Clears
// 5: Number of unique positions = 20683
// 6: Number of unique positions = 440669
