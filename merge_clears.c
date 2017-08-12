#include <stdio.h>
#include <stdlib.h>

#include "full-dict/full.h"

#include "puyobot/tablebase.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("No files given\n");
        return 0;
    }
    double *values_a;
    double *values_b;
    FullDict *dict_a = read_clears(&values_a, argv[1]);
    for (int i = 2; i < argc; ++i) {
        FullDict *dict_b = read_clears(&values_b, argv[i]);
        printf("Merging dicts containing %zu and %zu entries\n", dict_a->num_keys, dict_b->num_keys);
        FullDict *dict_c = full_dict_merge(dict_a, dict_b);
        double *values_c = malloc(sizeof(double) * dict_c->num_keys);
        TablePosition *positions = dict_c->keys;
        for (size_t j = 0; j < dict_c->num_keys; ++j) {
            if (full_dict_contains(dict_a, positions + j)) {
                values_c[j] = values_a[full_dict_index(dict_a, positions + j)];
            } else {
                values_c[j] = values_b[full_dict_index(dict_b, positions + j)];
            }
        }
        free(values_a);
        free(values_b);
        full_dict_delete(dict_a);
        full_dict_delete(dict_b);
        values_a = values_c;
        dict_a = dict_c;
    }
    write_clears(dict_a, values_a, "out.dump");
    free(values_a);
    full_dict_delete(dict_a);
    return 0;
}
