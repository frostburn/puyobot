#include <stdlib.h>
#include <string.h>

#include <ncurses.h>

#include "puyobot/ui/init.h"
#include "puyobot/ui/state.h"
#include "puyobot/record.h"

PlayRecord* todo_actually_load_stuff() {
    PlayRecord *record = play_record_new();
    for (int i = 0; i < 100; ++i) {
        content_t deal = rand_piece();
        record_move(record, &deal, 1, CHOICES[jrand() % NUM_CHOICES]);
    }
    return record;
}

void playback() {
    WINDOW *state_win = newwin(2 * HEIGHT + 2, 2 * WIDTH + 3, 0, 0);
    WINDOW *deal_win = newwin(7, 3, 0, 2 * WIDTH + 5);
    WINDOW *score_win = newwin(4, 30, 7, 2 * WIDTH + 5);
    box(state_win, 0, 0);
    box(deal_win, 0, 0);
    box(score_win, 0, 0);
    refresh();

    PlayRecord *record = todo_actually_load_stuff();  // TODO
    size_t record_index = 0;

    State *history = malloc(sizeof(State));
    history[0] = record->initial_state;
    size_t history_size = 1;

    int *score_history = malloc(sizeof(int));
    score_history[0] = 0;
    int *chain_history = malloc(sizeof(int));
    chain_history[0] = 0;
    int game_running = 1;
    while (game_running) {
        int choosing = 1;
        while (choosing) {
            wmove(score_win, 1, 1);
            wprintw(score_win, "Score=%d              ", score_history[record_index]);
            wmove(score_win, 2, 1);
            wprintw(score_win, "Chain=%d              ", chain_history[record_index]);
            box(score_win, 0, 0);

            preview_deal_and_choice(state_win, history + record_index, record->deals[record_index], record->choices[record_index]);
            preview_deals(deal_win, record->deals + record_index + 1, NUM_DEALS - 1);
            wrefresh(state_win);
            wrefresh(deal_win);
            wrefresh(score_win);
            int ch = getch();
            switch (ch)
            {
                case KEY_LEFT:
                    if (record_index > 0) {
                        --record_index;
                        choosing = 0;
                    }
                    break;
                case KEY_RIGHT:
                    if (record_index < record->num_moves -1) {
                        ++record_index;
                        choosing = 0;
                    }
                    break;
                case 'q':
                    choosing = 0;
                    game_running = 0;
                    break;
            }
        }
        if (!game_running) {
            break;
        }
        if (record_index >= history_size) {
            ++history_size;
            history = realloc(history, history_size * sizeof(State));
            chain_history = realloc(chain_history, history_size * sizeof(int));
            score_history = realloc(score_history, history_size * sizeof(int));
            memcpy(history + history_size - 1, history + history_size - 2, sizeof(State));
            if (!apply_deal_and_choice(history + history_size - 1, record->deals[record_index - 1], record->choices[record_index - 1])) {
                history[history_size - 1] = record->initial_state;
            }
            int chain = 0;
            int score = resolve(history + history_size - 1, &chain);
            chain_history[history_size - 1] = chain;
            score_history[history_size - 1] = score;
        }
    }
    free(history);
    free(score_history);
    free(chain_history);
    delwin(state_win);
    delwin(score_win);
    delwin(deal_win);
}

int main() {
    init_ui();

    playback();

    endwin();
    return 0;
}
