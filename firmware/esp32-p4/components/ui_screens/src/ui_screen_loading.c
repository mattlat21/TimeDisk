#include "ui_screens_registry.h"
#include "ui_widgets.h"
#include "ui_wedge.h"
#include "ui_theme.h"
#include "ui_nav.h"

#include <esp_log.h>
#include <esp_timer.h>

static lv_obj_t *s_scr;
static lv_obj_t *s_lbl_title;
static lv_obj_t *s_lbl_status;
static ui_wedge_t *s_settings_wedge;
static bool s_menu_mode;

static void settings_btn_cb(lv_event_t *e)
{
    (void)e;
    ui_nav_go(UI_SCREEN_SETTINGS);
}

static void loading_apply_layout(void)
{
    if (s_lbl_title == NULL) {
        return;
    }

    if (s_menu_mode) {
        lv_obj_align(s_lbl_title, LV_ALIGN_CENTER, 0, 0);
        if (s_lbl_status != NULL) {
            lv_obj_add_flag(s_lbl_status, LV_OBJ_FLAG_HIDDEN);
        }
        if (s_settings_wedge != NULL) {
            ui_wedge_set_visible(s_settings_wedge, false);
        }
        return;
    }

    lv_obj_align(s_lbl_title, LV_ALIGN_CENTER, 0, -20);
    if (s_lbl_status != NULL) {
        lv_obj_remove_flag(s_lbl_status, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_settings_wedge != NULL) {
        ui_wedge_set_visible(s_settings_wedge, true);
    }
}

void ui_screen_loading_build(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    const ui_theme_t *t = ui_theme_get();
    s_scr = ui_widgets_create_screen();
    screens[UI_SCREEN_LOADING] = s_scr;

    s_lbl_title = lv_label_create(s_scr);
    lv_label_set_text(s_lbl_title, "Loading");
    lv_obj_set_style_text_color(s_lbl_title, t->white, 0);
    lv_obj_set_style_text_font(s_lbl_title, &lv_font_montserrat_26, 0);

    s_lbl_status = lv_label_create(s_scr);
    lv_label_set_text(s_lbl_status, "");
    lv_obj_set_style_text_color(s_lbl_status, t->secondary, 0);
    lv_obj_set_style_text_font(s_lbl_status, &lv_font_montserrat_20, 0);
    lv_obj_align(s_lbl_status, LV_ALIGN_CENTER, 0, 30);

    s_settings_wedge = ui_wedge_create_overlay(s_scr, UI_WEDGE_SETTINGS);
    if (s_settings_wedge != NULL) {
        ui_wedge_bind(s_settings_wedge, UI_WEDGE_SETTINGS, settings_btn_cb, NULL);
    }

    loading_apply_layout();
}

void ui_screen_loading_set_menu_mode(bool menu)
{
    s_menu_mode = menu;
    loading_apply_layout();
    // #region agent log
    ESP_LOGI("DBG06e366",
             "{\"sessionId\":\"06e366\",\"hypothesisId\":\"H2\",\"location\":\"ui_screen_loading.c:set_menu_mode\","
             "\"message\":\"menu_mode_set\",\"data\":{\"menu\":%d},\"timestamp\":%lu}",
             (int)menu, (unsigned long)(esp_timer_get_time() / 1000ULL));
    // #endregion
}

void ui_screen_loading_on_show(void)
{
    ui_screen_loading_set_status("Starting...");
}

void ui_screen_loading_set_status(const char *text)
{
    if (s_lbl_status == NULL || text == NULL || s_menu_mode) {
        return;
    }
    lv_label_set_text(s_lbl_status, text);
}

void ui_screen_loading_apply_theme(void)
{
    const ui_theme_t *t = ui_theme_get();
    if (s_scr == NULL) {
        return;
    }
    ui_widgets_style_circle_panel(s_scr);
    if (s_lbl_status != NULL) {
        lv_obj_set_style_text_color(s_lbl_status, t->secondary, 0);
    }
    if (s_settings_wedge != NULL) {
        ui_wedge_refresh_theme(s_settings_wedge);
    }
}
