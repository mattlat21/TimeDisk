#include "ui_screens_registry.h"
#include "ui_common.h"
#include "ui_theme.h"
#include "ui_nav.h"
#include "app_config.h"
#include <stdio.h>
#include <string.h>

static lv_obj_t *s_scr_bright;
static lv_obj_t *s_scr_dim;
static lv_obj_t *lbl_time_bright;
static lv_obj_t *lbl_mode_bright;
static lv_obj_t *lbl_time_dim;
static lv_obj_t *lbl_mode_dim;

static const char *mode_name(app_mode_t m)
{
    switch (m) {
    case APP_MODE_WAKE: return "Wake";
    case APP_MODE_WIND_DOWN: return "Wind Down";
    case APP_MODE_SLEEP: return "Sleep";
    case APP_MODE_REST: return "Rest";
    default: return "Wake";
    }
}

static void refresh_labels(lv_obj_t *lbl_time, lv_obj_t *lbl_mode)
{
    app_runtime_t *rt = app_runtime_get();
    char tbuf[16];
    if (rt->time_valid) {
        ui_common_format_hh_mm(tbuf, sizeof(tbuf), 12, 0);
    } else {
        snprintf(tbuf, sizeof(tbuf), "--:--");
    }
    lv_label_set_text(lbl_time, tbuf);
    lv_label_set_text(lbl_mode, mode_name(rt->current_mode));
}

static void screen_tap_cb(lv_event_t *e)
{
    (void)e;
    ui_screen_id_t cur = ui_nav_current();
    if (cur == UI_SCREEN_TOD_DIM) {
        ui_nav_go(UI_SCREEN_TOD_BRIGHT);
    }
}

static void menu_btn_cb(lv_event_t *e)
{
    (void)e;
    ui_nav_start_aa(UI_SCREEN_TOD_BRIGHT, UI_SCREEN_MENU);
}

static void build_one(lv_obj_t **scr, lv_obj_t **lbl_time, lv_obj_t **lbl_mode, bool dim)
{
    const ui_theme_t *t = ui_theme_get();
    *scr = ui_common_create_screen();

    *lbl_time = lv_label_create(*scr);
    lv_obj_set_style_text_color(*lbl_time, t->white, 0);
    lv_obj_set_style_text_font(*lbl_time, &lv_font_montserrat_26, 0);
    lv_obj_align(*lbl_time, LV_ALIGN_CENTER, 0, -30);

    *lbl_mode = lv_label_create(*scr);
    lv_obj_set_style_text_color(*lbl_mode, dim ? t->panel : t->secondary, 0);
    lv_obj_set_style_text_font(*lbl_mode, &lv_font_montserrat_26, 0);
    lv_obj_align(*lbl_mode, LV_ALIGN_CENTER, 0, 40);

    lv_obj_t *menu_btn = lv_button_create(*scr);
    lv_obj_set_size(menu_btn, 80, 40);
    lv_obj_align(menu_btn, LV_ALIGN_BOTTOM_MID, 0, -80);
    lv_obj_set_style_bg_color(menu_btn, t->ring, 0);
    lv_obj_t *ml = lv_label_create(menu_btn);
    lv_label_set_text(ml, "Menu");
    lv_obj_set_style_text_color(ml, t->white, 0);
    lv_obj_center(ml);
    lv_obj_add_event_cb(menu_btn, menu_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(*scr, screen_tap_cb, LV_EVENT_CLICKED, NULL);
}

extern "C" void ui_screen_tod_build(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    build_one(&s_scr_bright, &lbl_time_bright, &lbl_mode_bright, false);
    build_one(&s_scr_dim, &lbl_time_dim, &lbl_mode_dim, true);
    screens[UI_SCREEN_TOD_BRIGHT] = s_scr_bright;
    screens[UI_SCREEN_TOD_DIM] = s_scr_dim;
}

extern "C" void ui_screen_tod_on_show(bool dim)
{
    refresh_labels(dim ? lbl_time_dim : lbl_time_bright, dim ? lbl_mode_dim : lbl_mode_bright);
    ui_nav_apply_dim(dim);
}
