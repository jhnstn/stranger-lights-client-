#ifndef SEQUENCER_H
#define SEQUENCER_H

#include <stdint.h>
#include "protocol.h"

#define MAX_SEQUENCE_STEPS 512

typedef struct {
    int pos;
    int delay_ms;
} sequence_step_t;

typedef struct {
    sequence_step_t steps[MAX_SEQUENCE_STEPS];
    int count;
} sequence_buffer_t;

void sequencer_init(void);
void sequencer_record_step(int pos, int delay_ms);
void sequencer_commit_record_buffer(void);
void sequencer_clear_record_buffer(void);
void sequencer_task(void *pvParameters);
int sequencer_get_play_count(void);
int sequencer_get_record_count(void);

#endif // SEQUENCER_H
