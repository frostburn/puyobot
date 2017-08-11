#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <omp.h>

#include "full-dict/full.h"

#include "puyobot/bottom.h"
#include "puyobot/tablebase.h"
#include "puyobot/util.h"

double clear_chance(TablePosition position, int remaining) {
    if (!remaining) {
        return 0;
    }
    // Probabilistic deals
    assert(!position.num_deals);

    if (!can_clear_with_any(position, remaining)) {
        return 0;
    }
    double likelihood = 0;
    position.num_deals = 1;
    for (int i = 0; i < NUM_DEAL_COLORS; ++i) {
        for (int j = i; j < NUM_DEAL_COLORS; ++j) {
            position.deals[0] = make_piece(i, j);
            double best = 0;
            for (int k = 0; k < NUM_CHOICES; ++k) {
                TablePosition child = table_position_apply_choice(position, CHOICES[k]);
                if (child.num_deals == TABLE_POSITION_INVALID) {
                    continue;
                }
                if (table_position_empty(child)) {
                    best = 1;
                    break;
                }
            }
            if (best < 1) {
                for (int k = 0; k < NUM_CHOICES; ++k) {
                    TablePosition child = table_position_apply_choice(position, CHOICES[k]);
                    if (child.num_deals == TABLE_POSITION_INVALID) {
                        continue;
                    }
                    double child_likelihood = clear_chance(child, remaining - 1);
                    if (child_likelihood > best) {
                        best = child_likelihood;
                        if (best == 1) {
                            break;
                        }
                    }
                }
            }
            likelihood += best;
            if (j != i) {
                likelihood += best;
            }
        }
    }
    return likelihood / (NUM_DEAL_COLORS * NUM_DEAL_COLORS);
}

double solve_root(FullDict *leaf_dict, double *leaf_values, TablePosition position) {
    if (!position.num_deals) {
        canonize_table_position(&position);
        return leaf_values[full_dict_index(leaf_dict, &position)];
    }
    // Deterministic deals
    for (int k = 0; k < NUM_CHOICES; ++k) {
        TablePosition child = table_position_apply_choice(position, CHOICES[k]);
        if (child.num_deals == TABLE_POSITION_INVALID) {
            continue;
        }
        if (table_position_empty(child)) {
            return 1;
        }
    }
    double best = 0;
    for (int k = 0; k < NUM_CHOICES; ++k) {
        TablePosition child = table_position_apply_choice(position, CHOICES[k]);
        if (child.num_deals == TABLE_POSITION_INVALID) {
            continue;
        }
        double child_likelihood = solve_root(leaf_dict, leaf_values, child);
        if (child_likelihood > best) {
            best = child_likelihood;
            if (best == 1) {
                return 1;
            }
        }
    }
    return best;
}

void collect_leaves(FullDict *dict, TablePosition position) {
    if (!position.num_deals) {
        canonize_table_position(&position);
        full_dict_append(dict, &position);
        return;
    }
    // Deterministic deals
    for (int k = 0; k < NUM_CHOICES; ++k) {
        TablePosition child = table_position_apply_choice(position, CHOICES[k]);
        if (child.num_deals == TABLE_POSITION_INVALID) {
            continue;
        }
        collect_leaves(dict, child);
    }
}

void collect_roots(FullDict *dict, TablePosition position, int remaining) {
    if (!remaining) {
        canonize_table_position(&position);
        full_dict_append(dict, &position);
    }
    for (int i = 0; i < NUM_CHOICES; ++i) {
        TablePosition child = table_position_apply_choice(position, CHOICES[i]);
        if (child.num_deals == TABLE_POSITION_INVALID) {
            continue;
        }
        collect_roots(dict, child, remaining - 1);
    }
}

void collect_from_deals(FullDict *dict, keys_t *deals_keys, size_t num_keys, int num_deals, int depth) {
    for (size_t j = 0; j < num_keys; ++j) {
        TablePosition position;
        position.num_deals = num_deals;
        deals_from_key(position.deals, position.num_deals, deals_keys[j]);
        memset(position.floor, 0, NUM_DEAL_COLORS * sizeof(puyos_t));
        printf("Collecting %zu / %zu\n", j, num_keys);
        collect_roots(dict, position, depth);
    }
    full_dict_finalize(dict);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Please give the file containing the deals\n");
        return 0;
    }
    if (argc  < 3) {
        printf("Please give the root depth of the search\n");
    }
    if (argc < 4) {
        printf("Please give the leaf depth of the serach\n");
    }
    FullDict *deals_dict = malloc(sizeof(FullDict));
    char *buffer = file_to_buffer(argv[1]);
    char *buffer_start = buffer;
    int num_deals = *((int*)buffer);
    buffer += sizeof(int);
    full_dict_associate(deals_dict, compare_keys, buffer);

    int root_depth = atoi(argv[2]);
    int leaf_depth = atoi(argv[3]);

    FullDict *root_dict = full_dict_new(sizeof(TablePosition), compare_table_position);
    collect_from_deals(root_dict, deals_dict->keys, deals_dict->num_keys, num_deals, root_depth);
    printf("%zu roots collected\n", root_dict->num_keys);
    TablePosition *roots = root_dict->keys;
    FullDict *leaf_dict = full_dict_new(sizeof(TablePosition), compare_table_position);
    for (size_t i = 0; i < root_dict->num_keys; ++i) {
        print_table_position(roots[i]);
        collect_leaves(leaf_dict, roots[i]);
    }
    full_dict_finalize(leaf_dict);
    printf("%zu leaves collected\n", leaf_dict->num_keys);
    TablePosition *leaves = leaf_dict->keys;
    double *leaf_chances = malloc(sizeof(double) * leaf_dict->num_keys);
    #pragma omp parallel for
    for (size_t i = 0; i < leaf_dict->num_keys; ++i) {
        if (i % 10 == 0) {
            printf("caching leaf %zu / %zu\n", i, leaf_dict->num_keys);
        }
        leaf_chances[i] = clear_chance(leaves[i], leaf_depth);
    }
    size_t num_chances = 0;
    double *chances = malloc(sizeof(double) * root_dict->num_keys);
    for (size_t i = 0; i < root_dict->num_keys; ++i) {
        chances[i] = solve_root(leaf_dict, leaf_chances, roots[i]);
        if (chances[i]) {
            print_table_position(roots[i]);
            printf("chance = %f\n", chances[i]);
            ++num_chances;
        }
    }
    printf("%zu out of %zu positions have a chance of clearing\n", num_chances, root_dict->num_keys);
    if (argc > 4) {
        printf("Saving the result...\n");
        FILE *f = fopen(argv[4], "w");
        full_dict_write(root_dict, f);
        fwrite(chances, sizeof(double), root_dict->num_keys, f);
        fclose(f);
    }

    full_dict_delete(root_dict);
    full_dict_delete(leaf_dict);
    free(chances);
    free(leaf_chances);
    free(deals_dict);
    free(buffer_start);
    return 0;
}
