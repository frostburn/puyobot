#ifndef PUYOBOT_RECORD_H_GUARD
#define PUYOBOT_RECORD_H_GUARD

#include "puyobot/solver/policy.h"
#include "puyobot/deal.h"

typedef struct PlayRecord {
    State initial_state;
    size_t num_moves;
    size_t size;
    size_t head;
    content_t *deals;
    content_t *choices;
} PlayRecord;

PlayRecord* play_record_new();

void play_record_delete(PlayRecord *record);

void play_record_save(PlayRecord *record, const char *filename);

PlayRecord* play_record_load(const char *filename);

void record_move(PlayRecord *record, content_t *deals, int num_deals, content_t choice);

content_t record_policy(PlayRecord *record, policy_fun policy, void *s, content_t *deals, int  num_deals);

#endif
