// Classic scoring
#define NUM_GROUP_BONUS (8)
#define NUM_CHAIN_POWERS (24)
#define MAX_CLEAR_BONUS (999)

static int COLOR_BONUS[NUM_COLORS] = {0, 0, 3, 6, 12, 24};
static int GROUP_BONUS[NUM_GROUP_BONUS] = {0, 2, 3, 4, 5, 6, 7, 10};
static int CHAIN_POWERS[NUM_CHAIN_POWERS] = {
    0, 8, 16, 32, 64, 96, 128, 160, 192, 224, 256, 288,
    320, 352, 384, 416, 448, 480, 512, 544, 576, 608, 640, 672
};
