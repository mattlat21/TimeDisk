/**
 * @file app_config.c
 * @brief Default configuration and accessors for app_config_t / app_runtime_t.
 */

#include "app_config.h"
#include <stdio.h>
#include <string.h>

static app_config_t s_cfg;
static app_runtime_t s_rt;

void app_config_init_defaults(void)
{
    memset(&s_cfg, 0, sizeof(s_cfg));
    memset(&s_rt, 0, sizeof(s_rt));

    snprintf(s_cfg.ntp_server, sizeof(s_cfg.ntp_server), "%s", "pool.ntp.org");

    s_cfg.timeout_splash_sec = 3;
    s_cfg.timeout_tod_dim_sec = 600;
    s_cfg.timeout_aa_sec = 60;
    s_cfg.timeout_main_menu_sec = 60;
    s_cfg.timeout_timer_dim_sec = 900;

    s_cfg.ui_primary_color = 0x7A24BC;
    s_cfg.ui_secondary_color = 0x6BCA24;

    s_cfg.timer_duration_sec = 300;
    s_cfg.timer_style_id = 0;

    s_cfg.aa_methods = 0x03;
    snprintf(s_cfg.aa_pin, sizeof(s_cfg.aa_pin), "%s", "0000");

    s_rt.current_mode = APP_MODE_WAKE;
    s_rt.display_brightness = 100;
}

app_config_t *app_config_get(void)
{
    return &s_cfg;
}

app_runtime_t *app_runtime_get(void)
{
    return &s_rt;
}

bool app_config_wifi_ssid_missing(void)
{
    return s_cfg.wifi_ssid[0] == '\0';
}

bool app_config_wifi_password_unset(void)
{
    return !s_cfg.wifi_password_set;
}
