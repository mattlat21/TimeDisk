/**
 * @file ui_display.c
 * @brief Project-owned display settings applied after BSP init.
 */

#include "sdkconfig.h"
#include "esp_check.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"
#include "lvgl.h"
#include "ui_display.h"

static const char *TAG = "ui_display";

/** Matches the leading fields of esp_lvgl_port's private lvgl_port_display_ctx_t. */
typedef struct {
    int disp_type;
    esp_lcd_panel_io_handle_t io_handle;
    esp_lcd_panel_handle_t panel_handle;
} ui_display_port_ctx_hdr_t;

static esp_err_t ui_display_get_panel(esp_lcd_panel_handle_t *panel_out)
{
    lv_display_t *disp = lv_display_get_default();
    ESP_RETURN_ON_FALSE(disp != NULL, ESP_ERR_INVALID_STATE, TAG, "No default LVGL display");

    ui_display_port_ctx_hdr_t *ctx = (ui_display_port_ctx_hdr_t *)lv_display_get_driver_data(disp);
    ESP_RETURN_ON_FALSE(ctx != NULL && ctx->panel_handle != NULL, ESP_ERR_INVALID_STATE, TAG,
                        "No LCD panel handle on display driver data");

    *panel_out = ctx->panel_handle;
    return ESP_OK;
}

esp_err_t ui_display_apply_settings(void)
{
    esp_lcd_panel_handle_t panel = NULL;
    ESP_RETURN_ON_ERROR(ui_display_get_panel(&panel), TAG, "Panel handle unavailable");

    /* BSP always inverts at init; apply the project setting explicitly. */
#if CONFIG_UI_DISPLAY_INVERT
    ESP_RETURN_ON_ERROR(esp_lcd_panel_invert_color(panel, true), TAG, "Panel color invert failed");
    ESP_LOGI(TAG, "Display color invert enabled");
#else
    ESP_RETURN_ON_ERROR(esp_lcd_panel_invert_color(panel, false), TAG, "Panel color invert disable failed");
    ESP_LOGI(TAG, "Display color invert disabled");
#endif

#if CONFIG_UI_DISPLAY_ROTATE_180
    lv_display_t *disp_rot = lv_display_get_default();
    ESP_RETURN_ON_FALSE(disp_rot != NULL, ESP_ERR_INVALID_STATE, TAG, "No default LVGL display");
    lv_display_set_rotation(disp_rot, LV_DISPLAY_ROTATION_180);
    ESP_LOGI(TAG, "Display and touch rotated 180 degrees");
#endif

    return ESP_OK;
}
