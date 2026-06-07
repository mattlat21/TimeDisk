/**
 * @file app_config.c
 * @brief Default configuration, runtime state, and boot init (NVS load).
 */

#include "app_config.h"
#include <esp_log.h>
#include <stdio.h>
#include <string.h>

static const char *TAG = "app_config";

static app_config_t s_cfg;
static app_runtime_t s_rt;

void app_config_apply_defaults(void)
{
    memset(&s_cfg, 0, sizeof(s_cfg));

    snprintf(s_cfg.ntp_server, sizeof(s_cfg.ntp_server), "%s", "pool.ntp.org");

    s_cfg.timeout_splash_sec = 3;
    s_cfg.timeout_tod_dim_sec = 600;
    s_cfg.timeout_tod_menu_sec = 120;
    s_cfg.timeout_aa_sec = 60;
    s_cfg.timeout_main_menu_sec = 60;
    s_cfg.timeout_timer_dim_sec = 900;

    s_cfg.ui_primary_color = 0x7A24BC;
    s_cfg.ui_secondary_color = 0x6BCA24;

    s_cfg.timer_duration_sec = 300;
    s_cfg.timer_style_id = 0;

    s_cfg.wind_down_sec = 0;
    s_cfg.sleep_sec = 0;
    s_cfg.rest_sec = 0;

    s_cfg.aa_methods = 0x00;
    snprintf(s_cfg.aa_pin, sizeof(s_cfg.aa_pin), "%s", "0000");

    s_cfg.wifi_network_count = 0;
    s_cfg.timezone_set = false;
    s_cfg.timezone_id[0] = '\0';
    s_cfg.theme_set = false;
}

void app_runtime_reset(void)
{
    memset(&s_rt, 0, sizeof(s_rt));
    s_rt.current_mode = APP_MODE_WAKE;
    s_rt.display_brightness = 100;
}

esp_err_t app_config_init(void)
{
    app_config_apply_defaults();
    app_runtime_reset();

    esp_err_t err = app_nvs_init();
    if (err != ESP_OK) {
        return err;
    }

    err = app_nvs_load();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS load failed, continuing with defaults");
    }

    if (app_nvs_has_stored_config()) {
        ESP_LOGI(TAG, "config loaded from NVS");
    } else {
        ESP_LOGI(TAG, "no NVS config yet (factory defaults in RAM)");
    }

    return ESP_OK;
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
    return s_cfg.wifi_network_count == 0;
}

bool app_config_wifi_password_unset(void)
{
    return app_config_wifi_network_password_unset(0);
}

int app_config_wifi_network_count(void)
{
    return (int)s_cfg.wifi_network_count;
}

const app_wifi_network_t *app_config_wifi_network_get(int index)
{
    if (index < 0 || index >= (int)s_cfg.wifi_network_count) {
        return NULL;
    }
    return &s_cfg.wifi_networks[index];
}

bool app_config_wifi_network_password_unset(int index)
{
    const app_wifi_network_t *net = app_config_wifi_network_get(index);
    if (net == NULL) {
        return true;
    }
    return !net->password_set;
}

void app_config_wifi_networks_copy(app_wifi_network_t *dst, uint8_t *dst_count,
                                   const app_wifi_network_t *src, uint8_t src_count)
{
    if (dst == NULL || dst_count == NULL) {
        return;
    }
    if (src_count > APP_WIFI_NETWORK_MAX) {
        src_count = APP_WIFI_NETWORK_MAX;
    }
    *dst_count = src_count;
    if (src_count > 0 && src != NULL) {
        memcpy(dst, src, (size_t)src_count * sizeof(app_wifi_network_t));
    }
}

esp_err_t app_config_wifi_network_set(int index, const char *ssid, const char *password, bool password_set)
{
    if (ssid == NULL || ssid[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }
    if (index < 0 || index > (int)s_cfg.wifi_network_count) {
        return ESP_ERR_INVALID_ARG;
    }
    if (index == (int)s_cfg.wifi_network_count) {
        if (s_cfg.wifi_network_count >= APP_WIFI_NETWORK_MAX) {
            return ESP_ERR_NO_MEM;
        }
        index = (int)s_cfg.wifi_network_count;
        s_cfg.wifi_network_count++;
    }

    app_wifi_network_t *net = &s_cfg.wifi_networks[index];
    snprintf(net->ssid, sizeof(net->ssid), "%s", ssid);
    if (password != NULL) {
        snprintf(net->password, sizeof(net->password), "%s", password);
        net->password_set = password_set;
    }
    return ESP_OK;
}

esp_err_t app_config_wifi_network_delete(int index)
{
    if (index < 0 || index >= (int)s_cfg.wifi_network_count) {
        return ESP_ERR_INVALID_ARG;
    }

    for (int i = index; i < (int)s_cfg.wifi_network_count - 1; i++) {
        s_cfg.wifi_networks[i] = s_cfg.wifi_networks[i + 1];
    }
    memset(&s_cfg.wifi_networks[s_cfg.wifi_network_count - 1], 0, sizeof(app_wifi_network_t));
    s_cfg.wifi_network_count--;
    return ESP_OK;
}

bool app_config_timezone_unset(void)
{
    return !s_cfg.timezone_set;
}

bool app_config_theme_unset(void)
{
    return !s_cfg.theme_set;
}
