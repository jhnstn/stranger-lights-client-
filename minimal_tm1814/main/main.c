#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/rmt_tx.h"

static const char *TAG = "minimal_tm1814";

// ====== User Tunables ======
#define TM1814_GPIO_PIN 13
#define TM1814_NUM_BULBS 40

// Protocol knobs
#define TM1814_INVERT_OUT 1        // 1 = invert in RMT (TM1814 needs inverted data)
#define TM1814_COLOR_ORDER_GRBW 1  // 1 = GRBW, 0 = WRGB

// Timing (800kHz, 1.25us bit time)
#define RMT_RESOLUTION_HZ 10000000 // 10MHz (1 tick = 100ns)
#define T0H_TICKS 4  // 400ns
#define T0L_TICKS 9  // 900ns
#define T1H_TICKS 7  // 700ns
#define T1L_TICKS 6  // 600ns
// Reset: low >300us

// ====== Driver State ======
static rmt_channel_handle_t tx_channel = NULL;
static rmt_encoder_handle_t copy_encoder = NULL;
static uint8_t raw_data[TM1814_NUM_BULBS * 4];
static rmt_symbol_word_t *tx_symbols = NULL;
static size_t symbol_buffer_size = 0;

static void tm1814_init(void) {
    ESP_LOGI(TAG, "Init TM1814: GPIO=%d invert=%d order=%s",
             TM1814_GPIO_PIN,
             TM1814_INVERT_OUT,
             TM1814_COLOR_ORDER_GRBW ? "GRBW" : "WRGB");

    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = TM1814_GPIO_PIN,
        .mem_block_symbols = 64,
        .resolution_hz = RMT_RESOLUTION_HZ,
        .trans_queue_depth = 4,
        .flags.invert_out = TM1814_INVERT_OUT,
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &tx_channel));
    ESP_ERROR_CHECK(rmt_enable(tx_channel));

    rmt_copy_encoder_config_t copy_config = {};
    ESP_ERROR_CHECK(rmt_new_copy_encoder(&copy_config, &copy_encoder));

    memset(raw_data, 0, sizeof(raw_data));
    symbol_buffer_size = sizeof(raw_data) * 8 + 100;
    tx_symbols = malloc(symbol_buffer_size * sizeof(rmt_symbol_word_t));
}

static void tm1814_set_bulb_rgbw(int idx, uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
    if (idx < 0 || idx >= TM1814_NUM_BULBS) return;
    int offset = idx * 4;
    if (TM1814_COLOR_ORDER_GRBW) {
        raw_data[offset + 0] = g;
        raw_data[offset + 1] = r;
        raw_data[offset + 2] = b;
        raw_data[offset + 3] = w;
    } else {
        raw_data[offset + 0] = w;
        raw_data[offset + 1] = r;
        raw_data[offset + 2] = g;
        raw_data[offset + 3] = b;
    }
}

static void tm1814_refresh(void) {
    if (!tx_channel || !tx_symbols) return;

    size_t current_sym = 0;
    for (size_t i = 0; i < sizeof(raw_data); i++) {
        uint8_t byte = raw_data[i];
        for (int bit = 7; bit >= 0; bit--) {
            rmt_symbol_word_t *s = &tx_symbols[current_sym++];
            if (byte & (1 << bit)) {
                s->level0 = 1;
                s->duration0 = T1H_TICKS;
                s->level1 = 0;
                s->duration1 = T1L_TICKS;
            } else {
                s->level0 = 1;
                s->duration0 = T0H_TICKS;
                s->level1 = 0;
                s->duration1 = T0L_TICKS;
            }
        }
    }

    // Reset (low for 300us)
    rmt_symbol_word_t *reset = &tx_symbols[current_sym++];
    reset->level0 = TM1814_INVERT_OUT ? 1 : 0; // keep pin low after inversion
    reset->duration0 = 3000; // 300us
    reset->level1 = reset->level0;
    reset->duration1 = 3000;

    rmt_transmit_config_t tx_config = {
        .loop_count = 0,
    };
    ESP_ERROR_CHECK(rmt_transmit(tx_channel, copy_encoder, tx_symbols,
                                 current_sym * sizeof(rmt_symbol_word_t),
                                 &tx_config));
}

void app_main(void) {
    tm1814_init();

    int state = 0;
    while (1) {
        ESP_LOGI(TAG, "Blink state: %s", state == 0 ? "RED" : "BLUE");
        for (int i = 0; i < TM1814_NUM_BULBS; i++) {
            if (state == 0) {
                tm1814_set_bulb_rgbw(i, 255, 0, 0, 0);
            } else {
                tm1814_set_bulb_rgbw(i, 0, 0, 255, 0);
            }
        }
        tm1814_refresh();
        state = 1 - state;
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
