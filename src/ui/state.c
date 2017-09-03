#include <stdlib.h>

#include "puyobot/ui/state.h"

void wprint_state(WINDOW *win, State *s) {
    wmove(win, 0, 1);
    for (int i = 0; i < WIDTH; ++i) {
        wprintw(win, " %c", 'A' + i);
    }
    wmove(win, 1, 0);
    for (int j = 0; j < NUM_FLOORS; ++j) {
        for (int i = 0; i < HEIGHT * WIDTH; ++i) {
            if (i % V_SHIFT == 0) {
                int l = i / V_SHIFT + j * HEIGHT;
                wmove(win, l + 1, 0);
                if (l < 10) {
                    wprintw(win, "%d", l);
                } else {
                    wprintw(win, "%c", 'a' + l - 10);
                }
            }
            wattron(win, A_BOLD);
            if (j == 0 && i / WIDTH == GHOST_Y) {
                wattroff(win, A_BOLD);
            }
            puyos_t p = (1ULL << i);
            int any = 0;
            for (int k = 0; k < NUM_COLORS; ++k) {
                if (p & s->floors[j][k]) {
                    wattron(win, COLOR_PAIR(k + 1));
                    // Alas, ncursesw doesn't clear wide unicode characters cleanly...
                    if (k == GARBAGE) {
                        wprintw(win, " O");
                        // wprintw(win, " ◎");
                    } else {
                        // Landing previews are implemented as overlaid bitboards
                        if (p & s->floors[j][GARBAGE]) {
                            wprintw(win, " .");
                            // wprintw(win, " ◌");
                        } else {
                            wprintw(win, " 0");
                            // wprintw(win, " ●");
                        }
                    }
                    wattroff(win, COLOR_PAIR(k + 1));
                    any  = 1;
                    break;
                }
            }
            wattroff(win, A_BOLD);
            if (!any) {
                wprintw(win, "  ");
            }
        }
    }
    wmove(win, HEIGHT * 2 + 1, 0);
}

void just_apply_deal_and_choice(State *state, content_t deal, content_t choice) {
    if (choice == CHOICE_PASS) {
        return;
    }
    content_t color1 = deal & COLOR1_MASK;
    content_t color2 = deal >> COLOR2_SHIFT;
    content_t orientation = choice & ~CHOICE_X_MASK;
    content_t color1_x = choice & CHOICE_X_MASK;
    content_t color2_x = color1_x;
    content_t color1_y = 0;
    content_t color2_y = 1;
    if (orientation == CHOICE_90) {
        color2_x++;
        color2_y--;
    } else if (orientation == CHOICE_180) {
        color1_y++;
        color2_y--;
    } else if (orientation == CHOICE_270) {
        color1_x++;
        color2_y--;
    }
    puyos_t all = 0;
    for (int i = 0; i < NUM_COLORS; ++i) {
        all |= state->floors[0][i];
    }

    puyos_t puyo1 = 1ULL << (color1_x + V_SHIFT * color1_y);
    state->floors[0][color1] |= puyo1;
    puyos_t puyo2 = 1ULL << (color2_x + V_SHIFT * color2_y);
    state->floors[0][color2] |= puyo2;
}

void preview_deal_and_choice(WINDOW *win, State *s, content_t deal, content_t choice) {
    puyos_t all[2];
    get_state_mask(s, all);
    State *c = copy_state(s);
    just_apply_deal_and_choice(c, deal, choice);
    handle_gravity(c);
    for (int i = 0; i < NUM_FLOORS; ++i) {
        for (int j = 0; j < NUM_COLORS - 1; ++j) {
            c->floors[i][GARBAGE] |= c->floors[i][j] & ~all[i];
        }
    }
    just_apply_deal_and_choice(c, deal, choice);
    for (int j = 0; j < NUM_COLORS - 1; ++j) {
        puyos_t piece = c->floors[0][j] & (TOP | (TOP << V_SHIFT));
        c->floors[0][j] ^= piece;
        c->floors[0][j] |= piece << ((GHOST_Y - 4) * V_SHIFT);
    }
    wprint_state(win, c);
    free(c);
}

void preview_deals(WINDOW *win, content_t *deals, int num_deals) {
    wattron(win, A_BOLD);
    int y = 1;
    for (int i = 0; i < num_deals; ++i) {
        wmove(win, y, 1);
        content_t color = deal_color1(deals[i]);
        wattron(win, COLOR_PAIR(color + 1));
        wprintw(win, "0");
        wattroff(win, COLOR_PAIR(color + 1));
        ++y;
        wmove(win, y, 1);
        color = deal_color2(deals[i]);
        wattron(win, COLOR_PAIR(color + 1));
        wprintw(win, "0");
        wattroff(win, COLOR_PAIR(color + 1));
        y += 2;
    }
    wattroff(win, A_BOLD);
}

void wprint_player(WINDOW *win, Player *p) {
    wprint_state(win, &p->state);
    double sending = (p->chain_score + p->leftover_score) / (double)TARGET_SCORE;
    wprintw(win,
        "score=%d\nchain=%d\nsending=%.2f\nreceiving=%d\nall clear=%d\ngame overs=%d\n",
        p->total_score, p->chain, sending, p->pending_nuisance, p->all_clear_bonus, p->game_overs
    );
}

void wprint_game_status(WINDOW *win, Game *g, int player_index) {
    PracticeGame *pg = game_as_practice(g, player_index);
    Player *p;
    int incoming = -1;
    int delay = 0;
    if (pg) {
        p = &pg->player;
        incoming = pg->incoming;
        delay = pg->delay;
        free(pg);
    } else {
        p = g->players + player_index;
    }
    double sending = (p->chain_score + p->leftover_score) / (double)TARGET_SCORE;
    wmove(win, 1, 1);
    wprintw(win,
        "score=%d\n chain=%d\n sending=%.2f\n receiving=%d\n all clear=%d\n game overs=%d\n",
        p->total_score, p->chain, sending, p->pending_nuisance, p->all_clear_bonus, p->game_overs
    );
    if (incoming >= 0) {
        wprintw(win, " incoming=%d/%d\n", incoming, delay);
    } else {
        wprintw(win, " incoming=n/a\n");
    }
}

State* pop_preview(State *s) {
    puyos_t all[2];
    State *c = copy_state(s);
    handle_gravity(c);
    State *cc = copy_state(c);
    clear_groups(cc, 0);
    get_state_mask(cc, all);
    free(cc);
    for (int i = 0; i < NUM_FLOORS; ++i) {
        for (int j = 0; j < NUM_COLORS - 1; ++j) {
            c->floors[i][GARBAGE] |= c->floors[i][j] & ~all[i];
        }
    }
    return c;
}
