#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "esp_err.h"

typedef enum {
    CMD_LS,
    CMD_LE,
    CMD_STOP,
    CMD_P,
    CMD_PARAM,
    CMD_INVALID
} command_type_t;

typedef struct {
    command_type_t type;
    union {
        struct {
            int pos;
            int delay_ms;
        } p;
        struct {
            int easeIn_ms;
            int easeOut_ms;
        } param;
    } data;
} command_t;

command_t protocol_parse_line(const char *line);

#endif // PROTOCOL_H
