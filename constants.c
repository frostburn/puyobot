#define WIDTH (6)
#define HEIGHT (10)
#define H_SHIFT (1)
#define V_SHIFT (6)
#define FULL (0xfffffffffffffffULL)
#define TOP (0x3fULL)
#define BOTTOM (0xfc0000000000000ULL)
#define LEFT_WALL (0x41041041041041ULL)
#define RIGHT_BLOCK (0xfbefbefbefbefbeULL)
#define RIGHT_WALL (0x820820820820820ULL)
#define LEFT_BLOCK (0x7df7df7df7df7dfULL)
#define LEFT_SIDE (0x1c71c71c71c71c7ULL)
#define RIGHT_SIDE (0xe38e38e38e38e38ULL)
#define TOP_TO_BOTTOM (V_SHIFT * (HEIGHT - 1))

#define GHOST_Y (7)
#define DEATH_BLOCK (0x3ffffffffffULL)
#define GHOST_LINE (0xfc0000000000ULL)
#define LIFE_BLOCK (0xfff000000000000ULL)
#define LIFE_HEIGHT (12)

#define TOTAL_SPACE (WIDTH * (LIFE_HEIGHT + 1))

#define NUM_FLOORS (2)
#define TOTAL_HEIGHT (20)
#define NUM_COLORS (6)
#define RED (0)
#define GREEN (1)
#define YELLOW (2)
#define BLUE (3)
#define PURPLE (4)
#define GARBAGE (5)
#define CLEAR_THRESHOLD (4)

#define MAX_GROUPS (WIDTH * LIFE_HEIGHT / 2)  // Assuming even width and height

#define MAX_DEALS (5)

static char* COLOR_NAMES[NUM_COLORS] = {"red", "green", "yellow", "blue", "purple", "garbage"};
