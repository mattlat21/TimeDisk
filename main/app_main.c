/**
 * @file app_main.c
 * @brief Firmware entry: board display init and UI navigation bootstrap.
 */

#include "esp_hosted.h"
#include "esp_log.h"
#include "lvgl.h"
#include "bsp/display.h"
#include "bsp/esp-bsp.h"
#include "ui_nav.h"

static const char *TAG = "app_main";

void app_main(void)
{
    bsp_display_cfg_t cfg = {
        .lvgl_port_cfg = ESP_LVGL_PORT_INIT_CONFIG(),
        .buffer_size = BSP_LCD_DRAW_BUFF_SIZE,
        .double_buffer = BSP_LCD_DRAW_BUFF_DOUBLE,
        .flags = {
            .buff_dma = true,
            .buff_spiram = true,
            .sw_rotate = false,
        }
    };
    bsp_display_start_with_config(&cfg);
    bsp_display_backlight_on();
    bsp_display_brightness_set(100);

    ESP_LOGI(TAG, "ESP Hosted init (after display)");
    ESP_ERROR_CHECK(esp_hosted_init());

    bsp_display_lock(0);
    ui_nav_init();
    bsp_display_unlock();
}
