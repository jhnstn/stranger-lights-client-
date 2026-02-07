#include <stdio.h>
#include "nvs_flash.h"
#include "esp_log.h"
#include "wifi.h"
#include "state.h"
#include "websocket.h"
#include "sequencer.h"
#include "renderer.h"
#include "mqtt_handler.h"

static const char *TAG = "main";

void app_main(void) {
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Starting TM1814 Light Controller...");

    // Initialize State Machine
    state_init();

    // Initialize Wi-Fi
    wifi_init_sta();

    // Start WebSocket Server
    start_websocket_server();

    // Initialize and Start MQTT
    mqtt_handler_init();
    mqtt_handler_start();

    // Start Sequencer and Renderer Tasks
    // xTaskCreate(sequencer_task, "sequencer_task", 4096, NULL, 5, NULL); // DISABLED FOR DEBUG
    xTaskCreate(renderer_task, "renderer_task", 4096, NULL, 6, NULL);

    ESP_LOGI(TAG, "System initialized.");
}
