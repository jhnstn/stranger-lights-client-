#include "sequencer.h"
#include "state.h"
#include "renderer.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>

static const char *TAG = "sequencer";

static sequence_buffer_t record_buffer;
static sequence_buffer_t play_buffer;

extern QueueHandle_t cmd_queue;

void sequencer_init(void) {
    sequencer_clear_record_buffer();
    
    // Default Fibonacci sequence: [1, 1, 2, 3, 5, 8, 13, 21, 34]
    int fib[] = {1, 1, 2, 3, 5, 8, 13, 21, 34};
    int num_steps = sizeof(fib) / sizeof(fib[0]);
    
    for (int i = 0; i < num_steps; i++) {
        play_buffer.steps[i].pos = fib[i];
        // 500ms delay between steps, 2000ms on the last one
        play_buffer.steps[i].delay_ms = (i == num_steps - 1) ? 2000 : 500;
    }
    play_buffer.count = num_steps;
    
    ESP_LOGI(TAG, "Sequencer initialized with default Fibonacci sequence (%d steps).", play_buffer.count);
}

void sequencer_record_step(int pos, int delay_ms) {
    if (record_buffer.count < MAX_SEQUENCE_STEPS) {
        if (pos >= 0 && pos < 40) {
            record_buffer.steps[record_buffer.count].pos = pos;
            record_buffer.steps[record_buffer.count].delay_ms = delay_ms;
            record_buffer.count++;
        }
    } else {
        ESP_LOGW(TAG, "Record buffer full!");
    }
}

void sequencer_commit_record_buffer(void) {
    memcpy(&play_buffer, &record_buffer, sizeof(sequence_buffer_t));
    ESP_LOGI(TAG, "Record buffer committed to play buffer. Steps: %d", play_buffer.count);
}

void sequencer_clear_record_buffer(void) {
    record_buffer.count = 0;
    ESP_LOGI(TAG, "Record buffer cleared.");
}

void sequencer_task(void *pvParameters) {
    int loop_index = 0;
    uint32_t last_touch_time = 0;
    
    uint32_t next_scheduled_time = 0;

    while (1) {
        command_t cmd;
        
        // Handle incoming commands (Streaming or Recording)
        if (current_state.mode != MODE_LOOPING) {
            if (xQueueReceive(cmd_queue, &cmd, 10 / portTICK_PERIOD_MS) == pdPASS) {
                if (cmd.type == CMD_P) {
                    touch_event_t event = {
                        .pos = cmd.data.p.pos,
                        .easeIn_ms = current_state.easeIn_ms,
                        .easeOut_ms = current_state.easeOut_ms
                    };
                    renderer_push_touch(event);
                }
            }
        } else {
            // Looping Playback
            if (play_buffer.count > 0) {
                uint32_t now = esp_timer_get_time() / 1000;
                
                if (now >= next_scheduled_time) {
                    sequence_step_t *step = &play_buffer.steps[loop_index];
                    
                    touch_event_t event = {
                        .pos = step->pos,
                        .easeIn_ms = current_state.easeIn_ms,
                        .easeOut_ms = current_state.easeOut_ms
                    };
                    ESP_LOGI(TAG, "Playing step %d: pos=%d, delay=%dms", loop_index, step->pos, step->delay_ms);
                    renderer_push_touch(event);
                    
                    last_touch_time = now;
                    // next = start + easeIn + delay
                    next_scheduled_time = now + current_state.easeIn_ms + step->delay_ms;
                    
                    loop_index = (loop_index + 1) % play_buffer.count;
                }
                vTaskDelay(10 / portTICK_PERIOD_MS);
            } else {
                vTaskDelay(100 / portTICK_PERIOD_MS);
            }
        }
        
        // Reset loop index if mode changes
        if (current_state.mode != MODE_LOOPING) {
            loop_index = 0;
            next_scheduled_time = 0;
        }
    }
}
int sequencer_get_play_count(void) {
    return play_buffer.count;
}

int sequencer_get_record_count(void) {
    return record_buffer.count;
}
