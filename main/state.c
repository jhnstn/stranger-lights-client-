#include "state.h"
#include "sequencer.h"
#include "renderer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "mqtt_handler.h"

static const char *TAG = "state";
static const char *MQTT_TOPIC_STATUS = "stranger/lights/status";

controller_state_t current_state;
QueueHandle_t cmd_queue;

static void publish_state(void) {
    const char *state_str = "UNKNOWN";
    switch (current_state.mode) {
        case MODE_RECORDING: state_str = "RECORDING"; break;
        case MODE_LOOPING:   state_str = "PLAYING"; break;
        case MODE_STREAMING: state_str = "STOPPED"; break;
        default: break;
    }
    mqtt_handler_publish(MQTT_TOPIC_STATUS, state_str, 0, 1); // QOS 0, Retain 1
}

void state_init(void) {
    current_state.mode = MODE_LOOPING;
    current_state.sequence_version = 0;
    current_state.easeIn_ms = 1000;
    current_state.easeOut_ms = 1000;
    
    cmd_queue = xQueueCreate(32, sizeof(command_t));
    sequencer_init();
    renderer_init();
    ESP_LOGI(TAG, "State machine initialized. Default mode: LOOPING");
    publish_state();
}

void state_handle_command(command_t cmd) {
    switch (cmd.type) {
        case CMD_LS:
            current_state.mode = MODE_RECORDING;
            sequencer_clear_record_buffer();
            ESP_LOGI(TAG, "Mode transition: RECORDING");
            break;
        case CMD_LE:
            current_state.mode = MODE_LOOPING;
            current_state.sequence_version++;
            sequencer_commit_record_buffer();
            ESP_LOGI(TAG, "Mode transition: LOOPING. Sequence version: %lu", current_state.sequence_version);
            break;
        case CMD_STOP:
            current_state.mode = MODE_STREAMING;
            renderer_stop();
            ESP_LOGI(TAG, "Mode transition: STREAMING (STOP)");
            break;
        case CMD_PARAM:
            current_state.easeIn_ms = cmd.data.param.easeIn_ms;
            current_state.easeOut_ms = cmd.data.param.easeOut_ms;
            ESP_LOGI(TAG, "Params updated: easeIn=%d, easeOut=%d", current_state.easeIn_ms, current_state.easeOut_ms);
            break;
        case CMD_P:
            if (current_state.mode == MODE_RECORDING) {
                sequencer_record_step(cmd.data.p.pos, cmd.data.p.delay_ms);
            }
            // Always queue for live execution (streaming or recording)
            xQueueSend(cmd_queue, &cmd, 0);
            break;
        default:
            break;
    }
    
    // Publish state if it changed the mode
    if (cmd.type == CMD_LS || cmd.type == CMD_LE || cmd.type == CMD_STOP) {
        publish_state();
    }
}

