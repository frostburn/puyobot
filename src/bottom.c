#include <string.h>

#include "puyobot/bottom.h"

char color_label(int color) {
    int c = '0' + color;
    if (c > '9') {
        c = c - '9' + 'a' - 1;
    }
    if (c > 'z') {
        c = c - 'z' + 'A' - 1;
    }
    return c;
}

void print_bottom_spam(puyos_t *floor, int num_links, int num_colors) {
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
                if (k < num_links) {
                    printf(" %c", color_label(k));
                } else {
                    printf(" @");
                }
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

void print_bottom(puyos_t *floor, int num_colors) {
    print_bottom_spam(floor, num_colors, num_colors);
}

int handle_bottom_gravity(puyos_t *floor, int num_colors) {
    puyos_t all;
    all = 0;
    for (int j = 0; j < num_colors; ++j) {
        all |= floor[j];
    }

    int iterations = 0;
    puyos_t temp;
    do {
        temp = all;
        puyos_t bellow, falling;
        bellow = (all >> V_SHIFT) | BOTTOM;
        all = 0;
        for (int i = 0; i < num_colors; ++i) {
            falling = floor[i] & ~bellow;
            floor[i] = (falling << V_SHIFT) | (floor[i] & bellow);
            all |= floor[i];
        }
        ++iterations;
    } while (temp != all);
    return iterations;
}

int clear_bottom_groups(puyos_t *floor, int num_colors, int *color_cleared) {
    int num_cleared = 0;
    for (int i = 0; i < num_colors; ++i) {
        puyos_t bottom = floor[i];
        for (int j = WIDTH * HEIGHT - 2; j >= 0; j -= 2) {
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
        int iterations = handle_bottom_gravity(floor, num_colors);
        if (iterations == 1 && chain > 0) {
            break;
        }
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

void print_conflicts(char* conflicts, int num_colors) {
    printf(" ");
    for (int i = 0; i < num_colors; ++i) {
        printf(" %c", color_label(i));
    }
    printf("\n");
    for (int i = 0; i < num_colors; ++i) {
        printf("%c", color_label(i));
        for (int j = 0; j < num_colors; ++j) {
            if (conflicts[i + j * num_colors]) {
                printf(" @");
            } else {
                printf("  ");
            }
        }
        printf("\n");
    }
}

int bottom_deal_and_choice(puyos_t *floor, content_t deal, content_t choice) {
    if (choice == CHOICE_PASS) {
        return 0;
    }
    content_t color1 = deal & COLOR1_MASK;
    content_t color2 = deal >> COLOR2_SHIFT;
    content_t orientation = choice & ~CHOICE_X_MASK;
    content_t color1_x = choice & CHOICE_X_MASK;
    content_t color2_x = color1_x;
    content_t color1_y = 0;
    content_t color2_y = 1;
    if (orientation == CHOICE_90) {
        color2_x++;
        color2_y--;
    } else if (orientation == CHOICE_180) {
        color1_y++;
        color2_y--;
    } else if (orientation == CHOICE_270) {
        color1_x++;
        color2_y--;
    }

    puyos_t puyo1 = 1ULL << (color1_x + V_SHIFT * color1_y);
    puyos_t puyo2 = 1ULL << (color2_x + V_SHIFT * color2_y);

    puyos_t piece = puyo1 | puyo2;
    for (int i = 0; i < NUM_DEAL_COLORS; ++i) {
        if (piece & floor[i]) {
            return 0;
        }
    }

    floor[color1] |= puyo1;
    floor[color2] |= puyo2;

    return 1;
}
