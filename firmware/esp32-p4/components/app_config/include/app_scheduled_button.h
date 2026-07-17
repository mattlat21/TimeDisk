/**
 * @file app_scheduled_button.h
 * @brief Evaluate which scheduled TOD button is active (time window + mode).
 */

#pragma once

#include "app_config.h"

#include <stdbool.h>
#include <stdint.h>

/** True if now_min is inside [start_min, end_min) with overnight / all-day rules. */
bool app_scheduled_button_time_in_window(uint16_t now_min, uint16_t start_min, uint16_t end_min);

/**
 * First enabled button matching local time and current_mode, or NULL.
 * Requires time_valid.
 */
const app_scheduled_button_t *app_scheduled_button_active(void);

/** Label for a schedule action ("Start Sleep", etc.), or NULL if unknown. */
const char *app_scheduled_button_label(uint8_t action);
