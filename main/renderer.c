#include <string.h>
#include "renderer.h"
#include "envelope.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "tm1814_driver.h"

static const char *TAG = "renderer";

static envelope_t envelopes[MAX_ACTIVE_ENVELOPES];
static QueueHandle_t touch_queue;

void renderer_init(void) {
    memset(envelopes, 0, sizeof(envelopes));
    touch_queue = xQueueCreate(64, sizeof(touch_event_t));
    tm1814_init();
}

void renderer_push_touch(touch_event_t event) {
    if (xQueueSend(touch_queue, &event, 0) != pdPASS) {
        ESP_LOGW(TAG, "Touch queue overflow!");
    }
}

void renderer_stop(void) {
    for (int i = 0; i < MAX_ACTIVE_ENVELOPES; i++) {
        envelopes[i].active = false;
    }
    for (int i = 0; i < TM1814_NUM_BULBS; i++) {
        tm1814_set_bulb_intensity(i, 0.0f);
    }
    tm1814_refresh();
    ESP_LOGI(TAG, "Renderer stopped and cleared.");
}

// static void process_touch_events(void) {
//     touch_event_t event;
//     while (xQueueReceive(touch_queue, &event, 0) == pdPASS) {
//         bool found = false;
//         for (int i = 0; i < MAX_ACTIVE_ENVELOPES; i++) {
//             if (!envelopes[i].active) {
//                 envelopes[i].pos = event.pos;
//                 envelopes[i].easeIn_ms = event.easeIn_ms;
//                 envelopes[i].easeOut_ms = event.easeOut_ms;
//                 envelopes[i].start_time = esp_timer_get_time() / 1000;
//                 envelopes[i].active = true;
//                 found = true;
//                 break;
//             }
//         }
//         if (!found) {
//             ESP_LOGW(TAG, "No free envelope slots!");
//         }
//     }
// }

void renderer_task(void *pvParameters) {
    const TickType_t refresh_period = pdMS_TO_TICKS(100); // Refresh every 100ms (10Hz)
    TickType_t last_refresh = xTaskGetTickCount();
    
    int64_t last_toggle_time = esp_timer_get_time() / 1000;
    int debug_state = 0;

    while (1) {
        // Toggle color every 500ms
        int64_t now = esp_timer_get_time() / 1000;
        if (now - last_toggle_time > 2000) {
            debug_state = (debug_state + 1) % 2; // Red/Blue only
            last_toggle_time = now;
            
            ESP_LOGW(TAG, "DEBUG: State %d", debug_state);
            
            for (int i = 0; i < TM1814_NUM_BULBS; i++) {
                switch (debug_state) {
                    case 0: tm1814_set_bulb_rgbw(i, 255, 0, 0, 0); break; // Red
                    case 1: tm1814_set_bulb_rgbw(i, 0, 0, 255, 0); break; // Blue
                }
            }
        }

        // Always refresh to keep TM1814 happy
        tm1814_refresh();
        vTaskDelayUntil(&last_refresh, refresh_period);
    }
}
