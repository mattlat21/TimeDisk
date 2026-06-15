/**
 * @file app_network_setup.c
 * @brief Fallback soft-AP and HTTP web UI for device configuration.
 */

#include "app_network.h"
#include "app_network_web_ui.h"
#include "app_config.h"

#include "injected/esp_wifi.h"

#include <esp_http_server.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include <string.h>

static const char *TAG = "app_net_setup";

#define SETUP_AP_SSID       APP_NETWORK_SETUP_AP_SSID
#define SETUP_AP_CHANNEL    1
#define SETUP_AP_MAX_CONN   4

static httpd_handle_t s_httpd;
static bool s_setup_ap_active;
static char s_setup_status[96];

static void set_setup_status(const char *status)
{
    if (status != NULL) {
        snprintf(s_setup_status, sizeof(s_setup_status), "%s", status);
    }
}

bool app_network_setup_ap_active(void)
{
    return s_setup_ap_active;
}

static esp_err_t setup_http_start(void)
{
    if (s_httpd != NULL) {
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 16;
    config.stack_size = 8192;

    esp_err_t err = httpd_start(&s_httpd, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start failed: %s", esp_err_to_name(err));
        return err;
    }

    err = app_network_web_ui_register(s_httpd);
    if (err != ESP_OK) {
        httpd_stop(s_httpd);
        s_httpd = NULL;
        return err;
    }

    ESP_LOGI(TAG, "web UI started");
    return ESP_OK;
}

esp_err_t app_network_web_ui_start(void)
{
    return setup_http_start();
}

bool app_network_web_ui_active(void)
{
    return s_httpd != NULL;
}

static void setup_http_stop(void)
{
    if (s_httpd != NULL) {
        httpd_stop(s_httpd);
        s_httpd = NULL;
    }
}

static esp_err_t setup_ap_begin(void)
{
    esp_netif_create_default_wifi_ap();

    esp_wifi_stop();
    vTaskDelay(pdMS_TO_TICKS(100));

    wifi_config_t ap_cfg = {0};
    snprintf((char *)ap_cfg.ap.ssid, sizeof(ap_cfg.ap.ssid), "%s", SETUP_AP_SSID);
    ap_cfg.ap.ssid_len = (uint8_t)strlen(SETUP_AP_SSID);
    ap_cfg.ap.channel = SETUP_AP_CHANNEL;
    ap_cfg.ap.max_connection = SETUP_AP_MAX_CONN;
    ap_cfg.ap.authmode = WIFI_AUTH_OPEN;

    esp_err_t err = esp_wifi_set_mode(WIFI_MODE_AP);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_wifi_set_config(WIFI_IF_AP, &ap_cfg);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_wifi_start();
    if (err != ESP_OK) {
        return err;
    }

    esp_netif_t *ap = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    esp_netif_ip_info_t ip = {0};
    if (ap != NULL && esp_netif_get_ip_info(ap, &ip) == ESP_OK) {
        char msg[96];
        snprintf(msg, sizeof(msg), "Join %s · http://" IPSTR, SETUP_AP_SSID, IP2STR(&ip.ip));
        set_setup_status(msg);
    } else {
        set_setup_status("Join " SETUP_AP_SSID " · http://192.168.4.1");
    }

    return ESP_OK;
}

esp_err_t app_network_setup_ap_start(void)
{
    if (s_setup_ap_active) {
        return ESP_OK;
    }

    esp_err_t err = setup_ap_begin();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "AP start failed: %s", esp_err_to_name(err));
        return err;
    }

    err = setup_http_start();
    if (err != ESP_OK) {
        esp_wifi_stop();
        return err;
    }

    s_setup_ap_active = true;
    ESP_LOGI(TAG, "setup AP active (%s)", s_setup_status);
    return ESP_OK;
}

void app_network_setup_ap_stop(void)
{
    if (!s_setup_ap_active) {
        return;
    }

    esp_wifi_stop();
    s_setup_ap_active = false;
    ESP_LOGI(TAG, "setup AP stopped (web UI kept running)");
}

const char *app_network_setup_status_text(void)
{
    return s_setup_status;
}

bool app_network_get_web_ui_url(char *out, size_t out_len)
{
    char ip[40];

    if (out == NULL || out_len == 0) {
        return false;
    }
    out[0] = '\0';
    if (s_httpd == NULL) {
        return false;
    }
    if (!app_network_get_device_ip(ip, sizeof(ip))) {
        return false;
    }

    snprintf(out, out_len, "http://%s", ip);
    return true;
}
