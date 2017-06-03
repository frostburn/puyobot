#define NUM_CHOICES (22)
#define CHOICE_0 (0)
#define CHOICE_90 (64)
#define CHOICE_180 (128)
#define CHOICE_270 (192)
#define CHOICE_X_MASK (0x3f)
#define COLOR1_MASK (15)
#define COLOR2_SHIFT (4)
#define DEATH_SCORE (1e44)

typedef unsigned char content_t;
typedef double (*step_fun)(void *s, content_t, content_t);
typedef void* (*copy_fun)(void *s);

static content_t ROTATIONS[4] = {
    CHOICE_0, CHOICE_90, CHOICE_180, CHOICE_270,
};
static content_t CHOICES[NUM_CHOICES] = {
    0 | CHOICE_0, 1 | CHOICE_0, 2 | CHOICE_0, 3 | CHOICE_0, 4 | CHOICE_0, 5 | CHOICE_0,
    0 | CHOICE_90, 1 | CHOICE_90, 2 | CHOICE_90, 3 | CHOICE_90, 4 | CHOICE_90,
    0 | CHOICE_180, 1 | CHOICE_180, 2 | CHOICE_180, 3 | CHOICE_180, 4 | CHOICE_180, 5 | CHOICE_180,
    0 | CHOICE_270, 1 | CHOICE_270, 2 | CHOICE_270, 3 | CHOICE_270, 4 | CHOICE_270,
};

content_t make_piece(content_t color1, content_t color2) {
    return color1 | (color2 << COLOR2_SHIFT);
}

content_t rand_piece() {
    return make_piece(jrand() % (NUM_COLORS - 1), jrand() % (NUM_COLORS - 1));
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

int apply_deal_and_choice(state *s, content_t deal, content_t choice) {
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
    puyos_t all = 0;
    for (int i = 0; i < NUM_COLORS; ++i) {
        all |= s->floors[0][i];
    }
    puyos_t puyo1 = 1ULL << (color1_x + V_SHIFT * color1_y);
    s->floors[0][color1] |= puyo1;
    puyos_t puyo2 = 1ULL << (color2_x + V_SHIFT * color2_y);
    s->floors[0][color2] |= puyo2;

    if (
        ((1ULL << (color1_x + V_SHIFT * GHOST_Y)) & all) &&
        ((1ULL << (color2_x + V_SHIFT * GHOST_Y)) & all)
    ) {
        return 0;
    }
    return 1;
}

void clear_deal_and_choice(state *s) {
    for (int i = 0; i < NUM_COLORS; ++i) {
        s->floors[0][i] &= ~(TOP | (TOP << V_SHIFT));
    }
}

void* state_copy(void *s) {
    void *c = copy_state(s);
    return c;
}

double state_step(void *s, content_t deal, content_t choice) {
    int valid = apply_deal_and_choice(s, deal, choice);
    if (!valid) {
        clear_state(s);
        return -DEATH_SCORE;
    }
    return resolve(s, NULL);
}
