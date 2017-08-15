#ifndef PUYOBOT_CONSTANTS_H_GUARD
#define PUYOBOT_CONSTANTS_H_GUARD

#define WIDTH (6)
#define HEIGHT (10)

#define GHOST_Y (7)
#define LIFE_HEIGHT (12)
#define TOTAL_SPACE (WIDTH * (LIFE_HEIGHT + 1))

#define NUM_FLOORS (2)
#define TOTAL_HEIGHT (20)

#define CLEAR_THRESHOLD (4)

#define NUM_DEAL_COLORS (4)

// MAX_CHAIN = (TOTAL_SPACE / CLEAR_THRESHOLD) rounded down.
#define MAX_CHAIN (19)

#endif
