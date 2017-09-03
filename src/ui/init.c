#include <assert.h>
#include <locale.h>

#include <ncurses.h>

#include "jkiss/jkiss.h"

#include "puyobot/state.h"

void init_colors() {
    assert(has_colors() != FALSE);
    start_color();
    init_pair(RED + 1, COLOR_RED, COLOR_BLACK);
    init_pair(GREEN + 1, COLOR_GREEN, COLOR_BLACK);
    init_pair(YELLOW + 1, COLOR_YELLOW, COLOR_BLACK);
    init_pair(BLUE + 1, COLOR_BLUE, COLOR_BLACK);
    init_pair(PURPLE + 1, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(GARBAGE + 1, COLOR_CYAN, COLOR_BLACK);
}

void init_ui() {
    jkiss_init();
    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    init_colors();
}
