/**
 * @file app_network_web_ui_json.c
 * @brief Shared JSON builders for REST web UI payloads.
 */

#include "app_network_web_ui_internal.h"
#include "app_network.h"
#include "app_config.h"
#include "app_checkpoint.h"
#include "app_schedule.h"

#include <cJSON.h>
#include <esp_app_desc.h>
#include <esp_timer.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char *mode_str(app_mode_t mode)
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

static const char *net_state_str(app_network_state_t st)
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

static void json_add_wifi_list(cJSON *arr, const app_config_t *cfg)
{
    for (int i = 0; i < (int)cfg->wifi_network_count; i++) {
        cJSON *net = cJSON_CreateObject();
        if (net == NULL) {
            continue;
        }
        cJSON_AddNumberToObject(net, "index", i);
        cJSON_AddStringToObject(net, "ssid", cfg->wifi_networks[i].ssid);
        cJSON_AddBoolToObject(net, "password_set", cfg->wifi_networks[i].password_set);
        cJSON_AddItemToArray(arr, net);
    }
}

void app_network_web_ui_json_add_config(cJSON *root, const app_config_t *cfg)
{
    cJSON *wifi = cJSON_CreateArray();
    json_add_wifi_list(wifi, cfg);
    cJSON_AddItemToObject(root, "wifi_networks", wifi);
    cJSON_AddNumberToObject(root, "wifi_network_count", cfg->wifi_network_count);
    cJSON_AddStringToObject(root, "ntp_server", cfg->ntp_server);
    cJSON_AddBoolToObject(root, "timezone_set", cfg->timezone_set);
    cJSON_AddStringToObject(root, "timezone_id", cfg->timezone_id);
    cJSON_AddBoolToObject(root, "theme_set", cfg->theme_set);
    cJSON_AddNumberToObject(root, "timeout_splash_sec", cfg->timeout_splash_sec);
    cJSON_AddNumberToObject(root, "timeout_tod_dim_sec", cfg->timeout_tod_dim_sec);
    cJSON_AddNumberToObject(root, "timeout_tod_menu_sec", cfg->timeout_tod_menu_sec);
    cJSON_AddNumberToObject(root, "timeout_aa_sec", cfg->timeout_aa_sec);
    cJSON_AddNumberToObject(root, "timeout_main_menu_sec", cfg->timeout_main_menu_sec);
    cJSON_AddNumberToObject(root, "timeout_timer_dim_sec", cfg->timeout_timer_dim_sec);
    cJSON_AddNumberToObject(root, "timeout_timer_done_sec", cfg->timeout_timer_done_sec);
    cJSON_AddNumberToObject(root, "backlight_bright_pct", cfg->backlight_bright_pct);
    cJSON_AddNumberToObject(root, "backlight_dim_pct", cfg->backlight_dim_pct);
    cJSON_AddBoolToObject(root, "tod_remaining_enabled", cfg->tod_remaining_enabled);
    cJSON_AddBoolToObject(root, "tod_remaining_dim_enabled", cfg->tod_remaining_dim_enabled);
    cJSON_AddBoolToObject(root, "tod_remaining_threshold_enabled", cfg->tod_remaining_threshold_enabled);
    cJSON_AddNumberToObject(root, "tod_remaining_threshold_sec", cfg->tod_remaining_threshold_sec);
    cJSON_AddNumberToObject(root, "ui_primary_color", cfg->ui_primary_color);
    cJSON_AddNumberToObject(root, "ui_secondary_color", cfg->ui_secondary_color);
    cJSON_AddNumberToObject(root, "timer_duration_sec", cfg->timer_duration_sec);
    cJSON_AddNumberToObject(root, "timer_style_id", cfg->timer_style_id);
    cJSON_AddNumberToObject(root, "wind_down_sec", cfg->wind_down_sec);
    cJSON_AddNumberToObject(root, "sleep_sec", cfg->sleep_sec);
    cJSON_AddNumberToObject(root, "rest_sec", cfg->rest_sec);
    cJSON_AddNumberToObject(root, "aa_methods", cfg->aa_methods);
    cJSON_AddStringToObject(root, "aa_pin", cfg->aa_pin);
    cJSON_AddBoolToObject(root, "aa_pin_set", cfg->aa_pin[0] != '\0');
    cJSON_AddBoolToObject(root, "mqtt_enabled", cfg->mqtt_enabled);
    cJSON_AddStringToObject(root, "mqtt_host", cfg->mqtt_host);
    cJSON_AddNumberToObject(root, "mqtt_port", cfg->mqtt_port);
    cJSON_AddStringToObject(root, "mqtt_username", cfg->mqtt_username);
    cJSON_AddBoolToObject(root, "mqtt_password_set", cfg->mqtt_password[0] != '\0');
}

void app_network_web_ui_json_add_live_status(cJSON *root)
{
    const app_runtime_t *rt = app_runtime_get();
    const esp_app_desc_t *app = esp_app_get_description();
    char dev_id[40] = "";
    char ip[40] = "";
    char ssid[APP_WIFI_SSID_MAX] = "";

    app_config_get_device_id(dev_id, sizeof(dev_id));
    app_network_get_device_ip(ip, sizeof(ip));
    app_network_get_connected_ssid(ssid, sizeof(ssid));

    app_checkpoint_status_t cp = {0};
    app_checkpoint_get_status(time(NULL), &cp);

    cJSON_AddStringToObject(root, "device_id", dev_id);
    cJSON_AddStringToObject(root, "fw_version", app != NULL ? app->version : "unknown");
    cJSON_AddStringToObject(root, "ip", ip);
    cJSON_AddStringToObject(root, "connected_ssid", ssid);
    cJSON_AddStringToObject(root, "wifi_status", net_state_str(app_network_get_state()));
    cJSON_AddBoolToObject(root, "setup_ap_active", app_network_setup_ap_active());
    cJSON_AddBoolToObject(root, "time_valid", rt->time_valid);
    cJSON_AddStringToObject(root, "current_mode", mode_str(rt->current_mode));
    cJSON_AddNumberToObject(root, "mode_remaining_sec", rt->mode_remaining_sec);
    cJSON_AddBoolToObject(root, "cycle_active", rt->cycle_active);
    cJSON_AddBoolToObject(root, "timer_running", rt->timer_running);
    cJSON_AddNumberToObject(root, "active_timer_remaining_sec", rt->active_timer_remaining_sec);
    cJSON_AddNumberToObject(root, "active_timer_start_utc", (double)rt->active_timer_start_utc);
    cJSON_AddNumberToObject(root, "active_timer_end_utc", (double)rt->active_timer_end_utc);
    if (rt->time_valid) {
        const time_t now = time(NULL);
        cJSON_AddNumberToObject(root, "device_now_utc", (double)now);
        cJSON_AddNumberToObject(root, "mode_display_remaining_sec",
                                app_schedule_mode_display_remaining_sec(now, rt));

        app_schedule_next_tod_t next = {0};
        if (app_schedule_next_tod_event(now, rt->current_mode, &next)) {
            cJSON *ev = cJSON_CreateObject();
            if (ev != NULL) {
                cJSON_AddNumberToObject(ev, "index", next.index);
                cJSON_AddNumberToObject(ev, "time_min", next.time_min);
                cJSON_AddStringToObject(ev, "action", app_schedule_action_str(next.action));
                cJSON_AddNumberToObject(ev, "duration_sec", next.duration_sec);
                cJSON_AddNumberToObject(ev, "remaining_sec", next.remaining_sec);
                cJSON_AddBoolToObject(ev, "changes_mode", next.changes_mode);
                cJSON_AddItemToObject(root, "next_tod_event", ev);
            }
        } else {
            cJSON_AddNullToObject(root, "next_tod_event");
        }

        if (rt->timer_running && rt->active_timer_end_utc > 0) {
            struct tm tm_end;
            localtime_r(&rt->active_timer_end_utc, &tm_end);
            cJSON_AddNumberToObject(root, "active_timer_end_hour", tm_end.tm_hour);
            cJSON_AddNumberToObject(root, "active_timer_end_min", tm_end.tm_min);
        }
    } else {
        cJSON_AddNumberToObject(root, "device_now_utc", 0);
        cJSON_AddNumberToObject(root, "mode_display_remaining_sec",
                                rt->cycle_active ? rt->mode_remaining_sec : 0);
        cJSON_AddNullToObject(root, "next_tod_event");
    }
    cJSON_AddNumberToObject(root, "uptime_sec", (double)(esp_timer_get_time() / 1000000LL));
    cJSON_AddBoolToObject(root, "cycle_checkpoint_active", cp.cycle_active);
    cJSON_AddStringToObject(root, "cycle_checkpoint_mode", mode_str(cp.cycle_mode));
    cJSON_AddNumberToObject(root, "cycle_checkpoint_remaining_sec", cp.cycle_remaining_sec);
    cJSON_AddBoolToObject(root, "timer_checkpoint_running", cp.timer_running);
    cJSON_AddNumberToObject(root, "timer_checkpoint_remaining_sec", cp.timer_remaining_sec);
    cJSON_AddStringToObject(root, "status_text", app_network_get_status_text());
}

char *app_network_web_ui_build_rest_status_json(void)
{
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return NULL;
    }

    app_network_web_ui_json_add_live_status(root);
    app_network_web_ui_json_add_config(root, app_config_get());

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}
