/**
 * @file app_ota.h
 * @brief Over-the-air firmware update via HTTP(S).
 */

#pragma once

#include <esp_err.h>
#include <stdbool.h>
#include <stddef.h>

/** Default OTA firmware URL (Settings → Update). */
#define APP_UPDATE_DEFAULT_FIRMWARE_URL "http://latimer.net/files/tdisk/latest.bin"

#define APP_UPDATE_URL_MAX 256

typedef enum {
    APP_UPDATE_STATE_IDLE = 0,
    APP_UPDATE_STATE_RUNNING,
    APP_UPDATE_STATE_SUCCESS,
    APP_UPDATE_STATE_FAILED,
} app_update_state_t;

typedef void (*app_update_progress_cb_t)(int percent, const char *status, void *user_data);
typedef void (*app_update_done_cb_t)(esp_err_t err, const char *message, void *user_data);

const char *app_update_get_version(void);
app_update_state_t app_update_get_state(void);
bool app_update_active(void);

/**
 * Download and install firmware from @p url on a background task.
 * Callbacks are invoked from the OTA task (not the LVGL thread).
 */
esp_err_t app_update_start(const char *url, app_update_progress_cb_t progress_cb,
                           app_update_done_cb_t done_cb, void *user_data);
