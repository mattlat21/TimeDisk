/**
 * @file ui_screen_first_boot.c
 * @brief One-time welcome screen shown before the theme wizard on first run.
 */

#include "ui_screens_registry.h"
#include "ui_layout.h"
#include "ui_widgets.h"
#include "ui_wedge.h"
#include "ui_theme.h"
#include "ui_nav.h"
#include "ui_assets.h"
#include "app_config.h"
#include "esp_log.h"

static lv_obj_t *s_scr;
static ui_wedge_button_t *s_next_wedge;
static bool s_wedge_visible;

static ui_wedge_config_t first_boot_next_wedge_cfg(void)
{
    const ui_theme_t *t = ui_theme_get();
    return (ui_wedge_config_t){
        .side = UI_WEDGE_SIDE_WIDE,
        .color = t->green,
        .icon = UI_WEDGE_ICON_ARROW_RIGHT,
    };
}

static void first_boot_show_wedge(void)
{
    if (s_next_wedge == NULL || s_wedge_visible) {
        return;
    }
    s_wedge_visible = true;
    lv_obj_clear_flag(s_next_wedge, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_next_wedge);
}

static void first_boot_hide_wedge(void)
{
    s_wedge_visible = false;
    if (s_next_wedge != NULL) {
        lv_obj_add_flag(s_next_wedge, LV_OBJ_FLAG_HIDDEN);
    }
}

static void first_boot_go_next(void)
{
    if (app_config_mark_first_boot_done() != ESP_OK) {
        ESP_LOGW("first_boot", "failed to persist first_boot_done");
    }
    ui_nav_go(UI_SCREEN_STARTUP_THEME);
}

static void screen_tap_cb(lv_event_t *e)
{
    (void)e;
    first_boot_show_wedge();
}

static void next_cb(lv_event_t *e)
{
    (void)e;
    first_boot_go_next();
}

void ui_screen_first_boot_build(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    s_scr = ui_widgets_create_screen_no_ring();
    screens[UI_SCREEN_FIRST_BOOT] = s_scr;

    lv_obj_t *img = lv_image_create(s_scr);
    lv_image_set_src(img, ui_assets_spiffs_path("first_boot_screen"));
    lv_obj_set_size(img, UI_DISP, UI_DISP);
    lv_obj_center(img);
    lv_obj_remove_flag(img, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

    ui_wedge_config_t next_cfg = first_boot_next_wedge_cfg();
    s_next_wedge = ui_wedge_button_create(s_scr, &next_cfg);
    if (s_next_wedge != NULL) {
        lv_obj_add_event_cb(s_next_wedge, next_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_add_flag(s_next_wedge, LV_OBJ_FLAG_HIDDEN);
    }

    lv_obj_add_flag(s_scr, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_scr, screen_tap_cb, LV_EVENT_CLICKED, NULL);
}

void ui_screen_first_boot_on_show(void)
{
    first_boot_hide_wedge();
    ui_nav_set_brightness(100);
}

void ui_screen_first_boot_apply_theme(void)
{
    if (s_scr != NULL) {
        ui_widgets_style_circle_panel_no_ring(s_scr);
    }
    if (s_next_wedge != NULL) {
        ui_wedge_config_t cfg = first_boot_next_wedge_cfg();
        ui_wedge_button_set_color(s_next_wedge, cfg.color);
        ui_wedge_button_set_icon(s_next_wedge, cfg.icon);
    }
}
