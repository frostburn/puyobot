typedef unsigned long long int puyos_t;

void print_puyos(puyos_t puyos) {
    printf(" ");
    for (int i = 0; i < WIDTH; ++i) {
        printf(" %c", 'A' + i);
    }
    printf("\n");
    for (int i = 0; i < 64; ++i) {
        if (i % V_SHIFT == 0) {
            int j = i / V_SHIFT;
            if (j < 10) {
                printf("%d", j);
            } else {
                printf("%c", 'a' + j - 10);
            }
        }
        if ((1ULL << i) & puyos) {
            printf(" @");
        } else {
            printf("  ");
        }
        if (i % V_SHIFT == V_SHIFT - 1){
            printf("\n");
        }
    }
    printf("\n");
}

void print_puyos_2(puyos_t *puyos) {
    printf(" ");
    for (int i = 0; i < WIDTH; ++i) {
        printf(" %c", 'A' + i);
    }
    printf("\n");
    for (int k = 0; k < 2; ++k) {
        for (int i = 0; i < WIDTH * HEIGHT; ++i) {
            if (i % V_SHIFT == 0) {
                int j = i / V_SHIFT + k * HEIGHT;
                if (j < 10) {
                    printf("%d", j);
                } else {
                    printf("%c", 'a' + j - 10);
                }
            }
            if ((1ULL << i) & puyos[k]) {
                printf(" @");
            } else {
                printf("  ");
            }
            if (i % V_SHIFT == V_SHIFT - 1){
                printf("\n");
            }
        }
    }
}

int popcount(puyos_t puyos) {
    return __builtin_popcountll(puyos);
}

puyos_t cross(puyos_t puyos) {
    return (
        puyos |
        ((puyos & RIGHT_BLOCK) >> H_SHIFT) |
        ((puyos << H_SHIFT) & RIGHT_BLOCK) |
        (puyos << V_SHIFT) |
        (puyos >> V_SHIFT)
    ) & FULL;
}

puyos_t beam_up(puyos_t puyos) {
    puyos |= puyos >> V_SHIFT;
    puyos |= puyos >> (2 * V_SHIFT);
    puyos |= puyos >> (4 * V_SHIFT);
    puyos |= puyos >> (8 * V_SHIFT);
    return puyos;
}

puyos_t beam_down(puyos_t puyos) {
    puyos |= puyos << V_SHIFT;
    puyos |= puyos << (2 * V_SHIFT);
    puyos |= puyos << (4 * V_SHIFT);
    puyos |= puyos << (8 * V_SHIFT);
    return puyos & FULL;
}

void beam_up_2(puyos_t *puyos) {
    puyos[1] |= puyos[1] >> V_SHIFT;
    puyos[1] |= puyos[1] >> (2 * V_SHIFT);
    puyos[1] |= puyos[1] >> (4 * V_SHIFT);
    puyos[1] |= puyos[1] >> (8 * V_SHIFT);
    puyos[0] |= (puyos[1] & TOP) << TOP_TO_BOTTOM;
    puyos[0] |= puyos[0] >> V_SHIFT;
    puyos[0] |= puyos[0] >> (2 * V_SHIFT);
    puyos[0] |= puyos[0] >> (4 * V_SHIFT);
    puyos[0] |= puyos[0] >> (8 * V_SHIFT);
}

void beam_down_2(puyos_t *puyos) {
    puyos[0] |= puyos[0] << V_SHIFT;
    puyos[0] |= puyos[0] << (2 * V_SHIFT);
    puyos[0] |= puyos[0] << (4 * V_SHIFT);
    puyos[0] |= puyos[0] << (8 * V_SHIFT);
    puyos[1] |= (puyos[0] & BOTTOM) >> TOP_TO_BOTTOM;
    puyos[1] |= puyos[1] << V_SHIFT;
    puyos[1] |= puyos[1] << (2 * V_SHIFT);
    puyos[1] |= puyos[1] << (4 * V_SHIFT);
    puyos[1] |= puyos[1] << (8 * V_SHIFT);
}

// Positive direction only and no clips here. So be safe.
void translate_2(puyos_t *puyos, int x, int y) {
    if (y < HEIGHT) {
        puyos[0] <<= x;
        puyos[1] <<= x;
        puyos[1] <<= y * V_SHIFT;
        puyos[1] |= puyos[0] >> (V_SHIFT * (HEIGHT - y));
        puyos[0] <<= y * V_SHIFT;
    } else {
        puyos[1] = puyos[0] << (x + V_SHIFT * (y - HEIGHT));
        puyos[0] = 0;
    }
    puyos[0] &= FULL;
    puyos[1] &= FULL;
}

puyos_t flood(register puyos_t source, register puyos_t target) {
    source &= target;
    if (!source){
        return source;
    }
    register puyos_t temp;
    do {
        temp = source;
        source |= (
            ((source & RIGHT_BLOCK) >> H_SHIFT) |
            ((source << H_SHIFT) & RIGHT_BLOCK) |
            (source << V_SHIFT) |
            (source >> V_SHIFT)
        ) & target;
    } while (temp != source);
    return source;
}

// Number of (diagonally connected) objects minus the number of holes
int euler(puyos_t puyos) {
    int pixels = popcount(puyos);

    int edges = 0;
    edges += popcount(puyos & LEFT_WALL);
    edges += popcount(puyos & TOP);
    edges += popcount(puyos | ((puyos & RIGHT_BLOCK) >> H_SHIFT));
    edges += popcount(puyos | (puyos >> V_SHIFT));

    int vertices = 0;
    vertices += puyos & 1;
    vertices += popcount((puyos | (puyos >> V_SHIFT)) & LEFT_WALL);
    vertices += popcount((puyos | ((puyos & RIGHT_BLOCK) >> H_SHIFT)) & TOP);
    vertices += popcount(
        puyos |
        (puyos >> V_SHIFT) |
        ((puyos & RIGHT_BLOCK) >> H_SHIFT) |
        ((puyos & RIGHT_BLOCK) >> (V_SHIFT + H_SHIFT))
    );

    // This is always broken so leaving the debug print here.
    // printf("e=%d, v=%d, p=%d\n", edges, vertices, pixels);

    return pixels - edges + vertices;
}

// Reference for bit parallel "raindrop" gravity
puyos_t drop_once(puyos_t puyos) {
    puyos_t bellow = (puyos >> V_SHIFT) | BOTTOM;
    puyos_t falling = puyos & ~bellow;
    return (falling << V_SHIFT) | (puyos & ~falling);
}

int num_groups(puyos_t puyos) {
    int num = 0;
    for (int j = 0; j < HEIGHT * WIDTH; j += 2) {
        puyos_t group = 3ULL << j;
        group = flood(group, puyos);
        if (group) {
            ++num;
            puyos ^= group;
        }
        if (!puyos) {
            break;
        }
    }
    return num;
}

/* Arrange the N elements of ARRAY in random order.
   Only effective if N is much smaller than RAND_MAX;
   if this may not be the case, use a better random
   number generator. */
void shuffle(puyos_t *array, size_t n) {
    if (n > 1) {
        for (size_t i = 0; i < n - 1; ++i) {
          size_t j = i + jrand() % (n - i);
          puyos_t t = array[j];
          array[j] = array[i];
          array[i] = t;
        }
    }
}

void shuffle_2(puyos_t *array, size_t n) {
    if (n > 1) {
        for (size_t i = 0; i < n - 1; ++i) {
          size_t j = i + jrand() % (n - i);
          puyos_t t0 = array[2*j];
          puyos_t t1 = array[2*j + 1];
          array[2*j] = array[2*i];
          array[2*i] = t0;
          array[2*j + 1] = array[2*i + 1];
          array[2*i + 1] = t1;
        }
    }
}


int has_gap(puyos_t puyos) {
    puyos = beam_up(puyos);
    int run = 0;
    int gap = 0;
    for (int i = 0; i < WIDTH; ++i) {
        puyos_t probe = 1ULL << i;
        if (probe & puyos) {
            run = 1;
            if (gap) {
                return 1;
            }
        } else if (run) {
            gap = 1;
        }
    }
    return 0;
}
