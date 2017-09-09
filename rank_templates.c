#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <omp.h>

#include "puyobot/solver/policy.h"
#include "puyobot/solver/ranking.h"
#include "puyobot/template/bottom.h"
#include "puyobot/template/bottom_match.h"
#include "puyobot/template/pattern.h"

BottomTemplate* get_template() {
    BottomTemplate *template;
    while (1) {
        template = bottom_chain_of_fours(7);
        if (!chip_bottom_trigger(template)) {
            free_bottom_template(template);
            continue;
        }
        break;
    }
    prepare_bottom_template(template, 0);
    return template;
}

int main() {
    jkiss_init();
    init_tetrominoes();

    omp_set_num_threads(11);

    double best_efficiency = 0;
    BottomTemplate *best_template = NULL;

    while (1) {
        BottomTemplate *template = get_template();
        print_bottom_template(template);

        content_t policy(void *s, content_t *deals, int  num_deals) {
            return template_policy(template, 0, 0.01, s, deals, num_deals);
        }

        RankingOptions options = {0};
        options.num_deals = 3;
        options.min_chain = 2;
        options.num_chains = 12;

        RankingResult result = iter_rank_policy(options, policy, 5);
        print_ranking_result(result, 1);

        double efficiency = result.score / ((double) result.puyos_played);
        if (!best_template || efficiency > best_efficiency) {
            free(best_template);
            best_template = copy_bottom_template(template);
            best_efficiency = efficiency;
        }
        printf("\nBest efficiency so far=%f\n", best_efficiency); 
        repr_bottom_template_floor(best_template);

        free_bottom_template(template);
    }

    return 0;
}
