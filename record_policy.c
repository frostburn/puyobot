#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <omp.h>

#include "jkiss/jkiss.h"

#include "puyobot/solver/policy.h"
#include "puyobot/solver/ranking.h"
#include "puyobot/record.h"

int main() {
    jkiss_init();

    omp_set_num_threads(11);

    PlayRecord *record = play_record_new();

    content_t policy (void *s, content_t *deals, int num_deals) {
        return record_policy(record, gcs_policy, s, deals, num_deals);
    }

    RankingOptions options = {0};
    options.num_deals = 3;
    options.min_chain = 2;
    options.num_chains = 2;

    rank_policy(options, policy);

    printf("Saving %zu moves\n", record->num_moves);

    play_record_save(record, "policy.record");

    play_record_delete(record);

    return 0;
}
