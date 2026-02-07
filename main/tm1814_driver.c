#include "tm1814_driver.h"
#include "driver/rmt_tx.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "tm1814";

// TM1814 Timing Constants (800kHz, inverted)
// 1.25us total: 0 -> 350/900ns, 1 -> 700/550ns (approx)

#define TM1814_INVERT_OUT 1

#define RMT_RESOLUTION_HZ 10000000 // 10MHz (1 tick = 100ns)

#define T0H_TICKS 4  // 400ns
#define T0L_TICKS 9  // 900ns
#define T1H_TICKS 7  // 700ns
#define T1L_TICKS 6  // 600ns

static rmt_channel_handle_t tx_channel = NULL;
static rmt_encoder_handle_t copy_encoder = NULL;

// Raw buffer for RGBW data: 4 bytes per bulb
static uint8_t raw_data[TM1814_NUM_BULBS * 4];

// Pre-allocated symbol buffer
static size_t symbol_buffer_size = 0;
static rmt_symbol_word_t *tx_symbols = NULL;

void tm1814_init(void) {
    ESP_LOGI(TAG, "Initializing custom TM1814 driver on GPIO %d (invert=%d, 800kHz)", TM1814_GPIO_PIN, TM1814_INVERT_OUT);
    
    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = TM1814_GPIO_PIN,
        .mem_block_symbols = 64,
        .resolution_hz = RMT_RESOLUTION_HZ,
        .trans_queue_depth = 4,
        .flags.invert_out = TM1814_INVERT_OUT, // Direct wiring: invert in RMT for TM1814
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_chan_config, &tx_channel));
    ESP_ERROR_CHECK(rmt_enable(tx_channel));
    
    rmt_copy_encoder_config_t copy_config = {};
    ESP_ERROR_CHECK(rmt_new_copy_encoder(&copy_config, &copy_encoder));
    
    memset(raw_data, 0, sizeof(raw_data));
    
    // Pre-allocate symbol buffer (8 symbols per byte + reset)
    symbol_buffer_size = sizeof(raw_data) * 8 + 100;
    tx_symbols = malloc(symbol_buffer_size * sizeof(rmt_symbol_word_t));
}

void tm1814_set_bulb_rgbw(int bulb_idx, uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
    if (bulb_idx < 0 || bulb_idx >= TM1814_NUM_BULBS) return;

    // TM1814 Order: Usually WRGB or RGBW?
    // Datasheets vary. Let's try W-R-G-B order based on some libraries.
    // Or standard RGBW.
    // WLED uses: R, G, B, W.
    
    int offset = bulb_idx * 4;
    // Adafruit/WLED for these strands: GRB + W last (GRBW).
    raw_data[offset + 0] = g;
    raw_data[offset + 1] = r;
    raw_data[offset + 2] = b;
    raw_data[offset + 3] = w;
}

void tm1814_set_bulb_intensity(int bulb_idx, float intensity) {
    // Legacy support: Just sets White channel
    tm1814_set_bulb_rgbw(bulb_idx, 0, 0, 0, (uint8_t)(intensity * 255.0f));
}

void tm1814_refresh(void) {
    if (!tx_channel || !tx_symbols) return;

    size_t current_sym = 0;
    
    for (size_t i = 0; i < sizeof(raw_data); i++) {
        uint8_t byte = raw_data[i];
        for (int bit = 7; bit >= 0; bit--) {
            rmt_symbol_word_t *s = &tx_symbols[current_sym++];
            
            if (byte & (1 << bit)) {
                // Bit 1
                s->level0 = 1;
                s->duration0 = T1H_TICKS;
                s->level1 = 0;
                s->duration1 = T1L_TICKS;
            } else {
                // Bit 0
                s->level0 = 1;
                s->duration0 = T0H_TICKS;
                s->level1 = 0;
                s->duration1 = T0L_TICKS;
            }
        }
    }
    
    // Reset code (Low for 300us). With invert_out = 1, level0=0 means pin High.
    
    rmt_symbol_word_t *reset = &tx_symbols[current_sym++];
    reset->level0 = TM1814_INVERT_OUT ? 1 : 0; // Keep pin Low for reset
    reset->duration0 = 3000; // 300us
    reset->level1 = 0;
    reset->duration1 = 3000; // padding
    
    // Transmit
    rmt_transmit_config_t tx_config = {
        .loop_count = 0,
    };
    ESP_ERROR_CHECK(rmt_transmit(tx_channel, copy_encoder, tx_symbols, current_sym * sizeof(rmt_symbol_word_t), &tx_config));
}
