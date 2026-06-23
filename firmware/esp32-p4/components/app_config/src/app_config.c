/**
 * @file app_config.c
 * @brief Default configuration, runtime state, and boot init (NVS load).
 */

#include "app_config.h"
#include <esp_log.h>
#include <esp_mac.h>
#include <soc/soc_caps.h>
#include <stdio.h>
#include <string.h>

static const char *TAG = "app_config";

static app_config_t s_cfg;
static app_runtime_t s_rt;
static void (*s_mqtt_saved_hook)(void);
static void (*s_display_saved_hook)(void);
static char s_device_id[40];
static bool s_device_id_ready;

static esp_err_t read_device_mac(uint8_t mac[6])
{
#if defined(SOC_WIFI_SUPPORTED) && SOC_WIFI_SUPPORTED
    if (esp_read_mac(mac, ESP_MAC_WIFI_STA) == ESP_OK) {
        return ESP_OK;
    }
#endif
    if (esp_read_mac(mac, ESP_MAC_ETH) == ESP_OK) {
        return ESP_OK;
    }
    return esp_read_mac(mac, ESP_MAC_EFUSE_FACTORY);
}

void app_config_set_mqtt_saved_hook(void (*hook)(void))
{
    s_mqtt_saved_hook = hook;
}

void app_config_set_display_saved_hook(void (*hook)(void))
{
    s_display_saved_hook = hook;
}

bool app_config_get_device_id(char *out, size_t out_len)
{
    if (out == NULL || out_len == 0) {
        return false;
    }
    if (!s_device_id_ready) {
        uint8_t mac[6];
        if (read_device_mac(mac) != ESP_OK) {
            snprintf(s_device_id, sizeof(s_device_id), "timedisk-unknown");
        } else {
            snprintf(s_device_id, sizeof(s_device_id), "timedisk-%02X%02X%02X%02X%02X%02X",
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        }
        s_device_id_ready = true;
    }
    snprintf(out, out_len, "%s", s_device_id);
    return true;
}

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
    s_cfg.timeout_timer_done_sec = 30;

    s_cfg.backlight_bright_pct = 100;
    s_cfg.backlight_dim_pct = 30;

    s_cfg.ui_primary_color = 0x7A24BC;
    s_cfg.ui_secondary_color = 0x6BCA24;

    s_cfg.timer_duration_sec = 300;
    s_cfg.timer_style_id = 0;

    s_cfg.wind_down_sec = 0;
    s_cfg.sleep_sec = 0;
    s_cfg.rest_sec = 0;

    s_cfg.aa_methods = 0x00;
    snprintf(s_cfg.aa_pin, sizeof(s_cfg.aa_pin), "%s", "0000");

    s_cfg.mqtt_enabled = false;
    s_cfg.mqtt_host[0] = '\0';
    s_cfg.mqtt_port = 1883;
    s_cfg.mqtt_username[0] = '\0';
    s_cfg.mqtt_password[0] = '\0';

    s_cfg.wifi_network_count = 0;
    s_cfg.schedule_event_count = 0;
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

int app_config_schedule_event_count(void)
{
    return (int)s_cfg.schedule_event_count;
}

const app_schedule_event_t *app_config_schedule_event_get(int index)
{
    if (index < 0 || index >= (int)s_cfg.schedule_event_count) {
        return NULL;
    }
    return &s_cfg.schedule_events[index];
}

void app_config_schedule_events_copy(app_schedule_event_t *dst, uint8_t *dst_count,
                                     const app_schedule_event_t *src, uint8_t src_count)
{
    if (dst == NULL || dst_count == NULL) {
        return;
    }
    if (src_count > APP_SCHEDULE_EVENT_MAX) {
        src_count = APP_SCHEDULE_EVENT_MAX;
    }
    *dst_count = src_count;
    if (src_count > 0 && src != NULL) {
        memcpy(dst, src, (size_t)src_count * sizeof(app_schedule_event_t));
    }
}

void app_config_schedule_events_sort_buf(app_schedule_event_t *events, uint8_t count)
{
    if (events == NULL || count <= 1) {
        return;
    }

    for (uint8_t i = 1; i < count; i++) {
        app_schedule_event_t tmp = events[i];
        uint8_t j = i;
        while (j > 0 && events[j - 1].time_min > tmp.time_min) {
            events[j] = events[j - 1];
            j--;
        }
        events[j] = tmp;
    }
}

void app_config_schedule_events_sort(void)
{
    app_config_schedule_events_sort_buf(s_cfg.schedule_events, s_cfg.schedule_event_count);
}

static esp_err_t schedule_event_sanitize(const app_schedule_event_t *event)
{
    if (event == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (event->time_min >= 24U * 60U) {
        return ESP_ERR_INVALID_ARG;
    }
    if (event->action > APP_SCHEDULE_ACTION_START_REST) {
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

esp_err_t app_config_schedule_event_set(int index, const app_schedule_event_t *event)
{
    esp_err_t err = schedule_event_sanitize(event);
    if (err != ESP_OK) {
        return err;
    }

    if (index < 0 || index > (int)s_cfg.schedule_event_count) {
        return ESP_ERR_INVALID_ARG;
    }
    if (index == (int)s_cfg.schedule_event_count) {
        if (s_cfg.schedule_event_count >= APP_SCHEDULE_EVENT_MAX) {
            return ESP_ERR_NO_MEM;
        }
        index = (int)s_cfg.schedule_event_count;
        s_cfg.schedule_event_count++;
    }

    s_cfg.schedule_events[index] = *event;
    app_config_schedule_events_sort();
    return ESP_OK;
}

esp_err_t app_config_schedule_event_delete(int index)
{
    if (index < 0 || index >= (int)s_cfg.schedule_event_count) {
        return ESP_ERR_INVALID_ARG;
    }

    for (int i = index; i < (int)s_cfg.schedule_event_count - 1; i++) {
        s_cfg.schedule_events[i] = s_cfg.schedule_events[i + 1];
    }
    memset(&s_cfg.schedule_events[s_cfg.schedule_event_count - 1], 0,
           sizeof(app_schedule_event_t));
    s_cfg.schedule_event_count--;
    app_config_schedule_events_sort();
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

bool app_config_first_boot_pending(void)
{
    return !s_cfg.first_boot_done;
}

esp_err_t app_config_mark_first_boot_done(void)
{
    s_cfg.first_boot_done = true;
    return app_nvs_save_all();
}

esp_err_t app_config_factory_reset(void)
{
    esp_err_t err = app_nvs_erase_all();
    if (err != ESP_OK) {
        return err;
    }
    app_config_apply_defaults();
    app_runtime_reset();
    return ESP_OK;
}

esp_err_t app_config_save_mqtt(void)
{
    esp_err_t err = app_nvs_save_mqtt();
    if (err == ESP_OK && s_mqtt_saved_hook != NULL) {
        s_mqtt_saved_hook();
    }
    return err;
}

esp_err_t app_config_save_display(void)
{
    esp_err_t err = app_nvs_save_display();
    if (err == ESP_OK && s_display_saved_hook != NULL) {
        s_display_saved_hook();
    }
    return err;
}
