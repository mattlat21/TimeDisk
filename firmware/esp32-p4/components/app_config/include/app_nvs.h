/**
 * @file app_nvs.h
 * @brief NVS persistence for app_config_t (namespace timedisk_cfg).
 *
 * Key layout and defaults match docs/data_model.md. Schedule durations and UTC
 * checkpoint end times are stored here; volatile countdown state is in RAM unless
 * restored via app_checkpoint after SNTP.
 */

#pragma once

#include <esp_err.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

/** NVS namespace for persisted settings. */
#define APP_NVS_NAMESPACE "timedisk_cfg"

/** Schema version written on save; bump when keys or semantics change. */
#define APP_NVS_CFG_VERSION 4

/** Number of timer styles (ring, water). */
#define APP_TIMER_STYLE_COUNT 2

#define APP_TIMER_STYLE_RING  0
#define APP_TIMER_STYLE_WATER 1

/**
 * Initialise NVS flash partition (safe to call once at boot).
 * Does not load configuration into RAM.
 */
esp_err_t app_nvs_init(void);

/** Load all persisted fields into app_config_get(); missing keys keep RAM defaults. */
esp_err_t app_nvs_load(void);

/** Persist the full app_config_t snapshot. */
esp_err_t app_nvs_save_all(void);

esp_err_t app_nvs_save_network(void);
esp_err_t app_nvs_save_timezone(void);
esp_err_t app_nvs_save_timeouts(void);
esp_err_t app_nvs_save_theme(void);
esp_err_t app_nvs_save_timer(void);
esp_err_t app_nvs_save_schedule(void);
esp_err_t app_nvs_save_aa(void);
esp_err_t app_nvs_save_mqtt(void);

/** Erase the timedisk_cfg namespace (factory reset of settings). */
esp_err_t app_nvs_erase_all(void);

/** True if cfg_ver exists in NVS (device has been saved at least once). */
bool app_nvs_has_stored_config(void);

/** UTC epoch end times for cycle/timer recovery (schema v4). */
esp_err_t app_nvs_checkpoint_load(time_t *winddown_end, time_t *sleep_end,
                                  time_t *rest_end, time_t *timer_start, time_t *timer_end);
esp_err_t app_nvs_checkpoint_save_cycle(time_t winddown_end, time_t sleep_end, time_t rest_end);
esp_err_t app_nvs_checkpoint_save_timer(time_t timer_start, time_t timer_end);
