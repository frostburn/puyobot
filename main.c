#include <stdio.h>
#include <stdlib.h>

#define WIDTH (6)
#define HEIGTH (10)
#define H_SHIFT (1)
#define V_SHIFT (6)
#define NORTH_WALL (0x3f)
#define WEST_WALL (0x1041041)
#define EAST_WALL (0x20820820)
#define WEST_BLOCK (0x1f7df7df)

#define NUM_FLOORS (2)
#define NUM_COLORS (6)
#define RED (0)
#define GREEN (1)
#define YELLOW (2)
#define BLUE (3)
#define PURPLE (4)
#define GARBAGE (5)


typedef unsigned long int puyos_t;

typedef struct state
{
    puyos_t floors[NUM_FLOORS][NUM_COLORS];
    int chain;
} state;

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

void print_state(state *s) {
    printf(" ");
    for (int i = 0; i < WIDTH; ++i) {
        printf(" %c", 'A' + i);
    }
    printf("\n");
    for (int j = 0; j < NUM_FLOORS; ++j) {
        for (int i = 0; i < HEIGTH * WIDTH; ++i) {
            if (i % V_SHIFT == 0) {
                int l = i / V_SHIFT + j * HEIGTH;
                if (l < 10) {
                    printf("%d", l);
                } else {
                    printf("%c", 'a' + l - 10);
                }
            }
            puyos_t p = (1ULL << i);
            int any = 0;
            for (int k = 0; k < NUM_COLORS; ++k) {
                if (p & s->floors[j][k]) {
                    printf("\x1b[3%dm", k + 1);
                    printf(" ●");
                    any  = 1;
                    break;
                }
            }
            printf("\x1b[0m");
            if (!any) {
                printf("  ");
            }
            if (i % V_SHIFT == V_SHIFT - 1){
                printf("\n");
            }
        }
    }
    printf("chain=%d\n", s->chain);
}

int main() {
    print_puyos(7018292150115006956ULL);
    state *s = calloc(1, sizeof(state));
    s->floors[0][0] = 3;
    s->floors[0][1] = 4;
    s->floors[1][2] = 1;
    s->floors[1][3] = 2;
    s->floors[1][4] = 8;
    s->floors[0][5] = 128;
    print_state(s);
    return 0;
}
