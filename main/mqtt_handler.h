#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include "esp_err.h"

/**
 * @brief Initialize the MQTT client and start the task.
 */
void mqtt_handler_init(void);

/**
 * @brief Start the MQTT client.
 */
void mqtt_handler_start(void);

/**
 * @brief Publish a message to a topic.
 */
int mqtt_handler_publish(const char *topic, const char *data, int qos, int retain);

#endif // MQTT_HANDLER_H
