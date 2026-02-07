#ifndef TM1814_DRIVER_H
#define TM1814_DRIVER_H

#include <stdint.h>
#include "esp_err.h"

#define TM1814_GPIO_PIN 13
#define TM1814_NUM_BULBS 40

void tm1814_init(void);
void tm1814_set_bulb_rgbw(int bulb_idx, uint8_t r, uint8_t g, uint8_t b, uint8_t w);
void tm1814_set_bulb_intensity(int bulb_idx, float intensity);
void tm1814_refresh(void);

#endif // TM1814_DRIVER_H
