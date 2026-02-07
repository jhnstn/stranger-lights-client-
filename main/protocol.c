#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "protocol.h"
#include "esp_log.h"

static const char *TAG = "protocol";

command_t protocol_parse_line(const char *line) {
    command_t cmd = { .type = CMD_INVALID };
    
    if (line == NULL || strlen(line) == 0) return cmd;

    if (strncmp(line, "LS", 2) == 0) {
        cmd.type = CMD_LS;
    } else if (strncmp(line, "LE", 2) == 0) {
        cmd.type = CMD_LE;
    } else if (strncmp(line, "STOP", 4) == 0) {
        cmd.type = CMD_STOP;
    } else if (line[0] == 'P' && line[1] == ' ') {
        if (sscanf(line, "P %d %d", &cmd.data.p.pos, &cmd.data.p.delay_ms) == 2) {
            cmd.type = CMD_P;
        }
    } else if (strncmp(line, "PARAM ", 6) == 0) {
        if (sscanf(line, "PARAM %d %d", &cmd.data.param.easeIn_ms, &cmd.data.param.easeOut_ms) == 2) {
            cmd.type = CMD_PARAM;
        }
    }

    return cmd;
}
