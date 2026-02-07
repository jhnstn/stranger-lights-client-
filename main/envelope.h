#ifndef ENVELOPE_H
#define ENVELOPE_H

#include <stdint.h>

#define MAX_ACTIVE_ENVELOPES 128

typedef struct {
    int pos;
    uint32_t start_time;
    int easeIn_ms;
    int easeOut_ms;
    float intensity;
    bool active;
} envelope_t;

float envelope_evaluate(envelope_t *env, uint32_t current_time);

#endif // ENVELOPE_H
