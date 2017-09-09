#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "puyobot/record.h"

PlayRecord* play_record_new() {
    PlayRecord *record = malloc(sizeof(PlayRecord));
    record->initial_state = (State){0};
    record->num_moves = 0;
    record->size = 4;
    record->deals = calloc(record->size, sizeof(content_t));
    record->choices = calloc(record->size, sizeof(content_t));
    record->messages = calloc(record->size, sizeof(char) * RECORD_MESSAGE_LEN);
    return record;
}

void play_record_delete(PlayRecord *record) {
    free(record->deals);
    free(record->choices);
    free(record->messages);
    free(record);
}

void play_record_save(PlayRecord *record, const char *filename) {
    FILE *f = fopen(filename, "wb");
    fwrite(record, sizeof(PlayRecord), 1, f);
    fwrite(record->deals, sizeof(content_t), record->size, f);
    fwrite(record->choices, sizeof(content_t), record->size, f);
    fwrite(record->messages, sizeof(char), record->size * RECORD_MESSAGE_LEN, f);
    fclose(f);
}

PlayRecord* play_record_load(const char *filename) {
    FILE *f = fopen(filename, "rb");
    PlayRecord *record = malloc(sizeof(PlayRecord));
    assert(fread(record, sizeof(PlayRecord), 1, f));
    record->deals = malloc(sizeof(content_t) * record->size);
    record->choices = malloc(sizeof(content_t) * record->size);
    record->messages = malloc(sizeof(char) * record->size * RECORD_MESSAGE_LEN);
    assert(fread(record->deals, sizeof(content_t), record->size, f));
    assert(fread(record->choices, sizeof(content_t), record->size, f));
    assert(fread(record->messages, sizeof(char), record->size * RECORD_MESSAGE_LEN, f));
    return record;
}

void record_move(PlayRecord *record, content_t *deals, int num_deals, content_t choice, const char message[]) {
    if (record->num_moves + num_deals > record->size) {
        record->size <<= 1;
        record->deals = realloc(record->deals, record->size * sizeof(content_t));
        record->choices = realloc(record->choices, record->size * sizeof(content_t));
        record->messages = realloc(record->messages, record->size * RECORD_MESSAGE_LEN * sizeof(char));
    }
    for (size_t i = 0; i < num_deals; ++i) {
        record->deals[record->num_moves + i] = deals[i];
        record->choices[record->num_moves + i] = CHOICE_PASS;
    }
    strncpy(record->messages + record->num_moves * RECORD_MESSAGE_LEN, message, RECORD_MESSAGE_LEN);
    record->choices[record->num_moves++] = choice;
}

content_t record_policy(PlayRecord *record, policy_fun policy, void *s, content_t *deals, int  num_deals) {
    content_t choice = policy(s, deals, num_deals);
    record_move(record, deals, num_deals, choice, "");
    return choice;
}
