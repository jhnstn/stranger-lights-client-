#include "envelope.h"
#include <esp_timer.h>

float envelope_evaluate(envelope_t *env, uint32_t current_time) {
    if (!env->active) return 0.0f;

    uint32_t elapsed = current_time - env->start_time;
    uint32_t total_duration = env->easeIn_ms + env->easeOut_ms;

    if (elapsed >= total_duration) {
        env->active = false;
        return 0.0f;
    }

    if (elapsed < (uint32_t)env->easeIn_ms) {
        // Ease-in ramp: 0 -> 1
        return (float)elapsed / (float)env->easeIn_ms;
    } else {
        // Ease-out ramp: 1 -> 0
        uint32_t ease_out_elapsed = elapsed - env->easeIn_ms;
        return 1.0f - ((float)ease_out_elapsed / (float)env->easeOut_ms);
    }
}
