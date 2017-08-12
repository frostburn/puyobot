#include "jkiss/jkiss.h"

#include "puyobot/deal.h"

const content_t ROTATIONS[4] = {
    CHOICE_0, CHOICE_90, CHOICE_180, CHOICE_270,
};
const content_t CHOICES[NUM_CHOICES] = {
    0 | CHOICE_0, 1 | CHOICE_0, 2 | CHOICE_0, 3 | CHOICE_0, 4 | CHOICE_0, 5 | CHOICE_0,
    0 | CHOICE_90, 1 | CHOICE_90, 2 | CHOICE_90, 3 | CHOICE_90, 4 | CHOICE_90,
    0 | CHOICE_180, 1 | CHOICE_180, 2 | CHOICE_180, 3 | CHOICE_180, 4 | CHOICE_180, 5 | CHOICE_180,
    0 | CHOICE_270, 1 | CHOICE_270, 2 | CHOICE_270, 3 | CHOICE_270, 4 | CHOICE_270,
};


content_t make_piece(content_t color1, content_t color2) {
    return color1 | (color2 << COLOR2_SHIFT);
}

content_t rand_piece() {
    return make_piece(jrand() % NUM_DEAL_COLORS, jrand() % NUM_DEAL_COLORS);
}

content_t deal_color1(content_t deal) {
    return deal & COLOR1_MASK;
}

content_t deal_color2(content_t deal) {
    return deal >> COLOR2_SHIFT;
}

content_t make_choice(int *x, int *orientation) {
    while (*orientation < 0) {
        *orientation += 4;
    }
    *orientation = *orientation % 4;
    content_t rotation = ROTATIONS[*orientation];
    int max_x = 5;
    if (rotation == CHOICE_90 || rotation == CHOICE_270) {
        max_x = 4;
    }
    if (*x < 0) {
        *x = 0;
    } else if (*x > max_x) {
        *x = max_x;
    }
    return *x | rotation;
}

content_t rand_choice(choice_set_t choice_set) {
    if (!choice_set) {
        return CHOICE_PASS;
    }
    int i = jrand() % NUM_CHOICES;
    while (!(choice_set & (1 << i))) {
        i = (i + 1) % NUM_CHOICES;
    }
    return CHOICES[i];
}

void print_deals(content_t *deals, int num_deals) {
    for (int i = 0; i < num_deals; ++i) {
        content_t color1 = deal_color1(deals[i]);
        content_t color2 = deal_color2(deals[i]);
        printf("\x1b[3%d;1m ●", color1 + 1);
        printf("\x1b[3%d;1m ●", color2 + 1);
        printf("  ");
    }
    printf("\x1b[0m\n");
}

void print_choice(content_t choice) {
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
    for (int i = 0; i < WIDTH; ++i) {
        printf(" %c", 'A' + i);
    }
    printf("\n");
    for (int y = 0; y < 2; ++y) {
        for (int x = 0; x < WIDTH; ++x) {
            if (color1_x == x && color1_y == y) {
                printf(" a");
            } else if (color2_x == x && color2_y == y) {
                printf(" b");
            } else {
                printf("  ");
            }
        }
        printf("\n");
    }
}

void print_choice_set(choice_set_t choice_set) {
    int k = 0;
    for (int j = 0; j < 4; ++j) {
        printf("%d degrees:", j * 90);
        for (int i = 0; i < WIDTH - (j % 2); ++i) {
            if (choice_set & (1 << k)) {
                printf(" @");
            } else {
                printf(" .");
            }
            ++k;
        }
        printf("\n");
    }
}
