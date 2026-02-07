#include "mqtt_handler.h"
#include "mqtt_client.h"
#include "protocol.h"
#include "state.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "mqtt_handler";
static esp_mqtt_client_handle_t client;

// Topic to subscribe to
#define MQTT_TOPIC_CMD "stranger/lights/cmd"

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            msg_id = esp_mqtt_client_subscribe(client, MQTT_TOPIC_CMD, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);

            if (strncmp(event->topic, MQTT_TOPIC_CMD, event->topic_len) == 0) {
                // Copy data to null-terminated string
                char *buf = malloc(event->data_len + 1);
                if (buf) {
                    memcpy(buf, event->data, event->data_len);
                    buf[event->data_len] = '\0';
                    
                    command_t cmd = protocol_parse_line(buf);
                    if (cmd.type != CMD_INVALID) {
                        state_handle_command(cmd);
                    }
                    free(buf);
                }
            }
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

void mqtt_handler_init(void) {
    // Note: In a real scenario, the broker URL should be configurable via flash or environment
    // For now, we will use a placeholder or local IP if the user provided one.
    // The user mentioned "Mosquitto on GCP", so we'll likely need a public IP later.
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://136.118.248.38",
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
}

void mqtt_handler_start(void) {
    esp_mqtt_client_start(client);
    ESP_LOGI(TAG, "MQTT client started.");
}

int mqtt_handler_publish(const char *topic, const char *data, int qos, int retain) {
    if (client == NULL) {
        return -1;
    }
    return esp_mqtt_client_publish(client, topic, data, 0, qos, retain);
}
