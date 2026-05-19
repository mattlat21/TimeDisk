/**
 * @file app_network.c
 * @brief WiFi STA + SNTP for TimeDisk boot (ESP32-P4 via esp_wifi_remote + esp_hosted).
 */

#include "app_network.h"
#include "app_config.h"
#include "injected/esp_wifi.h"
#include "injected/esp_wifi_default.h"

#include <esp_event.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_sntp.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

static const char *TAG = "app_network";

#define BOOT_TASK_STACK      4096
#define BOOT_TASK_PRIO       5
#define WIFI_CONNECT_TIMEOUT_MS  30000
#define SNTP_SYNC_TIMEOUT_MS     20000
#define SNTP_POLL_INTERVAL_MS    500

#define WIFI_CONNECTED_BIT  BIT0
#define WIFI_FAIL_BIT       BIT1

static EventGroupHandle_t s_wifi_events;
static bool s_stack_ready;
static bool s_boot_task_running;
static volatile bool s_boot_cancel;
/** True while wifi_connect_from_config() is stopping/restarting STA (ignore DISCONNECTED). */
static volatile bool s_wifi_teardown_in_progress;
static app_network_state_t s_state;
static char s_status[64];

static app_network_boot_done_cb_t s_boot_done_cb;
static void *s_boot_done_ctx;

void app_network_set_boot_done_callback(app_network_boot_done_cb_t cb, void *user_data)
{
    s_boot_done_cb = cb;
    s_boot_done_ctx = user_data;
}

static void set_state(app_network_state_t st, const char *status)
{
    s_state = st;
    if (status != NULL) {
        snprintf(s_status, sizeof(s_status), "%s", status);
    }
}

app_network_state_t app_network_get_state(void)
{
    return s_state;
}

const char *app_network_get_status_text(void)
{
    return s_status;
}

bool app_network_time_is_valid(void)
{
    return app_runtime_get()->time_valid;
}

bool app_network_boot_sync_active(void)
{
    return s_boot_task_running;
}

static void notify_boot_done(bool time_ok)
{
    app_runtime_t *rt = app_runtime_get();
    rt->time_valid = time_ok;
    if (s_boot_done_cb != NULL) {
        s_boot_done_cb(time_ok, s_boot_done_ctx);
    }
}

static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    (void)data;
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_boot_cancel || s_wifi_teardown_in_progress) {
            return;
        }
        set_state(APP_NETWORK_STATE_FAILED, "Wi-Fi disconnected");
        xEventGroupSetBits(s_wifi_events, WIFI_FAIL_BIT);
    }
}

static void ip_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *ev = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "got IP: " IPSTR, IP2STR(&ev->ip_info.ip));
        set_state(APP_NETWORK_STATE_GOT_IP, "Connected");
        xEventGroupSetBits(s_wifi_events, WIFI_CONNECTED_BIT);
    }
}

static esp_err_t ensure_stack(void)
{
    if (s_stack_ready) {
        return ESP_OK;
    }

    s_wifi_events = xEventGroupCreate();
    if (s_wifi_events == NULL) {
        return ESP_ERR_NO_MEM;
    }

    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    esp_netif_create_default_wifi_sta();

    wifi_init_config_t wcfg = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&wcfg);
    if (err != ESP_OK) {
        return err;
    }

    err = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                              &wifi_event_handler, NULL, NULL);
    if (err != ESP_OK) {
        return err;
    }
    err = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                              &ip_event_handler, NULL, NULL);
    if (err != ESP_OK) {
        return err;
    }

    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err != ESP_OK) {
        return err;
    }

    s_stack_ready = true;
    ESP_LOGI(TAG, "network stack ready (esp_wifi -> remote)");
    return ESP_OK;
}

esp_err_t app_network_init(void)
{
    return ensure_stack();
}

static bool wifi_already_connected(const char *ssid)
{
    esp_netif_t *sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (sta == NULL) {
        return false;
    }

    esp_netif_ip_info_t ip = {0};
    if (esp_netif_get_ip_info(sta, &ip) != ESP_OK || ip.ip.addr == 0) {
        return false;
    }

    wifi_ap_record_t ap = {0};
    if (esp_wifi_sta_get_ap_info(&ap) != ESP_OK) {
        return false;
    }

    return strncmp((const char *)ap.ssid, ssid, sizeof(ap.ssid)) == 0;
}

static esp_err_t wifi_connect_from_config(void)
{
    const app_config_t *cfg = app_config_get();

    if (app_config_wifi_ssid_missing()) {
        set_state(APP_NETWORK_STATE_FAILED, "No Wi-Fi network");
        return ESP_ERR_INVALID_STATE;
    }

    if (wifi_already_connected(cfg->wifi_ssid)) {
        ESP_LOGI(TAG, "already on %s, skipping reconnect", cfg->wifi_ssid);
        set_state(APP_NETWORK_STATE_GOT_IP, "Connected");
        return ESP_OK;
    }

    xEventGroupClearBits(s_wifi_events, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);

    wifi_config_t wc = {0};
    snprintf((char *)wc.sta.ssid, sizeof(wc.sta.ssid), "%s", cfg->wifi_ssid);
    snprintf((char *)wc.sta.password, sizeof(wc.sta.password), "%s", cfg->wifi_password);

    if (cfg->wifi_password[0] == '\0') {
        wc.sta.threshold.authmode = WIFI_AUTH_OPEN;
    } else {
        wc.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    }

    set_state(APP_NETWORK_STATE_CONNECTING, "Connecting…");

    s_wifi_teardown_in_progress = true;
    esp_wifi_disconnect();
    esp_wifi_stop();
    vTaskDelay(pdMS_TO_TICKS(100));

    esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &wc);
    if (err != ESP_OK) {
        s_wifi_teardown_in_progress = false;
        return err;
    }

    err = esp_wifi_start();
    if (err != ESP_OK) {
        s_wifi_teardown_in_progress = false;
        return err;
    }

    err = esp_wifi_connect();
    if (err != ESP_OK && err != ESP_ERR_WIFI_CONN) {
        s_wifi_teardown_in_progress = false;
        return err;
    }

    s_wifi_teardown_in_progress = false;
    xEventGroupClearBits(s_wifi_events, WIFI_FAIL_BIT);

    EventBits_t bits = xEventGroupWaitBits(s_wifi_events,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdTRUE,
                                           pdFALSE,
                                           pdMS_TO_TICKS(WIFI_CONNECT_TIMEOUT_MS));

    if (bits & WIFI_CONNECTED_BIT) {
        return ESP_OK;
    }

    set_state(APP_NETWORK_STATE_FAILED, "Wi-Fi failed");
    return ESP_FAIL;
}

static void sntp_stop(void)
{
    if (esp_sntp_enabled()) {
        esp_sntp_stop();
    }
}

static esp_err_t sntp_sync_from_config(void)
{
    const app_config_t *cfg = app_config_get();
    const char *server = cfg->ntp_server[0] != '\0' ? cfg->ntp_server : "pool.ntp.org";

    set_state(APP_NETWORK_STATE_SYNCING_TIME, "Syncing time…");
    sntp_stop();

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, server);
    esp_sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
    esp_sntp_init();

    const int max_polls = SNTP_SYNC_TIMEOUT_MS / SNTP_POLL_INTERVAL_MS;
    for (int i = 0; i < max_polls; i++) {
        if (s_boot_cancel) {
            sntp_stop();
            return ESP_ERR_INVALID_STATE;
        }
        if (esp_sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
            time_t now = 0;
            time(&now);
            ESP_LOGI(TAG, "SNTP sync OK, epoch %ld", (long)now);
            set_state(APP_NETWORK_STATE_READY, "Ready");
            return ESP_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(SNTP_POLL_INTERVAL_MS));
    }

    sntp_stop();
    set_state(APP_NETWORK_STATE_FAILED, "Time sync failed");
    return ESP_ERR_TIMEOUT;
}

static void boot_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "boot sync started");

    esp_err_t err = ensure_stack();
    if (err != ESP_OK) {
        set_state(APP_NETWORK_STATE_FAILED, "Network init failed");
        goto done;
    }

    if (s_boot_cancel) {
        goto done;
    }

    err = wifi_connect_from_config();
    if (err != ESP_OK || s_boot_cancel) {
        goto done;
    }

    err = sntp_sync_from_config();
    if (err != ESP_OK || s_boot_cancel) {
        goto done;
    }

    notify_boot_done(true);
    s_boot_task_running = false;
    vTaskDelete(NULL);
    return;

done:
    if (!s_boot_cancel) {
        notify_boot_done(false);
    }
    s_boot_task_running = false;
    vTaskDelete(NULL);
}

void app_network_cancel_boot_sync(void)
{
    s_boot_cancel = true;
}

void app_network_start_boot_sync(void)
{
    if (s_boot_task_running) {
        return;
    }

    s_boot_cancel = false;
    s_boot_task_running = true;
    set_state(APP_NETWORK_STATE_IDLE, "Starting…");

    BaseType_t ok = xTaskCreate(boot_task, "app_net_boot", BOOT_TASK_STACK, NULL, BOOT_TASK_PRIO, NULL);
    if (ok != pdPASS) {
        s_boot_task_running = false;
        set_state(APP_NETWORK_STATE_FAILED, "Task create failed");
        notify_boot_done(false);
    }
}
