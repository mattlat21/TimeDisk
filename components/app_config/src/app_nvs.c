/**
 * @file app_nvs.c
 * @brief NVS load/save for app_config_t per docs/data_model.md.
 *
 * NVS key names are <= 15 characters (ESP-IDF limit). Logical field names
 * are documented beside each key below.
 */

#include "app_nvs.h"
#include "app_config.h"
#include <esp_log.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <stdio.h>
#include <string.h>

static const char *TAG = "app_nvs";

/* Logical field              NVS key        Type */
#define KEY_CFG_VER             "cfg_ver"      /* uint32 schema version */
#define KEY_WIFI_SSID           "wifi_ssid"    /* string wifi_ssid */
#define KEY_WIFI_PASSWORD       "wifi_pass"    /* string wifi_password */
#define KEY_WIFI_PW_SET         "wifi_pw_set"  /* uint8  wifi_password_set flag */
#define KEY_NTP_SERVER          "ntp_server"   /* string ntp_server */
#define KEY_TZ_SET              "tz_set"       /* uint8  timezone_set */
#define KEY_TZ_ID               "tz_id"        /* string timezone_id */
#define KEY_TO_SPLASH           "to_splash"    /* uint32 timeout_splash_sec */
#define KEY_TO_TOD_DIM          "to_tod_dim"   /* uint32 timeout_tod_dim_sec */
#define KEY_TO_AA               "to_aa"        /* uint32 timeout_aa_sec */
#define KEY_TO_MENU             "to_menu"      /* uint32 timeout_main_menu_sec */
#define KEY_TO_TIMER_DIM        "to_tm_dim"    /* uint32 timeout_timer_dim_sec */
#define KEY_UI_PRIMARY          "ui_primary"   /* uint32 ui_primary_color */
#define KEY_UI_SECONDARY        "ui_secondary" /* uint32 ui_secondary_color */
#define KEY_THEME_SET           "theme_set"    /* uint8  theme_set */
#define KEY_TIMER_DUR           "timer_dur"    /* uint32 timer_duration_sec */
#define KEY_TIMER_STYLE         "timer_style"  /* uint8  timer_style_id */
#define KEY_WIND_DOWN           "wind_down"    /* uint32 wind_down_sec */
#define KEY_SLEEP               "sleep_sec"    /* uint32 sleep_sec */
#define KEY_REST                "rest_sec"     /* uint32 rest_sec */
#define KEY_AA_METHODS          "aa_methods"   /* uint8  aa_methods */
#define KEY_AA_PIN              "aa_pin"       /* string aa_pin */

static esp_err_t open_rw(nvs_handle_t *out)
{
    return nvs_open(APP_NVS_NAMESPACE, NVS_READWRITE, out);
}

static esp_err_t open_ro(nvs_handle_t *out)
{
    return nvs_open(APP_NVS_NAMESPACE, NVS_READONLY, out);
}

static esp_err_t set_str(nvs_handle_t h, const char *key, const char *val)
{
    return nvs_set_str(h, key, val);
}

static esp_err_t get_str(nvs_handle_t h, const char *key, char *buf, size_t buf_sz, const char *default_val)
{
    size_t len = buf_sz;
    esp_err_t err = nvs_get_str(h, key, buf, &len);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        if (default_val != NULL) {
            snprintf(buf, buf_sz, "%s", default_val);
        } else {
            buf[0] = '\0';
        }
        return ESP_OK;
    }
    if (err != ESP_OK) {
        return err;
    }
    buf[buf_sz - 1] = '\0';
    return ESP_OK;
}

static esp_err_t set_u32(nvs_handle_t h, const char *key, uint32_t val)
{
    return nvs_set_u32(h, key, val);
}

static esp_err_t get_u32(nvs_handle_t h, const char *key, uint32_t *val, uint32_t default_val)
{
    esp_err_t err = nvs_get_u32(h, key, val);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        *val = default_val;
        return ESP_OK;
    }
    return err;
}

static esp_err_t set_u8(nvs_handle_t h, const char *key, uint8_t val)
{
    return nvs_set_u8(h, key, val);
}

static esp_err_t get_u8(nvs_handle_t h, const char *key, uint8_t *val, uint8_t default_val)
{
    esp_err_t err = nvs_get_u8(h, key, val);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        *val = default_val;
        return ESP_OK;
    }
    return err;
}

static esp_err_t commit(nvs_handle_t h)
{
    return nvs_commit(h);
}

/** Mark namespace as initialised so app_nvs_load() accepts stored keys. */
static esp_err_t touch_cfg_ver(nvs_handle_t h)
{
    return set_u32(h, KEY_CFG_VER, APP_NVS_CFG_VERSION);
}

static void sanitize_loaded_config(app_config_t *cfg)
{
    if (cfg->timer_style_id >= APP_TIMER_STYLE_COUNT) {
        ESP_LOGW(TAG, "timer_style_id %u out of range, clamping to 0", (unsigned)cfg->timer_style_id);
        cfg->timer_style_id = 0;
    }

    cfg->aa_methods &= 0x03;

    if (cfg->timezone_set && cfg->timezone_id[0] == '\0') {
        ESP_LOGW(TAG, "timezone_set without id, clearing flag");
        cfg->timezone_set = false;
    }

    if (strlen(cfg->aa_pin) != 4) {
        ESP_LOGW(TAG, "invalid aa_pin length, resetting to 0000");
        snprintf(cfg->aa_pin, sizeof(cfg->aa_pin), "%s", "0000");
    }
    for (size_t i = 0; i < 4; i++) {
        if (cfg->aa_pin[i] < '0' || cfg->aa_pin[i] > '9') {
            snprintf(cfg->aa_pin, sizeof(cfg->aa_pin), "%s", "0000");
            break;
        }
    }
}

esp_err_t app_nvs_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "nvs_flash_erase (reason %s)", esp_err_to_name(err));
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_flash_init failed: %s", esp_err_to_name(err));
    }
    return err;
}

bool app_nvs_has_stored_config(void)
{
    nvs_handle_t h;
    esp_err_t err = open_ro(&h);
    if (err != ESP_OK) {
        return false;
    }
    uint32_t ver = 0;
    err = nvs_get_u32(h, KEY_CFG_VER, &ver);
    nvs_close(h);
    return err == ESP_OK;
}

esp_err_t app_nvs_load(void)
{
    app_config_t *cfg = app_config_get();
    nvs_handle_t h;
    esp_err_t err = open_ro(&h);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "namespace %s not found, using defaults", APP_NVS_NAMESPACE);
        return ESP_OK;
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open failed: %s", esp_err_to_name(err));
        return err;
    }

    uint32_t cfg_ver = 0;
    err = nvs_get_u32(h, KEY_CFG_VER, &cfg_ver);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "no stored config (cfg_ver missing), using defaults");
        nvs_close(h);
        return ESP_OK;
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "read cfg_ver failed: %s", esp_err_to_name(err));
        nvs_close(h);
        return err;
    }
    if (cfg_ver != APP_NVS_CFG_VERSION) {
        ESP_LOGW(TAG, "cfg_ver %lu != expected %d, using defaults for now",
                 (unsigned long)cfg_ver, APP_NVS_CFG_VERSION);
        nvs_close(h);
        return ESP_OK;
    }

    err = get_str(h, KEY_WIFI_SSID, cfg->wifi_ssid, sizeof(cfg->wifi_ssid), NULL);
    if (err != ESP_OK) {
        goto out;
    }

    uint8_t pw_set = 0;
    err = get_u8(h, KEY_WIFI_PW_SET, &pw_set, 0);
    if (err != ESP_OK) {
        goto out;
    }
    cfg->wifi_password_set = (pw_set != 0);

    err = get_str(h, KEY_WIFI_PASSWORD, cfg->wifi_password, sizeof(cfg->wifi_password), "");
    if (err != ESP_OK) {
        goto out;
    }

    err = get_str(h, KEY_NTP_SERVER, cfg->ntp_server, sizeof(cfg->ntp_server), "pool.ntp.org");
    if (err != ESP_OK) {
        goto out;
    }

    uint8_t tz_set = 0;
    err = get_u8(h, KEY_TZ_SET, &tz_set, 0);
    if (err != ESP_OK) {
        goto out;
    }
    cfg->timezone_set = (tz_set != 0);

    err = get_str(h, KEY_TZ_ID, cfg->timezone_id, sizeof(cfg->timezone_id), "");
    if (err != ESP_OK) {
        goto out;
    }

    err = get_u32(h, KEY_TO_SPLASH, &cfg->timeout_splash_sec, 3);
    if (err != ESP_OK) {
        goto out;
    }
    err = get_u32(h, KEY_TO_TOD_DIM, &cfg->timeout_tod_dim_sec, 600);
    if (err != ESP_OK) {
        goto out;
    }
    err = get_u32(h, KEY_TO_AA, &cfg->timeout_aa_sec, 60);
    if (err != ESP_OK) {
        goto out;
    }
    err = get_u32(h, KEY_TO_MENU, &cfg->timeout_main_menu_sec, 60);
    if (err != ESP_OK) {
        goto out;
    }
    err = get_u32(h, KEY_TO_TIMER_DIM, &cfg->timeout_timer_dim_sec, 900);
    if (err != ESP_OK) {
        goto out;
    }

    err = get_u32(h, KEY_UI_PRIMARY, &cfg->ui_primary_color, 0x7A24BC);
    if (err != ESP_OK) {
        goto out;
    }
    err = get_u32(h, KEY_UI_SECONDARY, &cfg->ui_secondary_color, 0x6BCA24);
    if (err != ESP_OK) {
        goto out;
    }
    {
        uint8_t theme_set = 0;
        err = get_u8(h, KEY_THEME_SET, &theme_set, 0);
        if (err != ESP_OK) {
            goto out;
        }
        cfg->theme_set = (theme_set != 0);
    }

    err = get_u32(h, KEY_TIMER_DUR, &cfg->timer_duration_sec, 300);
    if (err != ESP_OK) {
        goto out;
    }
    err = get_u8(h, KEY_TIMER_STYLE, &cfg->timer_style_id, 0);
    if (err != ESP_OK) {
        goto out;
    }

    err = get_u32(h, KEY_WIND_DOWN, &cfg->wind_down_sec, 0);
    if (err != ESP_OK) {
        goto out;
    }
    err = get_u32(h, KEY_SLEEP, &cfg->sleep_sec, 0);
    if (err != ESP_OK) {
        goto out;
    }
    err = get_u32(h, KEY_REST, &cfg->rest_sec, 0);
    if (err != ESP_OK) {
        goto out;
    }

    err = get_u8(h, KEY_AA_METHODS, &cfg->aa_methods, 0);
    if (err != ESP_OK) {
        goto out;
    }
    err = get_str(h, KEY_AA_PIN, cfg->aa_pin, sizeof(cfg->aa_pin), "0000");
    if (err != ESP_OK) {
        goto out;
    }

    sanitize_loaded_config(cfg);
    ESP_LOGI(TAG, "loaded config v%u from NVS", (unsigned)cfg_ver);

out:
    nvs_close(h);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "load failed: %s", esp_err_to_name(err));
    }
    return err;
}

static esp_err_t save_network_keys(nvs_handle_t h, const app_config_t *cfg)
{
    esp_err_t err = set_str(h, KEY_WIFI_SSID, cfg->wifi_ssid);
    if (err != ESP_OK) {
        return err;
    }
    err = set_str(h, KEY_WIFI_PASSWORD, cfg->wifi_password);
    if (err != ESP_OK) {
        return err;
    }
    return set_u8(h, KEY_WIFI_PW_SET, cfg->wifi_password_set ? 1 : 0);
}

static esp_err_t save_network_keys_and_ntp(nvs_handle_t h, const app_config_t *cfg)
{
    esp_err_t err = save_network_keys(h, cfg);
    if (err != ESP_OK) {
        return err;
    }
    return set_str(h, KEY_NTP_SERVER, cfg->ntp_server);
}

static esp_err_t save_timezone_keys(nvs_handle_t h, const app_config_t *cfg)
{
    esp_err_t err = set_u8(h, KEY_TZ_SET, cfg->timezone_set ? 1 : 0);
    if (err != ESP_OK) {
        return err;
    }
    return set_str(h, KEY_TZ_ID, cfg->timezone_id);
}

esp_err_t app_nvs_save_timezone(void)
{
    const app_config_t *cfg = app_config_get();
    nvs_handle_t h;
    esp_err_t err = open_rw(&h);
    if (err != ESP_OK) {
        return err;
    }
    err = save_timezone_keys(h, cfg);
    if (err == ESP_OK) {
        err = touch_cfg_ver(h);
    }
    if (err == ESP_OK) {
        err = commit(h);
    }
    nvs_close(h);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "saved timezone");
    } else {
        ESP_LOGE(TAG, "save timezone failed: %s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t app_nvs_save_network(void)
{
    const app_config_t *cfg = app_config_get();
    nvs_handle_t h;
    esp_err_t err = open_rw(&h);
    if (err != ESP_OK) {
        return err;
    }
    err = save_network_keys_and_ntp(h, cfg);
    if (err == ESP_OK) {
        err = touch_cfg_ver(h);
    }
    if (err == ESP_OK) {
        err = commit(h);
    }
    nvs_close(h);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "saved network");
    } else {
        ESP_LOGE(TAG, "save network failed: %s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t app_nvs_save_timeouts(void)
{
    const app_config_t *cfg = app_config_get();
    nvs_handle_t h;
    esp_err_t err = open_rw(&h);
    if (err != ESP_OK) {
        return err;
    }
    if ((err = set_u32(h, KEY_TO_SPLASH, cfg->timeout_splash_sec)) != ESP_OK ||
        (err = set_u32(h, KEY_TO_TOD_DIM, cfg->timeout_tod_dim_sec)) != ESP_OK ||
        (err = set_u32(h, KEY_TO_AA, cfg->timeout_aa_sec)) != ESP_OK ||
        (err = set_u32(h, KEY_TO_MENU, cfg->timeout_main_menu_sec)) != ESP_OK ||
        (err = set_u32(h, KEY_TO_TIMER_DIM, cfg->timeout_timer_dim_sec)) != ESP_OK) {
        nvs_close(h);
        return err;
    }
    err = touch_cfg_ver(h);
    if (err == ESP_OK) {
        err = commit(h);
    }
    nvs_close(h);
    return err;
}

esp_err_t app_nvs_save_theme(void)
{
    const app_config_t *cfg = app_config_get();
    nvs_handle_t h;
    esp_err_t err = open_rw(&h);
    if (err != ESP_OK) {
        return err;
    }
    if ((err = set_u32(h, KEY_UI_PRIMARY, cfg->ui_primary_color)) != ESP_OK ||
        (err = set_u32(h, KEY_UI_SECONDARY, cfg->ui_secondary_color)) != ESP_OK ||
        (err = set_u8(h, KEY_THEME_SET, cfg->theme_set ? 1 : 0)) != ESP_OK) {
        nvs_close(h);
        return err;
    }
    err = touch_cfg_ver(h);
    if (err == ESP_OK) {
        err = commit(h);
    }
    nvs_close(h);
    return err;
}

esp_err_t app_nvs_save_timer(void)
{
    const app_config_t *cfg = app_config_get();
    nvs_handle_t h;
    esp_err_t err = open_rw(&h);
    if (err != ESP_OK) {
        return err;
    }
    if ((err = set_u32(h, KEY_TIMER_DUR, cfg->timer_duration_sec)) != ESP_OK ||
        (err = set_u8(h, KEY_TIMER_STYLE, cfg->timer_style_id)) != ESP_OK) {
        nvs_close(h);
        return err;
    }
    err = touch_cfg_ver(h);
    if (err == ESP_OK) {
        err = commit(h);
    }
    nvs_close(h);
    return err;
}

esp_err_t app_nvs_save_schedule(void)
{
    const app_config_t *cfg = app_config_get();
    nvs_handle_t h;
    esp_err_t err = open_rw(&h);
    if (err != ESP_OK) {
        return err;
    }
    if ((err = set_u32(h, KEY_WIND_DOWN, cfg->wind_down_sec)) != ESP_OK ||
        (err = set_u32(h, KEY_SLEEP, cfg->sleep_sec)) != ESP_OK ||
        (err = set_u32(h, KEY_REST, cfg->rest_sec)) != ESP_OK) {
        nvs_close(h);
        return err;
    }
    err = touch_cfg_ver(h);
    if (err == ESP_OK) {
        err = commit(h);
    }
    nvs_close(h);
    return err;
}

esp_err_t app_nvs_save_aa(void)
{
    const app_config_t *cfg = app_config_get();
    nvs_handle_t h;
    esp_err_t err = open_rw(&h);
    if (err != ESP_OK) {
        return err;
    }
    if ((err = set_u8(h, KEY_AA_METHODS, cfg->aa_methods & 0x03)) != ESP_OK ||
        (err = set_str(h, KEY_AA_PIN, cfg->aa_pin)) != ESP_OK) {
        nvs_close(h);
        return err;
    }
    err = touch_cfg_ver(h);
    if (err == ESP_OK) {
        err = commit(h);
    }
    nvs_close(h);
    return err;
}

esp_err_t app_nvs_save_all(void)
{
    const app_config_t *cfg = app_config_get();
    nvs_handle_t h;
    esp_err_t err = open_rw(&h);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs_open failed: %s", esp_err_to_name(err));
        return err;
    }

    err = set_u32(h, KEY_CFG_VER, APP_NVS_CFG_VERSION);
    if (err != ESP_OK) {
        goto out;
    }

    err = save_network_keys_and_ntp(h, cfg);
    if (err != ESP_OK) {
        goto out;
    }

    err = save_timezone_keys(h, cfg);
    if (err != ESP_OK) {
        goto out;
    }

    if ((err = set_u32(h, KEY_TO_SPLASH, cfg->timeout_splash_sec)) != ESP_OK ||
        (err = set_u32(h, KEY_TO_TOD_DIM, cfg->timeout_tod_dim_sec)) != ESP_OK ||
        (err = set_u32(h, KEY_TO_AA, cfg->timeout_aa_sec)) != ESP_OK ||
        (err = set_u32(h, KEY_TO_MENU, cfg->timeout_main_menu_sec)) != ESP_OK ||
        (err = set_u32(h, KEY_TO_TIMER_DIM, cfg->timeout_timer_dim_sec)) != ESP_OK) {
        goto out;
    }

    if ((err = set_u32(h, KEY_UI_PRIMARY, cfg->ui_primary_color)) != ESP_OK ||
        (err = set_u32(h, KEY_UI_SECONDARY, cfg->ui_secondary_color)) != ESP_OK ||
        (err = set_u8(h, KEY_THEME_SET, cfg->theme_set ? 1 : 0)) != ESP_OK) {
        goto out;
    }

    if ((err = set_u32(h, KEY_TIMER_DUR, cfg->timer_duration_sec)) != ESP_OK ||
        (err = set_u8(h, KEY_TIMER_STYLE, cfg->timer_style_id)) != ESP_OK) {
        goto out;
    }

    if ((err = set_u32(h, KEY_WIND_DOWN, cfg->wind_down_sec)) != ESP_OK ||
        (err = set_u32(h, KEY_SLEEP, cfg->sleep_sec)) != ESP_OK ||
        (err = set_u32(h, KEY_REST, cfg->rest_sec)) != ESP_OK) {
        goto out;
    }

    if ((err = set_u8(h, KEY_AA_METHODS, cfg->aa_methods & 0x03)) != ESP_OK ||
        (err = set_str(h, KEY_AA_PIN, cfg->aa_pin)) != ESP_OK) {
        goto out;
    }

    err = commit(h);

out:
    nvs_close(h);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "saved full config");
    } else {
        ESP_LOGE(TAG, "save all failed: %s", esp_err_to_name(err));
    }
    return err;
}

esp_err_t app_nvs_erase_all(void)
{
    nvs_handle_t h;
    esp_err_t err = open_rw(&h);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        return ESP_OK;
    }
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_erase_all(h);
    if (err == ESP_OK) {
        err = commit(h);
    }
    nvs_close(h);
    if (err == ESP_OK) {
        ESP_LOGW(TAG, "erased namespace %s", APP_NVS_NAMESPACE);
    } else {
        ESP_LOGE(TAG, "erase failed: %s", esp_err_to_name(err));
    }
    return err;
}
