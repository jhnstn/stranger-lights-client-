#ifndef RENDERER_H
#define RENDERER_H

#include <stdint.h>
#include "esp_err.h"

typedef struct {
    int pos;
    int easeIn_ms;
    int easeOut_ms;
} touch_event_t;

void renderer_init(void);
void renderer_push_touch(touch_event_t event);
void renderer_stop(void);
void renderer_task(void *pvParameters);

#endif // RENDERER_H
