/**
 * @file app_network.h
 * @brief WiFi station connect and SNTP time sync (docs/data_flow.md boot path).
 */

#pragma once

#include <esp_err.h>
#include <stdbool.h>
#include <stddef.h>

/** SSID broadcast while setup soft-AP is active. */
#define APP_NETWORK_SETUP_AP_SSID "TimeDisk-Setup"

typedef enum {
    APP_NETWORK_STATE_IDLE = 0,
    APP_NETWORK_STATE_CONNECTING,
    APP_NETWORK_STATE_GOT_IP,
    APP_NETWORK_STATE_SYNCING_TIME,
    APP_NETWORK_STATE_READY,
    APP_NETWORK_STATE_FAILED,
    /** Soft AP + web UI for Wi-Fi configuration. */
    APP_NETWORK_STATE_SETUP_AP,
} app_network_state_t;

/** Called on the boot-sync task when connect + SNTP finish (success or failure). */
typedef void (*app_network_boot_done_cb_t)(bool time_ok, void *user_data);

void app_network_set_boot_done_callback(app_network_boot_done_cb_t cb, void *user_data);

/**
 * Initialise esp_netif, event loop, and WiFi driver (safe to call once at boot).
 */
esp_err_t app_network_init(void);

/**
 * Start (or restart) background boot sync: STA connect then SNTP using app_config.
 * Call from the Loading screen; register app_network_set_boot_done_callback() for completion.
 */
void app_network_start_boot_sync(void);

/** Cancel an in-flight boot sync (e.g. leaving Loading). */
void app_network_cancel_boot_sync(void);

app_network_state_t app_network_get_state(void);

/** Short status line for the Loading screen (valid until next state change). */
const char *app_network_get_status_text(void);

/** True after SNTP has set the system clock this session. */
bool app_network_time_is_valid(void);

/** True while the background boot-sync task is running. */
bool app_network_boot_sync_active(void);

/** True while the setup soft-AP and web UI are active. */
bool app_network_setup_ap_active(void);

/** Status line shown on the loading screen during setup AP mode. */
const char *app_network_setup_status_text(void);

/**
 * Start the setup soft-AP (TimeDisk-Setup) and web UI on 192.168.4.1.
 * Called internally when STA connect fails; may also be called explicitly.
 */
esp_err_t app_network_setup_ap_start(void);

/** Stop setup soft-AP Wi-Fi (HTTP web UI keeps running). */
void app_network_setup_ap_stop(void);

/**
 * Leave setup mode and retry STA connect + SNTP using saved networks.
 * Call from the web UI after editing networks.
 */
void app_network_reconnect_sta(void);

/**
 * When the web UI HTTP server is running, write http://x.x.x.x into @p out.
 * Works on setup AP and on LAN (STA) once an IP is assigned.
 * @return true if the web UI is reachable.
 */
bool app_network_get_web_ui_url(char *out, size_t out_len);

/** Start the Wi-Fi configuration web UI (no-op if already running). */
esp_err_t app_network_web_ui_start(void);

/** True while the HTTP configuration server is running. */
bool app_network_web_ui_active(void);

/**
 * Write the device IPv4 address (STA if connected, else setup AP) into @p out.
 * @return true if the device currently has an IP address.
 */
bool app_network_get_device_ip(char *out, size_t out_len);
