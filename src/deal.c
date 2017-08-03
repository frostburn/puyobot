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

content_t rand_choice(content_t min_x, content_t max_x) {
    content_t rotation = ROTATIONS[jrand() % 4];
    if (max_x == 5 && (rotation == CHOICE_90 || rotation == CHOICE_270)) {
        max_x = 4;
    }
    return (min_x + (jrand() % (1 + max_x - min_x))) | rotation;
}
