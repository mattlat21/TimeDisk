/**
 * @file app_network_web_ui.h
 * @brief TimeDisk settings web UI (SPA shell + JSON API).
 */

#pragma once

#include <esp_err.h>
#include <esp_http_server.h>
#include <stdint.h>

typedef struct {
    void (*timer_start)(uint32_t duration_sec, uint8_t style_id);
    void (*timer_cancel)(void);
} app_network_web_ui_timer_ops_t;

/** Register LVGL-thread timer control (called from ui_nav_init). */
void app_network_web_ui_set_timer_ops(const app_network_web_ui_timer_ops_t *ops);

esp_err_t app_network_web_ui_register(httpd_handle_t server);
