#ifndef WIFI_H
#define WIFI_H

#include "esp_err.h"

#define WIFI_SSID "MONSTERS"
#define WIFI_PASS "3234745713"
#define MAXIMUM_RETRY 5

void wifi_init_sta(void);

#endif // WIFI_H
