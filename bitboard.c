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
    );
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
