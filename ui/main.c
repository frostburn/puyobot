#include <assert.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ncurses.h>

#include "../constants.c"
#include "..jkiss/include/jkiss/jkiss.h"
#include "../jkiss/src/jkiss.c"
#include "../bitboard.c"
#include "../scoring.c"
#include "../state.c"
#include "../deal.c"
#include "../multiplayer.c"

#include "../eval.c"
#include "../tree.c"

#include "state.c"

#define NUM_DEALS (3)

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

void play_endless() {
    WINDOW *state_win = newwin(2 * HEIGHT + 2, 2 * WIDTH + 3, 0, 0);
    WINDOW *deal_win = newwin(7, 3, 0, 2 * WIDTH + 5);
    WINDOW *score_win = newwin(4, 30, 7, 2 * WIDTH + 5);
    box(state_win, 0, 0);
    box(deal_win, 0, 0);
    box(score_win, 0, 0);
    refresh();

    state *s = calloc(1, sizeof(state));
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
                case 'd':
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

content_t mc_practice_policy(void *pg, content_t *deals, int num_deals) {
    mc_options options = simple_mc_options(10000, random_policy);
    options.step = step_practice;
    options.copy = copy_practice;
    return iterate_mc(pg, deals, num_deals, options);
}

void play_multiplayer() {
    WINDOW *left_player_win = newwin(2 * HEIGHT + 2, 2 * WIDTH + 3, 0, 0);
    WINDOW *right_player_win = newwin(2 * HEIGHT + 2, 2 * WIDTH + 3, 0, 2 * WIDTH + 15);
    WINDOW *left_deal_win = newwin(7, 3, 0, 2 * WIDTH + 5);
    WINDOW *right_deal_win = newwin(7, 3, 0, 2 * WIDTH + 10);
    WINDOW *left_status_win = newwin(9, 2 * WIDTH + 10, 2 * HEIGHT + 3, 0);
    WINDOW *right_status_win = newwin(9, 2 * WIDTH + 10, 2 * HEIGHT + 3, 2 * WIDTH + 15);
    box(left_player_win, 0, 0);
    box(right_player_win, 0, 0);
    box(left_deal_win, 0, 0);
    box(right_deal_win, 0, 0);
    box(left_status_win, 0, 0);
    box(right_status_win, 0, 0);
    refresh();

    game *g = new_game(2, NUM_DEALS);
    content_t choices[2];

    wprint_state(left_player_win, &g->players[0].state);
    wprint_state(right_player_win, &g->players[1].state);
    wrefresh(left_player_win);
    wrefresh(right_player_win);

    int x = 0;
    int orientation = 0;
    content_t choice = 0;
    int game_running = 1;
    state *pop_previews[2] = {0};
    while (game_running) {
        practice_game *pg;
        preview_deals(left_deal_win, g->deals + 1 + g->players[0].deal_index, g->num_deals - 1);
        preview_deals(right_deal_win, g->deals + 1 + g->players[1].deal_index, g->num_deals - 1);
        if (pop_previews[1]) {
            wprint_state(right_player_win, pop_previews[1]);
            free(pop_previews[1]);
        }
        wprint_game_status(left_status_win, g, 0);
        box(left_status_win, 0, 0);
        wprint_game_status(right_status_win, g, 1);
        box(right_status_win, 0, 0);

        pg = game_as_practice(g, 1);
        if (pg) {
            // choices[1] = mc_practice_policy(pg, pg->deals, pg->num_deals);  // monte carlo alternative
            choices[1] = gcn_practice_policy(pg, pg->deals, pg->num_deals);
            preview_deal_and_choice(right_player_win, &pg->player.state, pg->deals[0], choices[1]);
            free(pg);
        }

        wrefresh(right_player_win);
        wrefresh(left_deal_win);
        wrefresh(right_deal_win);
        wrefresh(left_status_win);
        wrefresh(right_status_win);

        int choosing = 1;
        content_t deal = g->deals[g->players[0].deal_index];
        pg = game_as_practice(g, 0);
        state *s = NULL;
        if (pg) {
            s = &pg->player.state;
        } else {
            if (pop_previews[0]) {
                // Apply the here deal to preview it too.
                apply_deal_and_choice(pop_previews[0], deal, choice);
                wprint_state(left_player_win, pop_previews[0]);
                free(pop_previews[0]);
            }
            wrefresh(left_player_win);
            choosing = 0;
            getch();
        }
        manipulate_piece:
        while (choosing) {
            preview_deal_and_choice(left_player_win, s, deal, choice);
            wrefresh(left_player_win);
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
                case 'd':
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
        if (s && !state_is_full(s) && !apply_deal_and_choice(s, deal, choice)) {
            clear_deal_and_choice(s);
            choosing = 1;
            goto manipulate_piece;
        }
        choices[0] = choice;
        free(pg);

        for (int i = 0; i < g->num_players; ++i) {
            pg = game_as_practice(g, i);
            if (pg) {
                apply_deal_and_choice(&pg->player.state, g->deals[g->players[i].deal_index], choices[i]);
                pop_previews[i] = pop_preview(&pg->player.state);
                free(pg);
            }
            else {
                pop_previews[i] = pop_preview(&g->players[i].state);
            }
        }
        step_game(g, choices);
    }
    delwin(left_player_win);
    delwin(right_player_win);
    delwin(left_deal_win);
    delwin(right_deal_win);
    delwin(left_status_win);
    delwin(right_status_win);
    free(g);
}

void main() {
    jkiss_init();
    setlocale(LC_ALL, "");
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    init_colors();

    // play_endless();
    play_multiplayer();

    endwin();
}
