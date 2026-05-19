/**
 * @file app_network.h
 * @brief WiFi station connect and SNTP time sync (docs/data_flow.md boot path).
 */

#pragma once

#include <esp_err.h>
#include <stdbool.h>

typedef enum {
    APP_NETWORK_STATE_IDLE = 0,
    APP_NETWORK_STATE_CONNECTING,
    APP_NETWORK_STATE_GOT_IP,
    APP_NETWORK_STATE_SYNCING_TIME,
    APP_NETWORK_STATE_READY,
    APP_NETWORK_STATE_FAILED,
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
