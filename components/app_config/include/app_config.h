/**
 * @file app_config.h
 * @brief Application configuration and runtime state (RAM-backed; NVS planned).
 *
 * Holds WiFi credentials, UI timeouts, theme colors, timer/schedule settings,
 * and adult-authentication options. Separate from LVGL/UI code so logic can be
 * tested and persisted without pulling in the graphics stack.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#define APP_WIFI_SSID_MAX     33
#define APP_WIFI_PASSWORD_MAX 65
#define APP_NTP_SERVER_MAX    129
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

/** Persistent settings (future: NVS blob). */
typedef struct {
    char wifi_ssid[APP_WIFI_SSID_MAX];
    bool wifi_password_set;
    char wifi_password[APP_WIFI_PASSWORD_MAX];
    char ntp_server[APP_NTP_SERVER_MAX];

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

void app_config_init_defaults(void);
app_config_t *app_config_get(void);
app_runtime_t *app_runtime_get(void);

bool app_config_wifi_ssid_missing(void);
bool app_config_wifi_password_unset(void);
