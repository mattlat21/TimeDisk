/**
 * @file app_mqtt.c
 * @brief MQTT status, config/set, and command handling for Home Assistant.
 */

#include "app_mqtt.h"

#include "app_config.h"
#include "app_network.h"
#include "ui_nav.h"
#include "lvgl.h"

#include <cJSON.h>
#include <stdlib.h>
#include <esp_app_desc.h>
#include <esp_log.h>
#include <esp_mac.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <mqtt_client.h>
#include <stdio.h>
#include <string.h>

static const char *TAG = "app_mqtt";

#define STATUS_PERIOD_US     (30LL * 1000000LL)
#define DEBOUNCE_US          (500LL * 1000LL)
#define TOPIC_BUF            96
#define PAYLOAD_MAX          8192

static esp_mqtt_client_handle_t s_client;
static char s_device_id[32];
static char s_topic_prefix[48];
static bool s_connected;
static esp_timer_handle_t s_status_timer;
static esp_timer_handle_t s_debounce_timer;
static SemaphoreHandle_t s_publish_mux;

typedef struct {
    char action[32];
    uint32_t duration_sec;
    uint8_t style_id;
    bool has_duration;
    bool has_style;
} mqtt_cmd_t;

static void build_device_id(void)
{
    uint8_t mac[6];
    if (esp_read_mac(mac, ESP_MAC_WIFI_STA) != ESP_OK) {
        esp_read_mac(mac, ESP_MAC_ETH);
    }
    snprintf(s_device_id, sizeof(s_device_id), "timedisk-%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    snprintf(s_topic_prefix, sizeof(s_topic_prefix), "timedisk/%s", s_device_id);
}

bool app_mqtt_get_device_id(char *out, size_t out_len)
{
    return app_config_get_device_id(out, out_len);
}

bool app_mqtt_is_connected(void)
{
    return s_connected;
}

static const char *mode_to_str(app_mode_t mode)
{
    switch (mode) {
    case APP_MODE_WAKE:
        return "wake";
    case APP_MODE_WIND_DOWN:
        return "wind_down";
    case APP_MODE_SLEEP:
        return "sleep";
    case APP_MODE_REST:
        return "rest";
    default:
        return "wake";
    }
}

static const char *screen_to_str(ui_screen_id_t id)
{
    switch (id) {
    case UI_SCREEN_SPLASH:
        return "splash";
    case UI_SCREEN_LOADING:
        return "loading";
    case UI_SCREEN_TOD_BRIGHT:
        return "tod_bright";
    case UI_SCREEN_TOD_DIM:
        return "tod_dim";
    case UI_SCREEN_MENU:
        return "menu";
    case UI_SCREEN_SETTINGS:
        return "settings";
    case UI_SCREEN_TIMER_BRIGHT:
        return "timer_bright";
    case UI_SCREEN_TIMER_DIM:
        return "timer_dim";
    default:
        return "other";
    }
}

static const char *network_state_to_str(app_network_state_t st)
{
    switch (st) {
    case APP_NETWORK_STATE_IDLE:
        return "idle";
    case APP_NETWORK_STATE_CONNECTING:
        return "connecting";
    case APP_NETWORK_STATE_GOT_IP:
        return "got_ip";
    case APP_NETWORK_STATE_SYNCING_TIME:
        return "syncing_time";
    case APP_NETWORK_STATE_READY:
        return "ready";
    case APP_NETWORK_STATE_FAILED:
        return "failed";
    case APP_NETWORK_STATE_SETUP_AP:
        return "setup_ap";
    default:
        return "unknown";
    }
}

static void json_add_wifi_networks(cJSON *arr, const app_config_t *cfg)
{
    for (int i = 0; i < (int)cfg->wifi_network_count; i++) {
        cJSON *net = cJSON_CreateObject();
        if (net == NULL) {
            continue;
        }
        cJSON_AddStringToObject(net, "ssid", cfg->wifi_networks[i].ssid);
        cJSON_AddBoolToObject(net, "password_set", cfg->wifi_networks[i].password_set);
        cJSON_AddItemToArray(arr, net);
    }
}

static char *build_status_json(void)
{
    const app_config_t *cfg = app_config_get();
    const app_runtime_t *rt = app_runtime_get();
    const esp_app_desc_t *app = esp_app_get_description();
    char ip[40] = "";

    (void)app_network_get_device_ip(ip, sizeof(ip));

    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return NULL;
    }

    cJSON_AddStringToObject(root, "device_id", s_device_id);
    cJSON_AddStringToObject(root, "fw_version", app != NULL ? app->version : "unknown");

    cJSON *runtime = cJSON_CreateObject();
    cJSON_AddStringToObject(runtime, "current_mode", mode_to_str(rt->current_mode));
    cJSON_AddNumberToObject(runtime, "mode_remaining_sec", rt->mode_remaining_sec);
    cJSON_AddBoolToObject(runtime, "cycle_active", rt->cycle_active);
    cJSON_AddBoolToObject(runtime, "time_valid", rt->time_valid);
    cJSON_AddNumberToObject(runtime, "display_brightness", rt->display_brightness);
    cJSON_AddBoolToObject(runtime, "timer_running", rt->timer_running);
    cJSON_AddNumberToObject(runtime, "active_timer_remaining_sec", rt->active_timer_remaining_sec);
    cJSON_AddNumberToObject(runtime, "active_timer_total_sec", rt->active_timer_total_sec);
    cJSON_AddItemToObject(root, "runtime", runtime);

    cJSON *config = cJSON_CreateObject();
    cJSON *wifi = cJSON_CreateArray();
    json_add_wifi_networks(wifi, cfg);
    cJSON_AddItemToObject(config, "wifi_networks", wifi);
    cJSON_AddNumberToObject(config, "wifi_network_count", cfg->wifi_network_count);
    cJSON_AddStringToObject(config, "ntp_server", cfg->ntp_server);
    cJSON_AddBoolToObject(config, "timezone_set", cfg->timezone_set);
    cJSON_AddStringToObject(config, "timezone_id", cfg->timezone_id);
    cJSON_AddBoolToObject(config, "theme_set", cfg->theme_set);
    cJSON_AddNumberToObject(config, "timeout_splash_sec", cfg->timeout_splash_sec);
    cJSON_AddNumberToObject(config, "timeout_tod_dim_sec", cfg->timeout_tod_dim_sec);
    cJSON_AddNumberToObject(config, "timeout_tod_menu_sec", cfg->timeout_tod_menu_sec);
    cJSON_AddNumberToObject(config, "timeout_aa_sec", cfg->timeout_aa_sec);
    cJSON_AddNumberToObject(config, "timeout_main_menu_sec", cfg->timeout_main_menu_sec);
    cJSON_AddNumberToObject(config, "timeout_timer_dim_sec", cfg->timeout_timer_dim_sec);
    cJSON_AddNumberToObject(config, "ui_primary_color", cfg->ui_primary_color);
    cJSON_AddNumberToObject(config, "ui_secondary_color", cfg->ui_secondary_color);
    cJSON_AddNumberToObject(config, "timer_duration_sec", cfg->timer_duration_sec);
    cJSON_AddNumberToObject(config, "timer_style_id", cfg->timer_style_id);
    cJSON_AddNumberToObject(config, "wind_down_sec", cfg->wind_down_sec);
    cJSON_AddNumberToObject(config, "sleep_sec", cfg->sleep_sec);
    cJSON_AddNumberToObject(config, "rest_sec", cfg->rest_sec);
    cJSON_AddNumberToObject(config, "aa_methods", cfg->aa_methods);
    cJSON_AddBoolToObject(config, "aa_pin_set", cfg->aa_pin[0] != '\0');
    cJSON_AddBoolToObject(config, "mqtt_enabled", cfg->mqtt_enabled);
    cJSON_AddStringToObject(config, "mqtt_host", cfg->mqtt_host);
    cJSON_AddNumberToObject(config, "mqtt_port", cfg->mqtt_port);
    cJSON_AddStringToObject(config, "mqtt_username", cfg->mqtt_username);
    cJSON_AddBoolToObject(config, "mqtt_password_set", cfg->mqtt_password[0] != '\0');
    cJSON_AddItemToObject(root, "config", config);

    cJSON *ctx = cJSON_CreateObject();
    cJSON_AddStringToObject(ctx, "active_screen", screen_to_str(ui_nav_current()));
    cJSON_AddStringToObject(ctx, "wifi_status", network_state_to_str(app_network_get_state()));
    cJSON_AddStringToObject(ctx, "ip", ip);
    cJSON_AddNumberToObject(ctx, "uptime_sec", (double)(esp_timer_get_time() / 1000000LL));
    cJSON_AddBoolToObject(ctx, "mqtt_connected", s_connected);
    cJSON_AddItemToObject(root, "context", ctx);

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}

void app_mqtt_publish_status(void)
{
    if (s_client == NULL || !s_connected) {
        return;
    }

    char *payload = build_status_json();
    if (payload == NULL) {
        return;
    }

    char topic[TOPIC_BUF];
    snprintf(topic, sizeof(topic), "%s/status", s_topic_prefix);

    if (xSemaphoreTake(s_publish_mux, pdMS_TO_TICKS(2000)) == pdTRUE) {
        esp_mqtt_client_publish(s_client, topic, payload, 0, 1, 1);
        xSemaphoreGive(s_publish_mux);
    }
    cJSON_free(payload);
}

static void debounce_cb(void *arg)
{
    (void)arg;
    app_mqtt_publish_status();
}

void app_mqtt_publish_status_debounced(void)
{
    if (s_debounce_timer == NULL) {
        app_mqtt_publish_status();
        return;
    }
    esp_timer_stop(s_debounce_timer);
    esp_timer_start_once(s_debounce_timer, DEBOUNCE_US);
}

static void publish_announce(void)
{
    if (s_client == NULL || !s_connected) {
        return;
    }

    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return;
    }
    cJSON_AddStringToObject(root, "device_id", s_device_id);
    cJSON_AddStringToObject(root, "name", "TimeDisk");
    const esp_app_desc_t *app = esp_app_get_description();
    cJSON_AddStringToObject(root, "fw_version", app != NULL ? app->version : "unknown");

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (payload == NULL) {
        return;
    }

    char topic[TOPIC_BUF];
    snprintf(topic, sizeof(topic), "%s/announce", s_topic_prefix);
    esp_mqtt_client_publish(s_client, topic, payload, 0, 0, 0);
    cJSON_free(payload);
}

static void mqtt_cmd_async_cb(void *user_data)
{
    mqtt_cmd_t *cmd = (mqtt_cmd_t *)user_data;
    if (cmd == NULL) {
        return;
    }

    if (strcmp(cmd->action, "start_sleep_cycle") == 0) {
        ui_nav_mqtt_start_sleep_cycle();
    } else if (strcmp(cmd->action, "start_rest_cycle") == 0) {
        ui_nav_mqtt_start_rest_cycle();
    } else if (strcmp(cmd->action, "end_cycle") == 0) {
        ui_nav_mqtt_end_cycle();
    } else if (strcmp(cmd->action, "start_timer") == 0) {
        uint32_t dur = cmd->has_duration ? cmd->duration_sec : app_config_get()->timer_duration_sec;
        uint8_t style = cmd->has_style ? cmd->style_id : app_config_get()->timer_style_id;
        ui_nav_mqtt_start_timer(dur, style);
    } else if (strcmp(cmd->action, "cancel_timer") == 0) {
        ui_nav_mqtt_cancel_timer();
    } else {
        ESP_LOGW(TAG, "unknown command action: %s", cmd->action);
    }

    free(cmd);
}

static void dispatch_command(const char *payload, int len)
{
    if (payload == NULL || len <= 0) {
        return;
    }

    cJSON *root = cJSON_ParseWithLength(payload, (size_t)len);
    if (root == NULL) {
        ESP_LOGW(TAG, "command JSON parse failed");
        return;
    }

    const cJSON *action = cJSON_GetObjectItem(root, "action");
    if (!cJSON_IsString(action) || action->valuestring == NULL) {
        cJSON_Delete(root);
        return;
    }

    mqtt_cmd_t *cmd = calloc(1, sizeof(*cmd));
    if (cmd == NULL) {
        cJSON_Delete(root);
        return;
    }
    snprintf(cmd->action, sizeof(cmd->action), "%s", action->valuestring);

    const cJSON *dur = cJSON_GetObjectItem(root, "duration_sec");
    if (cJSON_IsNumber(dur)) {
        cmd->duration_sec = (uint32_t)dur->valuedouble;
        cmd->has_duration = true;
    }
    const cJSON *style = cJSON_GetObjectItem(root, "style_id");
    if (cJSON_IsNumber(style)) {
        cmd->style_id = (uint8_t)style->valuedouble;
        cmd->has_style = true;
    }

    cJSON_Delete(root);

    if (lv_async_call(mqtt_cmd_async_cb, cmd) != LV_RESULT_OK) {
        free(cmd);
        ESP_LOGW(TAG, "lv_async_call failed for command");
    }
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
    if (strcmp(key, "ui_primary_color") == 0 && cJSON_IsNumber(val)) {
        cfg->ui_primary_color = (uint32_t)val->valuedouble;
        return true;
    }
    if (strcmp(key, "ui_secondary_color") == 0 && cJSON_IsNumber(val)) {
        cfg->ui_secondary_color = (uint32_t)val->valuedouble;
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

static void handle_config_set(const char *payload, int len)
{
    if (payload == NULL || len <= 0) {
        return;
    }

    cJSON *root = cJSON_ParseWithLength(payload, (size_t)len);
    if (root == NULL) {
        ESP_LOGW(TAG, "config/set JSON parse failed");
        return;
    }

    app_config_t *cfg = app_config_get();
    bool changed_mqtt = false;
    bool changed_timeouts = false;
    bool changed_theme = false;
    bool changed_timer = false;
    bool changed_schedule = false;
    bool changed_aa = false;
    bool changed_network = false;

    for (const cJSON *item = root->child; item != NULL; item = item->next) {
        if (item->string == NULL) {
            continue;
        }
        const char *key = item->string;
        if (strcmp(key, "mqtt_host") == 0 || strcmp(key, "mqtt_port") == 0 ||
            strcmp(key, "mqtt_username") == 0 || strcmp(key, "mqtt_password") == 0 ||
            strcmp(key, "mqtt_enabled") == 0) {
            if (apply_config_field(cfg, key, item)) {
                changed_mqtt = true;
            }
        } else if (strncmp(key, "timeout_", 8) == 0) {
            if (apply_config_field(cfg, key, item)) {
                changed_timeouts = true;
            }
        } else if (strcmp(key, "ui_primary_color") == 0 || strcmp(key, "ui_secondary_color") == 0) {
            if (apply_config_field(cfg, key, item)) {
                changed_theme = true;
            }
        } else if (strcmp(key, "timer_duration_sec") == 0 || strcmp(key, "timer_style_id") == 0) {
            if (apply_config_field(cfg, key, item)) {
                changed_timer = true;
            }
        } else if (strcmp(key, "wind_down_sec") == 0 || strcmp(key, "sleep_sec") == 0 ||
                   strcmp(key, "rest_sec") == 0) {
            if (apply_config_field(cfg, key, item)) {
                changed_schedule = true;
            }
        } else if (strcmp(key, "aa_methods") == 0 || strcmp(key, "aa_pin") == 0) {
            if (apply_config_field(cfg, key, item)) {
                changed_aa = true;
            }
        } else if (strcmp(key, "ntp_server") == 0 || strcmp(key, "timezone_id") == 0) {
            if (apply_config_field(cfg, key, item)) {
                changed_network = true;
            }
        } else {
            apply_config_field(cfg, key, item);
        }
    }

    cJSON_Delete(root);

    if (changed_network) {
        app_config_save_network();
    }
    if (changed_timeouts) {
        app_config_save_timeouts();
    }
    if (changed_theme) {
        app_config_save_theme();
    }
    if (changed_timer) {
        app_config_save_timer();
    }
    if (changed_schedule) {
        app_config_save_schedule();
    }
    if (changed_aa) {
        app_config_save_aa();
    }
    if (changed_mqtt) {
        app_config_save_mqtt();
    }

    app_mqtt_publish_status();
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    (void)handler_args;
    (void)base;
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        s_connected = true;
        ESP_LOGI(TAG, "MQTT connected");
        {
            char topic[TOPIC_BUF];
            snprintf(topic, sizeof(topic), "%s/config/set", s_topic_prefix);
            esp_mqtt_client_subscribe(s_client, topic, 1);
            snprintf(topic, sizeof(topic), "%s/command", s_topic_prefix);
            esp_mqtt_client_subscribe(s_client, topic, 1);
        }
        {
            char avail_topic[TOPIC_BUF];
            snprintf(avail_topic, sizeof(avail_topic), "%s/availability", s_topic_prefix);
            esp_mqtt_client_publish(s_client, avail_topic, "online", 0, 1, 1);
        }
        publish_announce();
        app_mqtt_publish_status();
        break;
    case MQTT_EVENT_DISCONNECTED:
        s_connected = false;
        ESP_LOGW(TAG, "MQTT disconnected");
        break;
    case MQTT_EVENT_DATA:
        if (event->topic_len <= 0 || event->data_len <= 0) {
            break;
        }
        {
            char topic[TOPIC_BUF];
            int tlen = event->topic_len < (int)sizeof(topic) - 1 ? event->topic_len : (int)sizeof(topic) - 1;
            memcpy(topic, event->topic, (size_t)tlen);
            topic[tlen] = '\0';

            char cmd_topic[TOPIC_BUF];
            char cfg_topic[TOPIC_BUF];
            snprintf(cmd_topic, sizeof(cmd_topic), "%s/command", s_topic_prefix);
            snprintf(cfg_topic, sizeof(cfg_topic), "%s/config/set", s_topic_prefix);

            if (strcmp(topic, cmd_topic) == 0) {
                dispatch_command(event->data, event->data_len);
            } else if (strcmp(topic, cfg_topic) == 0) {
                handle_config_set(event->data, event->data_len);
            }
        }
        break;
    default:
        break;
    }
}

static void status_period_cb(void *arg)
{
    (void)arg;
    app_mqtt_publish_status();
}

static void network_state_cb(app_network_state_t state, void *user_data)
{
    (void)user_data;
    if (state == APP_NETWORK_STATE_READY) {
        app_mqtt_reconnect();
    }
}

static void mqtt_client_destroy(void)
{
    if (s_client != NULL) {
        esp_mqtt_client_stop(s_client);
        esp_mqtt_client_destroy(s_client);
        s_client = NULL;
    }
    s_connected = false;
}

void app_mqtt_reconnect(void)
{
    const app_config_t *cfg = app_config_get();

    mqtt_client_destroy();

    if (!cfg->mqtt_enabled || cfg->mqtt_host[0] == '\0') {
        return;
    }

    if (app_network_get_state() != APP_NETWORK_STATE_READY &&
        app_network_get_state() != APP_NETWORK_STATE_GOT_IP) {
        return;
    }

    char uri[128];
    snprintf(uri, sizeof(uri), "mqtt://%s:%u", cfg->mqtt_host, (unsigned)cfg->mqtt_port);

    char avail_topic[TOPIC_BUF];
    snprintf(avail_topic, sizeof(avail_topic), "%s/availability", s_topic_prefix);

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = uri,
        .credentials.username = cfg->mqtt_username[0] != '\0' ? cfg->mqtt_username : NULL,
        .credentials.authentication.password = cfg->mqtt_password[0] != '\0' ? cfg->mqtt_password : NULL,
        .session.last_will.topic = avail_topic,
        .session.last_will.msg = "offline",
        .session.last_will.qos = 1,
        .session.last_will.retain = true,
        .session.keepalive = 60,
    };

    s_client = esp_mqtt_client_init(&mqtt_cfg);
    if (s_client == NULL) {
        ESP_LOGE(TAG, "esp_mqtt_client_init failed");
        return;
    }

    esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_err_t err = esp_mqtt_client_start(s_client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mqtt start failed: %s", esp_err_to_name(err));
        mqtt_client_destroy();
    }
}

esp_err_t app_mqtt_init(void)
{
    build_device_id();
    app_config_get_device_id(s_device_id, sizeof(s_device_id));
    s_publish_mux = xSemaphoreCreateMutex();
    if (s_publish_mux == NULL) {
        return ESP_ERR_NO_MEM;
    }

    const esp_timer_create_args_t debounce_args = {
        .callback = debounce_cb,
        .name = "mqtt_deb",
    };
    ESP_ERROR_CHECK(esp_timer_create(&debounce_args, &s_debounce_timer));

    const esp_timer_create_args_t period_args = {
        .callback = status_period_cb,
        .name = "mqtt_stat",
    };
    ESP_ERROR_CHECK(esp_timer_create(&period_args, &s_status_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(s_status_timer, STATUS_PERIOD_US));

    ui_nav_set_mqtt_status_hook(app_mqtt_publish_status_debounced);
    app_config_set_mqtt_saved_hook(app_mqtt_reconnect);
    app_network_set_state_callback(network_state_cb, NULL);
    app_mqtt_reconnect();

    ESP_LOGI(TAG, "init device_id=%s", s_device_id);
    return ESP_OK;
}
