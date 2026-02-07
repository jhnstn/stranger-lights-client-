#ifndef STATE_H
#define STATE_H

#include <stdint.h>
#include "protocol.h"

typedef enum {
    MODE_STREAMING,
    MODE_RECORDING,
    MODE_LOOPING
} controller_mode_t;

typedef struct {
    controller_mode_t mode;
    uint32_t sequence_version;
    int easeIn_ms;
    int easeOut_ms;
} controller_state_t;

extern controller_state_t current_state;

void state_init(void);
void state_handle_command(command_t cmd);

#endif // STATE_H
