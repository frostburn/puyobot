#include <stdlib.h>

#include <ncurses.h>

#include "puyobot/ui/init.h"
#include "puyobot/ui/state.h"

void play_endless() {
    WINDOW *state_win = newwin(2 * HEIGHT + 2, 2 * WIDTH + 3, 0, 0);
    WINDOW *deal_win = newwin(7, 3, 0, 2 * WIDTH + 5);
    WINDOW *score_win = newwin(4, 30, 7, 2 * WIDTH + 5);
    box(state_win, 0, 0);
    box(deal_win, 0, 0);
    box(score_win, 0, 0);
    refresh();

    State *s = calloc(1, sizeof(State));
    content_t deals[NUM_DEALS];
    for (int i = 0; i < NUM_DEALS; ++i) {
        deals[i] = rand_piece();
    }
    int score = 0;
    int last_chain = 0;
    int last_score = 0;
    int x = 0;
    int orientation = 0;
    content_t choice = 0;
    int game_running = 1;
    while (game_running) {
        for (int i = 0; i < NUM_DEALS - 1; ++i) {
            deals[i] = deals[i + 1];
        }
        deals[NUM_DEALS - 1] = rand_piece();
        content_t deal = deals[0];
        int choosing = 1;
        manipulate_piece:
        while (choosing) {
            wmove(score_win, 1, 1);
            wprintw(score_win, "Score=%d", score);
            wmove(score_win, 2, 1);
            wprintw(score_win, "Last chain=%d (%d)\n", last_chain, last_score);
            box(score_win, 0, 0);

            preview_deal_and_choice(state_win, s, deal, choice);
            preview_deals(deal_win, deals + 1, NUM_DEALS - 1);
            wrefresh(state_win);
            wrefresh(deal_win);
            wrefresh(score_win);
            int ch = getch();
            switch (ch)
            {
                case KEY_LEFT:
                    --x;
                    break;
                case KEY_RIGHT:
                    ++x;
                    break;
                case KEY_DOWN:
                    choosing = 0;
                    break;
                case 'f':
                    --orientation;
                    break;
                case 's':
                    ++orientation;
                    break;
                case 'q':
                    choosing = 0;
                    game_running = 0;
                    break;
            }
            choice = make_choice(&x, &orientation);
        }
        if (!game_running) {
            break;
        }
        if (!apply_deal_and_choice(s, deal, choice)) {
            clear_deal_and_choice(s);
            choosing = 1;
            goto manipulate_piece;
        }
        int chain = 0;
        int hit_score = resolve(s, &chain);
        if (hit_score) {
            score += hit_score;
            last_score = hit_score;
            last_chain = chain;
        }
    }
    free(s);
    delwin(state_win);
    delwin(score_win);
    delwin(deal_win);
}

int main() {
    init_ui();

    play_endless();

    endwin();
    return 0;
}
