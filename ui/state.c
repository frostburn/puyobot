void wprint_state(WINDOW *win, state *s) {
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
}

void preview_deal_and_choice(WINDOW *win, state *s, content_t deal, content_t choice) {
    puyos_t all[2];
    get_state_mask(s, all);
    state *c = copy_state(s);
    apply_deal_and_choice(c, deal, choice);
    handle_gravity(c);
    for (int i = 0; i < NUM_FLOORS; ++i) {
        for (int j = 0; j < NUM_COLORS - 1; ++j) {
            c->floors[i][GARBAGE] |= c->floors[i][j] & ~all[i];
        }
    }
    apply_deal_and_choice(c, deal, choice);
    wprint_state(win, c);
    free(c);
}

void preview_deals(WINDOW *win, content_t *deals, int num_deals) {
    wattron(win, A_BOLD);
    for (int i = 0; i < num_deals; ++i) {
        wmove(win, 1, 2 + 3*i);
        content_t color = deal_color1(deals[i]);
        wattron(win, COLOR_PAIR(color + 1));
        wprintw(win, "0");
        wattroff(win, COLOR_PAIR(color + 1));
    }
    for (int i = 0; i < num_deals; ++i) {
        wmove(win, 2, 2 +  3*i);
        content_t color = deal_color2(deals[i]);
        wattron(win, COLOR_PAIR(color + 1));
        wprintw(win, "0");
        wattroff(win, COLOR_PAIR(color + 1));
    }
}
