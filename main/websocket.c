#include <esp_http_server.h>
#include <esp_log.h>
#include "websocket.h"
#include "protocol.h"
#include "state.h"
#include "sequencer.h"

static const char *TAG = "websocket";

static esp_err_t ws_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "Handshake done, the new connection was opened");
        return ESP_OK;
    }

    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    // In ESP-IDF v5, we get the frame length by calling recv with 0 length
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get length with %d", ret);
        return ret;
    }

    if (ws_pkt.len > 0) {
        buf = calloc(1, ws_pkt.len + 1);
        if (buf == NULL) {
            ESP_LOGE(TAG, "Failed to calloc memory for buf");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
            free(buf);
            return ret;
        }
        
        ESP_LOGI(TAG, "Received packet: %s", (char *)ws_pkt.payload);
        
        command_t cmd = protocol_parse_line((char *)ws_pkt.payload);
        if (cmd.type != CMD_INVALID) {
            state_handle_command(cmd);
        } else if (strcmp((char *)ws_pkt.payload, "STATUS") == 0) {
            char status_json[256];
            snprintf(status_json, sizeof(status_json), 
                     "{\"mode\": %d, \"sequence_version\": %lu, \"play_count\": %d, \"record_count\": %d, \"easeIn\": %d, \"easeOut\": %d}",
                     current_state.mode, current_state.sequence_version, 
                     sequencer_get_play_count(), sequencer_get_record_count(),
                     current_state.easeIn_ms, current_state.easeOut_ms);
            
            httpd_ws_frame_t response;
            memset(&response, 0, sizeof(httpd_ws_frame_t));
            response.type = HTTPD_WS_TYPE_TEXT;
            response.payload = (uint8_t *)status_json;
            response.len = strlen(status_json);
            httpd_ws_send_frame(req, &response);
        } else {
            ESP_LOGW(TAG, "Invalid command received");
        }
    }
    free(buf);
    return ESP_OK;
}

static const httpd_uri_t ws = {
    .uri        = "/ws",
    .method     = HTTP_GET,
    .handler    = ws_handler,
    .user_ctx   = NULL,
    .is_websocket = true
};

void start_websocket_server(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    ESP_LOGI(TAG, "Starting WebSocket server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &ws);
    }
}
