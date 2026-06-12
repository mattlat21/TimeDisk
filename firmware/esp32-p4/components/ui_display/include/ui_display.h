/**
 * @file ui_display.h
 * @brief Project-owned display settings (invert, rotation, etc.).
 */

#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Apply display settings from Kconfig after BSP/LVGL init.
 */
esp_err_t ui_display_apply_settings(void);

#ifdef __cplusplus
}
#endif
