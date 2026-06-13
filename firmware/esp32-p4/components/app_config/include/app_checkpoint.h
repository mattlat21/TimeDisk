/**
 * @file app_checkpoint.h
 * @brief UTC end-time checkpoints for mode cycle and timer recovery across reboot.
 *
 * End times are Unix epoch seconds (UTC). NVS keys: cyc_winddn_end, cyc_sleep_end,
 * cyc_rest_end, timer_start, timer_end — see docs/data_model.md.
 */

#pragma once

#include "app_config.h"

#include <esp_err.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

typedef struct {
    time_t winddown_end;
    time_t sleep_end;
    time_t rest_end;
    time_t timer_start;
    time_t timer_end;
} app_checkpoint_ends_t;

typedef struct {
    bool cycle_active;
    app_mode_t cycle_mode;
    uint32_t cycle_remaining_sec;
    bool timer_running;
    uint32_t timer_remaining_sec;
} app_checkpoint_status_t;

esp_err_t app_checkpoint_load_ends(app_checkpoint_ends_t *out);
esp_err_t app_checkpoint_save_ends(const app_checkpoint_ends_t *ends);

esp_err_t app_checkpoint_start_cycle(time_t start_utc,
                                     uint32_t wind_down_sec,
                                     uint32_t sleep_sec,
                                     uint32_t rest_sec);

esp_err_t app_checkpoint_clear_cycle(void);
esp_err_t app_checkpoint_set_timer(time_t start_utc, time_t end_utc);
esp_err_t app_checkpoint_cancel_timer(time_t now);
esp_err_t app_checkpoint_clear_timer(void);

void app_checkpoint_status_at(const app_checkpoint_ends_t *ends,
                              time_t now,
                              app_checkpoint_status_t *out);

esp_err_t app_checkpoint_get_status(time_t now, app_checkpoint_status_t *out);
esp_err_t app_checkpoint_apply_to_runtime(time_t now);
