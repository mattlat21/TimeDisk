/**
 * @file app_config.h
 * @brief Application configuration and runtime state.
 *
 * Persistent settings (app_config_t) are stored in NVS namespace timedisk_cfg
 * via app_nvs. Runtime state (app_runtime_t) lives in RAM only.
 * Schema: docs/data_model.md
 */

#pragma once

#include <esp_err.h>
#include <stdbool.h>
#include <stdint.h>

#include "app_nvs.h"

#define APP_WIFI_SSID_MAX     33
#define APP_WIFI_PASSWORD_MAX 65
#define APP_NTP_SERVER_MAX    129
#define APP_TIMEZONE_ID_MAX   48
#define APP_AA_PIN_LEN        5

/** Bit flags for enabled adult-auth methods (see app_config_t::aa_methods). */
#define AA_METHOD_PIN   0x01
#define AA_METHOD_MATHS 0x02

/** Day/night cycle modes driven by the mode engine in ui_nav. */
typedef enum {
    APP_MODE_WAKE = 0,
    APP_MODE_WIND_DOWN,
    APP_MODE_SLEEP,
    APP_MODE_REST,
} app_mode_t;

/** Persistent settings (NVS-backed). */
typedef struct {
    char wifi_ssid[APP_WIFI_SSID_MAX];
    /** false until user completes startup password wizard or Settings saves password. */
    bool wifi_password_set;
    char wifi_password[APP_WIFI_PASSWORD_MAX];
    char ntp_server[APP_NTP_SERVER_MAX];
    /** false until user completes timezone startup wizard. */
    bool timezone_set;
    char timezone_id[APP_TIMEZONE_ID_MAX];

    uint32_t timeout_splash_sec;
    uint32_t timeout_tod_dim_sec;
    uint32_t timeout_aa_sec;
    uint32_t timeout_main_menu_sec;
    uint32_t timeout_timer_dim_sec;

    uint32_t ui_primary_color;
    uint32_t ui_secondary_color;

    uint32_t timer_duration_sec;
    uint8_t timer_style_id;

    uint32_t wind_down_sec;
    uint32_t sleep_sec;
    uint32_t rest_sec;

    uint8_t aa_methods;
    char aa_pin[APP_AA_PIN_LEN];
} app_config_t;

/** Volatile runtime state (not persisted across reboot). */
typedef struct {
    app_mode_t current_mode;
    uint32_t mode_remaining_sec;
    bool cycle_active;
    bool time_valid;
    uint8_t display_brightness;
    uint32_t active_timer_remaining_sec;
    bool timer_running;
} app_runtime_t;

/**
 * Boot-time initialisation: RAM defaults, NVS init, load persisted config.
 * Call once before UI (replaces app_config_init_defaults).
 */
esp_err_t app_config_init(void);

/** Apply factory defaults from data_model.md (does not write NVS). */
void app_config_apply_defaults(void);

/** Reset runtime state to a fresh session (does not touch NVS). */
void app_runtime_reset(void);

app_config_t *app_config_get(void);
app_runtime_t *app_runtime_get(void);

bool app_config_wifi_ssid_missing(void);
bool app_config_wifi_password_unset(void);
bool app_config_timezone_unset(void);

/** Convenience wrappers around app_nvs_save_* (see app_nvs.h). */
static inline esp_err_t app_config_save_all(void)
{
    return app_nvs_save_all();
}

static inline esp_err_t app_config_save_network(void)
{
    return app_nvs_save_network();
}

static inline esp_err_t app_config_save_timezone(void)
{
    return app_nvs_save_timezone();
}

static inline esp_err_t app_config_save_timeouts(void)
{
    return app_nvs_save_timeouts();
}

static inline esp_err_t app_config_save_theme(void)
{
    return app_nvs_save_theme();
}

static inline esp_err_t app_config_save_timer(void)
{
    return app_nvs_save_timer();
}

static inline esp_err_t app_config_save_schedule(void)
{
    return app_nvs_save_schedule();
}

static inline esp_err_t app_config_save_aa(void)
{
    return app_nvs_save_aa();
}

static inline esp_err_t app_config_erase_nvs(void)
{
    return app_nvs_erase_all();
}

static inline bool app_config_has_stored_nvs(void)
{
    return app_nvs_has_stored_config();
}
