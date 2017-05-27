bottom_template* chain_of_order(int num_links, int order, int force_order) {
    bottom_template *template = calloc(1, sizeof(bottom_template));
    while (template->num_links < num_links) {
        for (int i = 0; i < num_links; ++i) {
            puyos_t fixed = 0;
            if (order > 0) {
                for (int j = order; j < template->num_links; ++j) {
                    fixed |= template->floor[j];
                }
            }
            if (!extend_bottom_chain(template, fixed)) {
                break;
            }
            if (order > 0 && force_order) {
                int o = order;
                if (i < order) {
                    o = i;
                }
                puyos_t beam = beam_up(template->floor[0]);
                for (int j = 1; j <= o; ++j) {
                    if (!(template->floor[j] & beam)) {
                        template->num_links = 0;
                        break;
                    }
                }
                if (!template->num_links) {
                    break;
                }
            }
        }
        if (template->num_links < num_links) {
            free(template->floor);
            template->floor = NULL;
            template->num_links = template->num_colors = 0;
        }
    }
    return template;
}

bottom_template *any_long_chain(int min_links) {
    bottom_template *template = calloc(1, sizeof(bottom_template));
    do {
        while(extend_bottom_chain(template, 0));
        while(tail_bottom_chain(template));
        if (template->num_links < min_links) {
            free(template->floor);
            template->floor = NULL;
            template->num_links = template->num_colors = 0;
        }
    } while (template->num_links < min_links);
    return template;
}
