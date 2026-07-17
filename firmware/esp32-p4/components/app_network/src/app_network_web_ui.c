/**
 * @file app_network_web_ui.c
 * @brief Multi-page TimeDisk web UI with light/dark theme and JSON API.
 */

#include "app_network_web_ui.h"
#include "app_network_web_ui_internal.h"
#include "app_network.h"
#include "app_config.h"
#include "app_ota.h"
#include "app_time.h"
#include "timezone_catalog.h"

#include <cJSON.h>
#include <dirent.h>
#include <esp_http_server.h>
#include <esp_log.h>
#include <esp_spiffs.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static const char *TAG = "web_ui";

#define WEB_POST_MAX 2048

static app_network_web_ui_timer_ops_t s_timer_ops;
static bool s_timer_ops_set;
static app_network_web_ui_mode_ops_t s_mode_ops;
static bool s_mode_ops_set;

void app_network_web_ui_set_timer_ops(const app_network_web_ui_timer_ops_t *ops)
{
    if (ops == NULL) {
        return;
    }
    s_timer_ops = *ops;
    s_timer_ops_set = true;
}

void app_network_web_ui_set_mode_ops(const app_network_web_ui_mode_ops_t *ops)
{
    if (ops == NULL) {
        return;
    }
    s_mode_ops = *ops;
    s_mode_ops_set = true;
}

static esp_err_t read_body(httpd_req_t *req, char *body, size_t body_len)
{
    if (body_len == 0) {
        return ESP_ERR_INVALID_SIZE;
    }
    body[0] = '\0';
    if (req->content_len <= 0) {
        return ESP_OK;
    }
    if ((size_t)req->content_len >= body_len) {
        return ESP_ERR_INVALID_SIZE;
    }
    int received = httpd_req_recv(req, body, (size_t)req->content_len);
    if (received <= 0) {
        return ESP_FAIL;
    }
    body[received] = '\0';
    return ESP_OK;
}

static esp_err_t send_json(httpd_req_t *req, char *json)
{
    if (json == NULL) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "JSON error");
        return ESP_FAIL;
    }
    httpd_resp_set_type(req, "application/json");
    esp_err_t err = httpd_resp_sendstr(req, json);
    free(json);
    return err;
}

static esp_err_t send_json_ok(httpd_req_t *req)
{
    return send_json(req, strdup("{\"ok\":true}"));
}

static bool apply_config_field(app_config_t *cfg, const char *key, const cJSON *val)
{
    if (key == NULL || val == NULL) {
        return false;
    }

    if (strcmp(key, "mqtt_enabled") == 0 && cJSON_IsBool(val)) {
        cfg->mqtt_enabled = cJSON_IsTrue(val);
        return true;
    }
    if (strcmp(key, "mqtt_host") == 0 && cJSON_IsString(val)) {
        snprintf(cfg->mqtt_host, sizeof(cfg->mqtt_host), "%s", val->valuestring);
        return true;
    }
    if (strcmp(key, "mqtt_port") == 0 && cJSON_IsNumber(val)) {
        cfg->mqtt_port = (uint16_t)val->valuedouble;
        return true;
    }
    if (strcmp(key, "mqtt_username") == 0 && cJSON_IsString(val)) {
        snprintf(cfg->mqtt_username, sizeof(cfg->mqtt_username), "%s", val->valuestring);
        return true;
    }
    if (strcmp(key, "mqtt_password") == 0 && cJSON_IsString(val)) {
        snprintf(cfg->mqtt_password, sizeof(cfg->mqtt_password), "%s", val->valuestring);
        return true;
    }
    if (strcmp(key, "ntp_server") == 0 && cJSON_IsString(val)) {
        snprintf(cfg->ntp_server, sizeof(cfg->ntp_server), "%s", val->valuestring);
        return true;
    }
    if (strcmp(key, "timezone_id") == 0 && cJSON_IsString(val)) {
        snprintf(cfg->timezone_id, sizeof(cfg->timezone_id), "%s", val->valuestring);
        cfg->timezone_set = cfg->timezone_id[0] != '\0';
        return true;
    }
    if (strcmp(key, "timeout_splash_sec") == 0 && cJSON_IsNumber(val)) {
        cfg->timeout_splash_sec = (uint32_t)val->valuedouble;
        return true;
    }
    if (strcmp(key, "timeout_tod_dim_sec") == 0 && cJSON_IsNumber(val)) {
        cfg->timeout_tod_dim_sec = (uint32_t)val->valuedouble;
        return true;
    }
    if (strcmp(key, "timeout_tod_menu_sec") == 0 && cJSON_IsNumber(val)) {
        cfg->timeout_tod_menu_sec = (uint32_t)val->valuedouble;
        return true;
    }
    if (strcmp(key, "timeout_aa_sec") == 0 && cJSON_IsNumber(val)) {
        cfg->timeout_aa_sec = (uint32_t)val->valuedouble;
        return true;
    }
    if (strcmp(key, "timeout_main_menu_sec") == 0 && cJSON_IsNumber(val)) {
        cfg->timeout_main_menu_sec = (uint32_t)val->valuedouble;
        return true;
    }
    if (strcmp(key, "timeout_timer_dim_sec") == 0 && cJSON_IsNumber(val)) {
        cfg->timeout_timer_dim_sec = (uint32_t)val->valuedouble;
        return true;
    }
    if (strcmp(key, "timeout_timer_done_sec") == 0 && cJSON_IsNumber(val)) {
        cfg->timeout_timer_done_sec = (uint32_t)val->valuedouble;
        return true;
    }
    if (strcmp(key, "backlight_bright_pct") == 0 && cJSON_IsNumber(val)) {
        uint8_t pct = (uint8_t)val->valuedouble;
        cfg->backlight_bright_pct = pct > 100 ? 100 : pct;
        return true;
    }
    if (strcmp(key, "backlight_dim_pct") == 0 && cJSON_IsNumber(val)) {
        uint8_t pct = (uint8_t)val->valuedouble;
        cfg->backlight_dim_pct = pct > 100 ? 100 : pct;
        return true;
    }
    if (strcmp(key, "ui_primary_color") == 0 && cJSON_IsNumber(val)) {
        cfg->ui_primary_color = (uint32_t)val->valuedouble;
        cfg->theme_set = true;
        return true;
    }
    if (strcmp(key, "ui_secondary_color") == 0 && cJSON_IsNumber(val)) {
        cfg->ui_secondary_color = (uint32_t)val->valuedouble;
        cfg->theme_set = true;
        return true;
    }
    if (strcmp(key, "timer_duration_sec") == 0 && cJSON_IsNumber(val)) {
        cfg->timer_duration_sec = (uint32_t)val->valuedouble;
        return true;
    }
    if (strcmp(key, "timer_style_id") == 0 && cJSON_IsNumber(val)) {
        cfg->timer_style_id = (uint8_t)val->valuedouble;
        return true;
    }
    if (strcmp(key, "wind_down_sec") == 0 && cJSON_IsNumber(val)) {
        cfg->wind_down_sec = (uint32_t)val->valuedouble;
        return true;
    }
    if (strcmp(key, "sleep_sec") == 0 && cJSON_IsNumber(val)) {
        cfg->sleep_sec = (uint32_t)val->valuedouble;
        return true;
    }
    if (strcmp(key, "rest_sec") == 0 && cJSON_IsNumber(val)) {
        cfg->rest_sec = (uint32_t)val->valuedouble;
        return true;
    }
    if (strcmp(key, "aa_methods") == 0 && cJSON_IsNumber(val)) {
        cfg->aa_methods = (uint8_t)val->valuedouble & 0x03;
        return true;
    }
    if (strcmp(key, "aa_pin") == 0 && cJSON_IsString(val)) {
        snprintf(cfg->aa_pin, sizeof(cfg->aa_pin), "%s", val->valuestring);
        return true;
    }
    return false;
}

static void save_config_changes(const char *key)
{
    app_config_t *cfg = app_config_get();

    if (strcmp(key, "mqtt_host") == 0 || strcmp(key, "mqtt_port") == 0 ||
        strcmp(key, "mqtt_username") == 0 || strcmp(key, "mqtt_password") == 0 ||
        strcmp(key, "mqtt_enabled") == 0) {
        app_config_save_mqtt();
        return;
    }
    if (strncmp(key, "timeout_", 8) == 0) {
        app_config_save_timeouts();
        return;
    }
    if (strcmp(key, "backlight_bright_pct") == 0 || strcmp(key, "backlight_dim_pct") == 0) {
        app_config_save_display();
        return;
    }
    if (strcmp(key, "ui_primary_color") == 0 || strcmp(key, "ui_secondary_color") == 0) {
        app_config_save_theme();
        return;
    }
    if (strcmp(key, "timer_duration_sec") == 0 || strcmp(key, "timer_style_id") == 0) {
        app_config_save_timer();
        return;
    }
    if (strcmp(key, "wind_down_sec") == 0 || strcmp(key, "sleep_sec") == 0 ||
        strcmp(key, "rest_sec") == 0) {
        app_config_save_schedule();
        return;
    }
    if (strcmp(key, "aa_methods") == 0 || strcmp(key, "aa_pin") == 0) {
        app_config_save_aa();
        return;
    }
    if (strcmp(key, "ntp_server") == 0) {
        app_config_save_network();
        return;
    }
    if (strcmp(key, "timezone_id") == 0) {
        app_config_save_timezone();
        app_time_apply_timezone_id(cfg->timezone_id);
    }
}

static esp_err_t api_status_get(httpd_req_t *req)
{
    return send_json(req, app_network_web_ui_build_rest_status_json());
}

static esp_err_t api_config_get(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return ESP_FAIL;
    }
    app_network_web_ui_json_add_config(root, app_config_get());
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return send_json(req, json);
}

static esp_err_t api_config_post(httpd_req_t *req)
{
    char body[WEB_POST_MAX];
    if (read_body(req, body, sizeof(body)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad request");
        return ESP_FAIL;
    }

    cJSON *root = cJSON_Parse(body);
    if (root == NULL || !cJSON_IsObject(root)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    app_config_t *cfg = app_config_get();
    for (const cJSON *item = root->child; item != NULL; item = item->next) {
        if (item->string == NULL) {
            continue;
        }
        if (apply_config_field(cfg, item->string, item)) {
            save_config_changes(item->string);
        }
    }

    cJSON_Delete(root);
    return send_json_ok(req);
}

static esp_err_t api_timezones_get(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateArray();
    if (root == NULL) {
        return ESP_FAIL;
    }

    for (size_t ci = 0; ci < timezone_catalog_country_count(); ci++) {
        cJSON *country = cJSON_CreateObject();
        if (country == NULL) {
            continue;
        }
        cJSON_AddStringToObject(country, "country", timezone_catalog_country_label(ci));
        cJSON *locs = cJSON_CreateArray();
        for (size_t li = 0; li < timezone_catalog_location_count(ci); li++) {
            cJSON *loc = cJSON_CreateObject();
            if (loc == NULL) {
                continue;
            }
            cJSON_AddStringToObject(loc, "label", timezone_catalog_location_label(ci, li));
            cJSON_AddStringToObject(loc, "id", timezone_catalog_timezone_id(ci, li));
            cJSON_AddItemToArray(locs, loc);
        }
        cJSON_AddItemToObject(country, "locations", locs);
        cJSON_AddItemToArray(root, country);
    }

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return send_json(req, json);
}

static esp_err_t api_wifi_get(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return ESP_FAIL;
    }
    cJSON *wifi = cJSON_CreateArray();
    const app_config_t *cfg = app_config_get();
    for (int i = 0; i < (int)cfg->wifi_network_count; i++) {
        cJSON *net = cJSON_CreateObject();
        if (net == NULL) {
            continue;
        }
        cJSON_AddNumberToObject(net, "index", i);
        cJSON_AddStringToObject(net, "ssid", cfg->wifi_networks[i].ssid);
        cJSON_AddBoolToObject(net, "password_set", cfg->wifi_networks[i].password_set);
        cJSON_AddItemToArray(wifi, net);
    }
    cJSON_AddItemToObject(root, "networks", wifi);
    cJSON_AddNumberToObject(root, "count", app_config_get()->wifi_network_count);
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return send_json(req, json);
}

static esp_err_t api_wifi_add_post(httpd_req_t *req)
{
    char body[WEB_POST_MAX];
    if (read_body(req, body, sizeof(body)) != ESP_OK) {
        return ESP_FAIL;
    }

    cJSON *root = cJSON_Parse(body);
    if (root == NULL) {
        return ESP_FAIL;
    }

    const cJSON *ssid = cJSON_GetObjectItem(root, "ssid");
    const cJSON *password = cJSON_GetObjectItem(root, "password");
    if (cJSON_IsString(ssid) && ssid->valuestring[0] != '\0') {
        int index = app_config_wifi_network_count();
        const char *pw = cJSON_IsString(password) ? password->valuestring : "";
        app_config_wifi_network_set(index, ssid->valuestring, pw, pw[0] != '\0');
        app_config_save_network();
    }

    cJSON_Delete(root);
    return send_json_ok(req);
}

static esp_err_t api_wifi_delete_post(httpd_req_t *req)
{
    char body[WEB_POST_MAX];
    if (read_body(req, body, sizeof(body)) != ESP_OK) {
        return ESP_FAIL;
    }

    cJSON *root = cJSON_Parse(body);
    if (root == NULL) {
        return ESP_FAIL;
    }

    const cJSON *index = cJSON_GetObjectItem(root, "index");
    if (cJSON_IsNumber(index)) {
        app_config_wifi_network_delete((int)index->valuedouble);
        app_config_save_network();
    }

    cJSON_Delete(root);
    return send_json_ok(req);
}

static esp_err_t api_wifi_edit_post(httpd_req_t *req)
{
    char body[WEB_POST_MAX];
    if (read_body(req, body, sizeof(body)) != ESP_OK) {
        return ESP_FAIL;
    }

    cJSON *root = cJSON_Parse(body);
    if (root == NULL) {
        return ESP_FAIL;
    }

    const cJSON *index_j = cJSON_GetObjectItem(root, "index");
    const cJSON *ssid_j = cJSON_GetObjectItem(root, "ssid");
    const cJSON *password_j = cJSON_GetObjectItem(root, "password");
    int index = cJSON_IsNumber(index_j) ? (int)index_j->valuedouble : -1;
    const app_wifi_network_t *existing = app_config_wifi_network_get(index);

    if (existing != NULL && cJSON_IsString(ssid_j) && ssid_j->valuestring[0] != '\0') {
        const char *pw = existing->password;
        bool pw_set = existing->password_set;
        if (cJSON_IsString(password_j) && password_j->valuestring[0] != '\0') {
            pw = password_j->valuestring;
            pw_set = true;
        }
        app_config_wifi_network_set(index, ssid_j->valuestring, pw, pw_set);
        app_config_save_network();
    }

    cJSON_Delete(root);
    return send_json_ok(req);
}

static esp_err_t api_wifi_reorder_post(httpd_req_t *req)
{
    char body[WEB_POST_MAX];
    if (read_body(req, body, sizeof(body)) != ESP_OK) {
        return ESP_FAIL;
    }

    cJSON *root = cJSON_Parse(body);
    if (root == NULL) {
        return ESP_FAIL;
    }

    const cJSON *from_j = cJSON_GetObjectItem(root, "from");
    const cJSON *to_j = cJSON_GetObjectItem(root, "to");
    if (cJSON_IsNumber(from_j) && cJSON_IsNumber(to_j)) {
        app_config_t *cfg = app_config_get();
        int from = (int)from_j->valuedouble;
        int to = (int)to_j->valuedouble;
        int count = (int)cfg->wifi_network_count;
        if (from >= 0 && to >= 0 && from < count && to < count && from != to) {
            app_wifi_network_t tmp = cfg->wifi_networks[from];
            if (from < to) {
                for (int i = from; i < to; i++) {
                    cfg->wifi_networks[i] = cfg->wifi_networks[i + 1];
                }
                cfg->wifi_networks[to] = tmp;
            } else {
                for (int i = from; i > to; i--) {
                    cfg->wifi_networks[i] = cfg->wifi_networks[i - 1];
                }
                cfg->wifi_networks[to] = tmp;
            }
            app_config_save_network();
        }
    }

    cJSON_Delete(root);
    return send_json_ok(req);
}

static esp_err_t api_wifi_connect_post(httpd_req_t *req)
{
    (void)req;
    app_network_reconnect_sta();
    return send_json_ok(req);
}

static const char *schedule_action_name(uint8_t action)
{
    switch ((app_schedule_action_t)action) {
    case APP_SCHEDULE_ACTION_START_SLEEP:
        return "start_sleep";
    case APP_SCHEDULE_ACTION_START_REST:
        return "start_rest";
    case APP_SCHEDULE_ACTION_START_WIND_DOWN:
        return "start_wind_down";
    case APP_SCHEDULE_ACTION_WAKE:
    default:
        return "wake";
    }
}

static bool schedule_action_from_json(const cJSON *val, uint8_t *action_out)
{
    if (action_out == NULL) {
        return false;
    }
    if (cJSON_IsNumber(val)) {
        const int n = (int)val->valuedouble;
        if (n >= APP_SCHEDULE_ACTION_WAKE && n <= APP_SCHEDULE_ACTION_START_WIND_DOWN) {
            *action_out = (uint8_t)n;
            return true;
        }
        return false;
    }
    if (!cJSON_IsString(val)) {
        return false;
    }
    if (strcmp(val->valuestring, "wake") == 0) {
        *action_out = APP_SCHEDULE_ACTION_WAKE;
        return true;
    }
    if (strcmp(val->valuestring, "start_sleep") == 0) {
        *action_out = APP_SCHEDULE_ACTION_START_SLEEP;
        return true;
    }
    if (strcmp(val->valuestring, "start_rest") == 0) {
        *action_out = APP_SCHEDULE_ACTION_START_REST;
        return true;
    }
    if (strcmp(val->valuestring, "start_wind_down") == 0) {
        *action_out = APP_SCHEDULE_ACTION_START_WIND_DOWN;
        return true;
    }
    return false;
}

static bool schedule_event_from_json(const cJSON *root, app_schedule_event_t *event_out)
{
    if (root == NULL || event_out == NULL || !cJSON_IsObject(root)) {
        return false;
    }

    app_schedule_event_t ev = {0};
    const cJSON *time_min = cJSON_GetObjectItem(root, "time_min");
    const cJSON *action = cJSON_GetObjectItem(root, "action");
    const cJSON *enabled = cJSON_GetObjectItem(root, "enabled");
    const cJSON *duration_sec = cJSON_GetObjectItem(root, "duration_sec");

    if (!cJSON_IsNumber(time_min) || time_min->valuedouble < 0 || time_min->valuedouble >= 24 * 60) {
        return false;
    }
    if (!schedule_action_from_json(action, &ev.action)) {
        return false;
    }

    ev.time_min = (uint16_t)time_min->valuedouble;
    ev.enabled = !cJSON_IsBool(enabled) || cJSON_IsTrue(enabled);
    ev.duration_sec = cJSON_IsNumber(duration_sec) ? (uint32_t)duration_sec->valuedouble : 0;
    if (ev.action == APP_SCHEDULE_ACTION_WAKE) {
        ev.duration_sec = 0;
    }
    *event_out = ev;
    return true;
}

static esp_err_t api_schedule_events_get(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return ESP_FAIL;
    }

    cJSON *events = cJSON_CreateArray();
    const app_config_t *cfg = app_config_get();
    for (int i = 0; i < (int)cfg->schedule_event_count; i++) {
        const app_schedule_event_t *ev = &cfg->schedule_events[i];
        cJSON *item = cJSON_CreateObject();
        if (item == NULL) {
            continue;
        }
        cJSON_AddNumberToObject(item, "index", i);
        cJSON_AddNumberToObject(item, "time_min", ev->time_min);
        cJSON_AddStringToObject(item, "action", schedule_action_name(ev->action));
        cJSON_AddBoolToObject(item, "enabled", ev->enabled);
        cJSON_AddNumberToObject(item, "duration_sec", ev->duration_sec);
        cJSON_AddItemToArray(events, item);
    }
    cJSON_AddItemToObject(root, "events", events);
    cJSON_AddNumberToObject(root, "count", cfg->schedule_event_count);
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return send_json(req, json);
}

static esp_err_t api_schedule_events_add_post(httpd_req_t *req)
{
    char body[WEB_POST_MAX];
    if (read_body(req, body, sizeof(body)) != ESP_OK) {
        return ESP_FAIL;
    }

    cJSON *root = cJSON_Parse(body);
    if (root == NULL) {
        return ESP_FAIL;
    }

    app_schedule_event_t ev;
    if (schedule_event_from_json(root, &ev)) {
        int index = app_config_schedule_event_count();
        if (app_config_schedule_event_set(index, &ev) == ESP_OK) {
            app_config_save_schedule();
        }
    }

    cJSON_Delete(root);
    return send_json_ok(req);
}

static esp_err_t api_schedule_events_edit_post(httpd_req_t *req)
{
    char body[WEB_POST_MAX];
    if (read_body(req, body, sizeof(body)) != ESP_OK) {
        return ESP_FAIL;
    }

    cJSON *root = cJSON_Parse(body);
    if (root == NULL) {
        return ESP_FAIL;
    }

    const cJSON *index_j = cJSON_GetObjectItem(root, "index");
    app_schedule_event_t ev;
    if (cJSON_IsNumber(index_j) && schedule_event_from_json(root, &ev)) {
        app_config_schedule_event_set((int)index_j->valuedouble, &ev);
        app_config_save_schedule();
    }

    cJSON_Delete(root);
    return send_json_ok(req);
}

static esp_err_t api_schedule_events_delete_post(httpd_req_t *req)
{
    char body[WEB_POST_MAX];
    if (read_body(req, body, sizeof(body)) != ESP_OK) {
        return ESP_FAIL;
    }

    cJSON *root = cJSON_Parse(body);
    if (root == NULL) {
        return ESP_FAIL;
    }

    const cJSON *index = cJSON_GetObjectItem(root, "index");
    if (cJSON_IsNumber(index)) {
        app_config_schedule_event_delete((int)index->valuedouble);
        app_config_save_schedule();
    }

    cJSON_Delete(root);
    return send_json_ok(req);
}

static const char *schedule_mode_name(app_mode_t mode)
{
    switch (mode) {
    case APP_MODE_WIND_DOWN:
        return "wind_down";
    case APP_MODE_SLEEP:
        return "sleep";
    case APP_MODE_REST:
        return "rest";
    case APP_MODE_WAKE:
    default:
        return "wake";
    }
}

static bool schedule_mode_bit_from_name(const char *name, uint8_t *bit_out)
{
    if (name == NULL || bit_out == NULL) {
        return false;
    }
    if (strcmp(name, "wake") == 0) {
        *bit_out = (uint8_t)APP_MODE_BIT(APP_MODE_WAKE);
        return true;
    }
    if (strcmp(name, "wind_down") == 0) {
        *bit_out = (uint8_t)APP_MODE_BIT(APP_MODE_WIND_DOWN);
        return true;
    }
    if (strcmp(name, "sleep") == 0) {
        *bit_out = (uint8_t)APP_MODE_BIT(APP_MODE_SLEEP);
        return true;
    }
    if (strcmp(name, "rest") == 0) {
        *bit_out = (uint8_t)APP_MODE_BIT(APP_MODE_REST);
        return true;
    }
    return false;
}

static uint8_t schedule_show_modes_from_json(const cJSON *val)
{
    if (cJSON_IsNumber(val)) {
        return (uint8_t)((unsigned)val->valuedouble & (unsigned)APP_MODE_BIT_ALL);
    }
    if (!cJSON_IsArray(val)) {
        return 0;
    }

    uint8_t bits = 0;
    const cJSON *item = NULL;
    cJSON_ArrayForEach(item, val) {
        uint8_t bit = 0;
        if (cJSON_IsString(item) && schedule_mode_bit_from_name(item->valuestring, &bit)) {
            bits |= bit;
        } else if (cJSON_IsNumber(item)) {
            const int mode = (int)item->valuedouble;
            if (mode >= APP_MODE_WAKE && mode <= APP_MODE_REST) {
                bits |= (uint8_t)APP_MODE_BIT(mode);
            }
        }
    }
    return bits;
}

static bool scheduled_button_from_json(const cJSON *root, app_scheduled_button_t *button_out)
{
    if (root == NULL || button_out == NULL || !cJSON_IsObject(root)) {
        return false;
    }

    app_scheduled_button_t btn = {0};
    const cJSON *start_min = cJSON_GetObjectItem(root, "start_min");
    const cJSON *end_min = cJSON_GetObjectItem(root, "end_min");
    const cJSON *action = cJSON_GetObjectItem(root, "action");
    const cJSON *enabled = cJSON_GetObjectItem(root, "enabled");
    const cJSON *show_modes = cJSON_GetObjectItem(root, "show_modes");

    if (!cJSON_IsNumber(start_min) || start_min->valuedouble < 0 || start_min->valuedouble >= 24 * 60) {
        return false;
    }
    if (!cJSON_IsNumber(end_min) || end_min->valuedouble < 0 || end_min->valuedouble >= 24 * 60) {
        return false;
    }
    if (!schedule_action_from_json(action, &btn.action)) {
        return false;
    }

    btn.start_min = (uint16_t)start_min->valuedouble;
    btn.end_min = (uint16_t)end_min->valuedouble;
    btn.enabled = !cJSON_IsBool(enabled) || cJSON_IsTrue(enabled);
    btn.show_modes = schedule_show_modes_from_json(show_modes);
    if (btn.show_modes == 0) {
        btn.show_modes = (uint8_t)APP_MODE_BIT_ALL;
    }
    *button_out = btn;
    return true;
}

static esp_err_t api_schedule_buttons_get(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return ESP_FAIL;
    }

    cJSON *buttons = cJSON_CreateArray();
    const app_config_t *cfg = app_config_get();
    for (int i = 0; i < (int)cfg->scheduled_button_count; i++) {
        const app_scheduled_button_t *btn = &cfg->scheduled_buttons[i];
        cJSON *item = cJSON_CreateObject();
        if (item == NULL) {
            continue;
        }
        cJSON_AddNumberToObject(item, "index", i);
        cJSON_AddNumberToObject(item, "start_min", btn->start_min);
        cJSON_AddNumberToObject(item, "end_min", btn->end_min);
        cJSON_AddStringToObject(item, "action", schedule_action_name(btn->action));
        cJSON_AddBoolToObject(item, "enabled", btn->enabled);

        cJSON *modes = cJSON_CreateArray();
        for (int mode = APP_MODE_WAKE; mode <= APP_MODE_REST; mode++) {
            if ((btn->show_modes & (uint8_t)APP_MODE_BIT(mode)) != 0) {
                cJSON_AddItemToArray(modes, cJSON_CreateString(schedule_mode_name((app_mode_t)mode)));
            }
        }
        cJSON_AddItemToObject(item, "show_modes", modes);
        cJSON_AddNumberToObject(item, "show_modes_mask", btn->show_modes);
        cJSON_AddItemToArray(buttons, item);
    }
    cJSON_AddItemToObject(root, "buttons", buttons);
    cJSON_AddNumberToObject(root, "count", cfg->scheduled_button_count);
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return send_json(req, json);
}

static esp_err_t api_schedule_buttons_add_post(httpd_req_t *req)
{
    char body[WEB_POST_MAX];
    if (read_body(req, body, sizeof(body)) != ESP_OK) {
        return ESP_FAIL;
    }

    cJSON *root = cJSON_Parse(body);
    if (root == NULL) {
        return ESP_FAIL;
    }

    app_scheduled_button_t btn;
    if (scheduled_button_from_json(root, &btn)) {
        int index = app_config_scheduled_button_count();
        if (app_config_scheduled_button_set(index, &btn) == ESP_OK) {
            app_config_save_schedule();
        }
    }

    cJSON_Delete(root);
    return send_json_ok(req);
}

static esp_err_t api_schedule_buttons_edit_post(httpd_req_t *req)
{
    char body[WEB_POST_MAX];
    if (read_body(req, body, sizeof(body)) != ESP_OK) {
        return ESP_FAIL;
    }

    cJSON *root = cJSON_Parse(body);
    if (root == NULL) {
        return ESP_FAIL;
    }

    const cJSON *index_j = cJSON_GetObjectItem(root, "index");
    app_scheduled_button_t btn;
    if (cJSON_IsNumber(index_j) && scheduled_button_from_json(root, &btn)) {
        app_config_scheduled_button_set((int)index_j->valuedouble, &btn);
        app_config_save_schedule();
    }

    cJSON_Delete(root);
    return send_json_ok(req);
}

static esp_err_t api_schedule_buttons_delete_post(httpd_req_t *req)
{
    char body[WEB_POST_MAX];
    if (read_body(req, body, sizeof(body)) != ESP_OK) {
        return ESP_FAIL;
    }

    cJSON *root = cJSON_Parse(body);
    if (root == NULL) {
        return ESP_FAIL;
    }

    const cJSON *index = cJSON_GetObjectItem(root, "index");
    if (cJSON_IsNumber(index)) {
        app_config_scheduled_button_delete((int)index->valuedouble);
        app_config_save_schedule();
    }

    cJSON_Delete(root);
    return send_json_ok(req);
}

static void reboot_task(void *arg)
{
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();
}

static esp_err_t api_reboot_post(httpd_req_t *req)
{
    (void)req;
    esp_err_t err = send_json_ok(req);
    if (err == ESP_OK) {
        if (xTaskCreate(reboot_task, "web_reboot", 2048, NULL, 1, NULL) != pdPASS) {
            ESP_LOGE(TAG, "reboot task create failed");
        }
    }
    return err;
}

static const char *update_state_name(app_update_state_t state)
{
    switch (state) {
    case APP_UPDATE_STATE_RUNNING:
        return "running";
    case APP_UPDATE_STATE_SUCCESS:
        return "success";
    case APP_UPDATE_STATE_FAILED:
        return "failed";
    case APP_UPDATE_STATE_IDLE:
    default:
        return "idle";
    }
}

static bool update_wifi_ready(void)
{
    const app_network_state_t net = app_network_get_state();
    return net == APP_NETWORK_STATE_READY || net == APP_NETWORK_STATE_GOT_IP;
}

static esp_err_t api_update_get(httpd_req_t *req)
{
    (void)req;
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return ESP_FAIL;
    }

    cJSON_AddStringToObject(root, "version", app_update_get_version());
    cJSON_AddStringToObject(root, "default_url", APP_UPDATE_DEFAULT_FIRMWARE_URL);
    cJSON_AddBoolToObject(root, "wifi_ready", update_wifi_ready());
    cJSON_AddBoolToObject(root, "active", app_update_active());
    cJSON_AddStringToObject(root, "state", update_state_name(app_update_get_state()));
    cJSON_AddNumberToObject(root, "percent", app_update_get_progress_percent());
    cJSON_AddStringToObject(root, "status", app_update_get_progress_status());
    cJSON_AddStringToObject(root, "message", app_update_get_last_message());

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return send_json(req, json);
}

static esp_err_t api_update_start_post(httpd_req_t *req)
{
    if (app_update_active()) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Update already running");
        return ESP_FAIL;
    }
    if (!update_wifi_ready()) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Connect to Wi-Fi first");
        return ESP_FAIL;
    }

    char body[WEB_POST_MAX];
    esp_err_t err = read_body(req, body, sizeof(body));
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad body");
        return err;
    }

    char url[APP_UPDATE_URL_MAX];
    snprintf(url, sizeof(url), "%s", APP_UPDATE_DEFAULT_FIRMWARE_URL);
    if (body[0] != '\0') {
        cJSON *root = cJSON_Parse(body);
        if (root != NULL) {
            const cJSON *url_j = cJSON_GetObjectItem(root, "url");
            if (cJSON_IsString(url_j) && url_j->valuestring[0] != '\0') {
                snprintf(url, sizeof(url), "%s", url_j->valuestring);
            }
            cJSON_Delete(root);
        }
    }

    err = app_update_start(url, NULL, NULL, NULL);
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Could not start update");
        return ESP_FAIL;
    }

    return send_json_ok(req);
}

static esp_err_t api_timer_start_post(httpd_req_t *req)
{
    if (!s_timer_ops_set || s_timer_ops.timer_start == NULL) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Timer not ready");
        return ESP_FAIL;
    }

    char body[WEB_POST_MAX];
    esp_err_t err = read_body(req, body, sizeof(body));
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad body");
        return err;
    }

    uint32_t duration_sec = 0;
    uint8_t style_id = app_config_get()->timer_style_id;
    if (body[0] != '\0') {
        cJSON *root = cJSON_Parse(body);
        if (root != NULL) {
            const cJSON *dur = cJSON_GetObjectItem(root, "duration_sec");
            if (cJSON_IsNumber(dur)) {
                duration_sec = (uint32_t)dur->valuedouble;
            }
            const cJSON *style = cJSON_GetObjectItem(root, "style_id");
            if (cJSON_IsNumber(style)) {
                style_id = (uint8_t)style->valuedouble;
            }
            cJSON_Delete(root);
        }
    }

    s_timer_ops.timer_start(duration_sec, style_id);
    return send_json_ok(req);
}

static esp_err_t api_timer_cancel_post(httpd_req_t *req)
{
    (void)req;
    if (!s_timer_ops_set || s_timer_ops.timer_cancel == NULL) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Timer not ready");
        return ESP_FAIL;
    }
    s_timer_ops.timer_cancel();
    return send_json_ok(req);
}

static esp_err_t api_mode_set_post(httpd_req_t *req)
{
    if (!s_mode_ops_set || s_mode_ops.mode_set == NULL) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Mode control not ready");
        return ESP_FAIL;
    }

    char body[WEB_POST_MAX];
    esp_err_t err = read_body(req, body, sizeof(body));
    if (err != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad body");
        return err;
    }

    cJSON *root = cJSON_Parse(body);
    if (root == NULL) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    const cJSON *action_j = cJSON_GetObjectItem(root, "action");
    uint8_t action = APP_SCHEDULE_ACTION_WAKE;
    if (!schedule_action_from_json(action_j, &action)) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid action");
        return ESP_FAIL;
    }

    uint32_t duration_sec = 0;
    if (action != APP_SCHEDULE_ACTION_WAKE) {
        const cJSON *duration_j = cJSON_GetObjectItem(root, "duration_sec");
        if (action == APP_SCHEDULE_ACTION_START_WIND_DOWN) {
            duration_sec = cJSON_IsNumber(duration_j) ? (uint32_t)duration_j->valuedouble : 0;
        } else {
            duration_sec = cJSON_IsNumber(duration_j) ? (uint32_t)duration_j->valuedouble : 86400;
        }
    }

    cJSON_Delete(root);
    s_mode_ops.mode_set(action, duration_sec);
    return send_json_ok(req);
}

static esp_err_t api_images_get(httpd_req_t *req)
{
    cJSON *root = cJSON_CreateArray();
    if (root == NULL) {
        return ESP_FAIL;
    }

    DIR *dir = opendir("/spiffs");
    if (dir != NULL) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_name[0] == '.') {
                continue;
            }
            char path[320];
            snprintf(path, sizeof(path), "/spiffs/%s", ent->d_name);
            struct stat st;
            if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) {
                continue;
            }
            cJSON *item = cJSON_CreateObject();
            if (item == NULL) {
                continue;
            }
            cJSON_AddStringToObject(item, "name", ent->d_name);
            cJSON_AddNumberToObject(item, "size_bytes", (double)st.st_size);
            cJSON_AddItemToArray(root, item);
        }
        closedir(dir);
    }

    size_t total = 0;
    size_t used = 0;
    cJSON *meta = cJSON_CreateObject();
    if (meta != NULL) {
        if (esp_spiffs_info("storage", &total, &used) == ESP_OK) {
            cJSON_AddNumberToObject(meta, "spiffs_total", (double)total);
            cJSON_AddNumberToObject(meta, "spiffs_used", (double)used);
        }
        cJSON_AddItemToObject(meta, "files", root);
        char *json = cJSON_PrintUnformatted(meta);
        cJSON_Delete(meta);
        return send_json(req, json);
    }

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return send_json(req, json);
}

static int image_filename_valid(const char *name)
{
    if (name == NULL || name[0] == '\0') {
        return 0;
    }

    size_t len = strlen(name);
    if (len < 5 || len > 64 || strcmp(name + len - 4, ".bin") != 0) {
        return 0;
    }

    if (strstr(name, "..") != NULL) {
        return 0;
    }

    for (size_t i = 0; i < len; i++) {
        const char c = name[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
            c == '.' || c == '_' || c == '-') {
            continue;
        }
        return 0;
    }

    return 1;
}

static esp_err_t api_images_file_get(httpd_req_t *req)
{
    char name[128];
    size_t qlen = httpd_req_get_url_query_len(req);

    if (qlen == 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing name");
        return ESP_FAIL;
    }

    char query[256];
    if (qlen + 1 > sizeof(query)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad query");
        return ESP_FAIL;
    }
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad query");
        return ESP_FAIL;
    }

    if (httpd_query_key_value(query, "name", name, sizeof(name)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing name");
        return ESP_FAIL;
    }

    if (!image_filename_valid(name)) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not found");
        return ESP_FAIL;
    }

    char path[320];
    snprintf(path, sizeof(path), "/spiffs/%s", name);

    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not found");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/octet-stream");
    httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=3600");

    char buf[4096];
    esp_err_t err = ESP_OK;

    while (true) {
        size_t n = fread(buf, 1, sizeof(buf), fp);
        if (n > 0) {
            if (httpd_resp_send_chunk(req, buf, n) != ESP_OK) {
                err = ESP_FAIL;
                break;
            }
        }
        if (n < sizeof(buf)) {
            if (ferror(fp)) {
                err = ESP_FAIL;
            }
            break;
        }
    }

    fclose(fp);
    httpd_resp_send_chunk(req, NULL, 0);
    return err;
}

static esp_err_t index_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html; charset=utf-8");

#include "app_network_web_ui_page.inc"

    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}

static const char FAVICON_SVG[] =
    "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 32 32\" fill=\"none\">"
    "<circle cx=\"16\" cy=\"16\" r=\"16\" fill=\"#7c3aed\"/>"
    "<circle cx=\"16\" cy=\"16\" r=\"12.5\" stroke=\"#fff\" stroke-opacity=\".22\" stroke-width=\"1.25\"/>"
    "<path d=\"M16 16V8.5\" stroke=\"#fff\" stroke-width=\"2.25\" stroke-linecap=\"round\"/>"
    "<path d=\"M16 16L22.5 19.5\" stroke=\"#fff\" stroke-width=\"2.25\" stroke-linecap=\"round\"/>"
    "<path d=\"M16 16L10 11.5\" stroke=\"#fff\" stroke-width=\"1.75\" stroke-linecap=\"round\"/>"
    "</svg>";

static esp_err_t favicon_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "image/svg+xml");
    httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=86400");
    return httpd_resp_send(req, FAVICON_SVG, HTTPD_RESP_USE_STRLEN);
}

esp_err_t app_network_web_ui_register(httpd_handle_t server)
{
    if (server == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    static const httpd_uri_t routes[] = {
        {.uri = "/", .method = HTTP_GET, .handler = index_get_handler},
        {.uri = "/favicon.ico", .method = HTTP_GET, .handler = favicon_get_handler},
        {.uri = "/favicon.svg", .method = HTTP_GET, .handler = favicon_get_handler},
        {.uri = "/api/status", .method = HTTP_GET, .handler = api_status_get},
        {.uri = "/api/config", .method = HTTP_GET, .handler = api_config_get},
        {.uri = "/api/config", .method = HTTP_POST, .handler = api_config_post},
        {.uri = "/api/timezones", .method = HTTP_GET, .handler = api_timezones_get},
        {.uri = "/api/wifi", .method = HTTP_GET, .handler = api_wifi_get},
        {.uri = "/api/wifi", .method = HTTP_POST, .handler = api_wifi_add_post},
        {.uri = "/api/wifi/delete", .method = HTTP_POST, .handler = api_wifi_delete_post},
        {.uri = "/api/wifi/edit", .method = HTTP_POST, .handler = api_wifi_edit_post},
        {.uri = "/api/wifi/reorder", .method = HTTP_POST, .handler = api_wifi_reorder_post},
        {.uri = "/api/wifi/connect", .method = HTTP_POST, .handler = api_wifi_connect_post},
        {.uri = "/api/schedule/events", .method = HTTP_GET, .handler = api_schedule_events_get},
        {.uri = "/api/schedule/events", .method = HTTP_POST, .handler = api_schedule_events_add_post},
        {.uri = "/api/schedule/events/edit", .method = HTTP_POST, .handler = api_schedule_events_edit_post},
        {.uri = "/api/schedule/events/delete", .method = HTTP_POST, .handler = api_schedule_events_delete_post},
        {.uri = "/api/schedule/buttons", .method = HTTP_GET, .handler = api_schedule_buttons_get},
        {.uri = "/api/schedule/buttons", .method = HTTP_POST, .handler = api_schedule_buttons_add_post},
        {.uri = "/api/schedule/buttons/edit", .method = HTTP_POST, .handler = api_schedule_buttons_edit_post},
        {.uri = "/api/schedule/buttons/delete", .method = HTTP_POST, .handler = api_schedule_buttons_delete_post},
        {.uri = "/api/reboot", .method = HTTP_POST, .handler = api_reboot_post},
        {.uri = "/api/update", .method = HTTP_GET, .handler = api_update_get},
        {.uri = "/api/update/start", .method = HTTP_POST, .handler = api_update_start_post},
        {.uri = "/api/timer/start", .method = HTTP_POST, .handler = api_timer_start_post},
        {.uri = "/api/timer/cancel", .method = HTTP_POST, .handler = api_timer_cancel_post},
        {.uri = "/api/mode/set", .method = HTTP_POST, .handler = api_mode_set_post},
        {.uri = "/api/images", .method = HTTP_GET, .handler = api_images_get},
        {.uri = "/api/images/file", .method = HTTP_GET, .handler = api_images_file_get},
    };

    for (size_t i = 0; i < sizeof(routes) / sizeof(routes[0]); i++) {
        esp_err_t err = httpd_register_uri_handler(server, &routes[i]);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "register %s failed", routes[i].uri);
            return err;
        }
    }

    ESP_LOGI(TAG, "web UI routes registered");
    return ESP_OK;
}
