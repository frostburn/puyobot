void print_bottom(puyos_t *floor, int num_colors) {
    printf(" ");
    for (int i = 0; i < WIDTH; ++i) {
        printf(" %c", 'A' + i);
    }
    printf("\n");
    for (int i = 0; i < HEIGHT * WIDTH; ++i) {
        if (i % V_SHIFT == 0) {
            printf("%d", i / V_SHIFT);
        }
        puyos_t p = (1ULL << i);
        int any = 0;
        for (int k = 0; k < num_colors; ++k) {
            if (p & floor[k]) {
                printf("\x1b[3%d;1m", k % 6 + 1);
                int c = '0' + k;
                if (c > '9') {
                    c = c - '9' + 'a';
                }
                if (c > 'z') {
                    c = c - 'z' + 'A';
                }
                printf(" %c", c);
                any  = 1;
                break;
            }
        }
        printf("\x1b[0m");
        if (!any) {
            printf("  ");
        }
        if (i % V_SHIFT == V_SHIFT - 1) {
            printf("\n");
        }
    }
}

void handle_bottom_gravity(puyos_t *floor, int num_colors) {
    puyos_t all;
    all = 0;
    for (int j = 0; j < num_colors; ++j) {
        all |= floor[j];
    }

    puyos_t temp;
    do {
        temp = all;
        puyos_t bellow, falling;
        bellow = (all >> V_SHIFT) | BOTTOM;
        all = 0;
        for (int i = 0; i < num_colors; ++i) {
            falling = floor[i] & ~bellow;
            floor[i] = (falling << V_SHIFT) | (floor[i] & ~falling);
            all |= floor[i];
        }
    } while (temp != all);
}

int clear_bottom_groups(puyos_t *floor, int num_colors, int *color_cleared) {
    int num_cleared = 0;
    for (int i = 0; i < num_colors; ++i) {
        puyos_t bottom = floor[i];
        for (int j = 0; j < HEIGHT * WIDTH; j += 2) {
            puyos_t bottom_group = 3ULL << j;
            bottom_group = flood(bottom_group, bottom);
            bottom ^= bottom_group;
            int group_size = popcount(bottom_group);
            if (group_size >= CLEAR_THRESHOLD) {
                floor[i] ^= bottom_group;
                num_cleared += group_size;
                *color_cleared = i;
            }
            if (!bottom) {
                break;
            }
        }
    }
    return num_cleared;
}

int resolve_bottom(puyos_t *floor, int num_colors, int *color_order) {
    int chain = -1;
    while(1) {
        ++chain;
        handle_bottom_gravity(floor, num_colors);
        int color_cleared;
        if (!clear_bottom_groups(floor, num_colors, &color_cleared)) {
            break;
        }
        if (color_order) {
            color_order[chain] = color_cleared;
        }
    }
    return chain;
}

char* color_conflicts(puyos_t *floor, int num_colors) {
    char *conflicts = malloc(num_colors * num_colors * sizeof(char));
    puyos_t *temp = malloc(num_colors * sizeof(puyos_t));
    memcpy(temp, floor, num_colors * sizeof(puyos_t));
    int chain = resolve_bottom(temp, num_colors, NULL);
    for (int i = 0; i < num_colors; ++i) {
        for (int j = 0; j < num_colors; ++j) {
            memcpy(temp, floor, num_colors * sizeof(puyos_t));
            temp[i] |= temp[j];
            if (i != j) {
                temp[j] = 0;
            }
            int new_chain = resolve_bottom(temp, num_colors, NULL);
            conflicts[i + num_colors * j] = (new_chain != chain);
        }
    }
    return conflicts;
}
