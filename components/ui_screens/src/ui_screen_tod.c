/**
 * @file ui_screen_tod.c
 * @brief Time of Day bright/dim screens with per-mode layouts (Wake, Wind Down, Sleep, Rest).
 */

#include "ui_screens_registry.h"
#include "ui_layout.h"
#include "ui_widgets.h"
#include "ui_format.h"
#include "ui_theme.h"
#include "ui_nav.h"
#include "app_config.h"
#include <stdio.h>
#include <string.h>

#define TOD_MODE_COUNT 4

#define TOD_SLEEP_PANEL_W  480
#define TOD_SLEEP_PANEL_H  200

typedef struct {
    lv_obj_t *root;
    lv_obj_t *lbl_time;
    lv_obj_t *lbl_subtitle;
    lv_obj_t *lbl_remaining;
    lv_obj_t *menu_btn;
} tod_mode_panel_t;

static lv_obj_t *s_scr_bright;
static lv_obj_t *s_scr_dim;
static tod_mode_panel_t s_panels_bright[TOD_MODE_COUNT];
static tod_mode_panel_t s_panels_dim[TOD_MODE_COUNT];
static bool s_showing_dim;

static tod_mode_panel_t *panel_for(app_mode_t mode, bool dim)
{
    if (mode >= TOD_MODE_COUNT) {
        mode = APP_MODE_WAKE;
    }
    return dim ? &s_panels_dim[mode] : &s_panels_bright[mode];
}

static void screen_tap_cb(lv_event_t *e)
{
    (void)e;
    if (ui_nav_current() == UI_SCREEN_TOD_DIM) {
        ui_nav_go(UI_SCREEN_TOD_BRIGHT);
    }
}

static void menu_btn_cb(lv_event_t *e)
{
    (void)e;
    ui_nav_start_aa(UI_SCREEN_TOD_BRIGHT, UI_SCREEN_MENU);
}

static lv_obj_t *create_menu_btn(lv_obj_t *parent)
{
    const ui_theme_t *t = ui_theme_get();

    lv_obj_t *menu_btn = lv_button_create(parent);
    lv_obj_set_size(menu_btn, 160, 80);
    lv_obj_align(menu_btn, LV_ALIGN_BOTTOM_MID, 0, -80);
    lv_obj_set_style_bg_color(menu_btn, t->ring, 0);
    lv_obj_set_style_radius(menu_btn, 16, 0);
    lv_obj_t *ml = lv_label_create(menu_btn);
    lv_label_set_text(ml, "Menu");
    lv_obj_set_style_text_color(ml, t->white, 0);
    lv_obj_set_style_text_font(ml, &lv_font_montserrat_26, 0);
    lv_obj_center(ml);
    lv_obj_add_event_cb(menu_btn, menu_btn_cb, LV_EVENT_CLICKED, NULL);
    return menu_btn;
}

static lv_obj_t *create_panel_root(lv_obj_t *parent)
{
    int32_t cw = 0;
    int32_t ch = 0;

    ui_layout_get_content_size(parent, &cw, &ch);

    lv_obj_t *root = lv_obj_create(parent);
    lv_obj_set_size(root, cw, ch);
    lv_obj_set_pos(root, 0, 0);
    lv_obj_set_style_bg_opa(root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(root, 0, 0);
    lv_obj_remove_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    return root;
}

static void refresh_panel_time(const tod_mode_panel_t *p)
{
    if (p->lbl_time == NULL) {
        return;
    }
    app_runtime_t *rt = app_runtime_get();
    char tbuf[16];
    if (rt->time_valid) {
        ui_format_hh_mm_now(tbuf, sizeof(tbuf));
    } else {
        snprintf(tbuf, sizeof(tbuf), "--:--");
    }
    lv_label_set_text(p->lbl_time, tbuf);
}

static void refresh_panel_remaining(const tod_mode_panel_t *p, app_mode_t mode)
{
    if (p->lbl_remaining == NULL) {
        return;
    }
    app_runtime_t *rt = app_runtime_get();
    if (!rt->cycle_active || mode == APP_MODE_WAKE) {
        lv_obj_add_flag(p->lbl_remaining, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    char buf[16];
    ui_format_mm_ss(buf, sizeof(buf), rt->mode_remaining_sec);
    lv_label_set_text(p->lbl_remaining, buf);
    lv_obj_remove_flag(p->lbl_remaining, LV_OBJ_FLAG_HIDDEN);
}

static void apply_dim_styles(const tod_mode_panel_t *p, bool dim)
{
    lv_opa_t opa = dim ? LV_OPA_60 : LV_OPA_COVER;
    if (p->lbl_time != NULL) {
        lv_obj_set_style_text_opa(p->lbl_time, opa, 0);
    }
    if (p->lbl_subtitle != NULL) {
        lv_obj_set_style_text_opa(p->lbl_subtitle, opa, 0);
    }
    if (p->lbl_remaining != NULL) {
        lv_obj_set_style_text_opa(p->lbl_remaining, opa, 0);
    }
}

static void refresh_panel(const tod_mode_panel_t *p, app_mode_t mode, bool dim)
{
    refresh_panel_time(p);
    refresh_panel_remaining(p, mode);
    apply_dim_styles(p, dim);
}

static void build_wake_panel(lv_obj_t *scr, tod_mode_panel_t *p, bool dim)
{
    const ui_theme_t *t = ui_theme_get();

    (void)dim;
    p->root = create_panel_root(scr);
    p->lbl_subtitle = NULL;
    p->lbl_remaining = NULL;

    p->lbl_time = lv_label_create(p->root);
    lv_obj_set_style_text_color(p->lbl_time, t->white, 0);
    lv_obj_set_style_text_font(p->lbl_time, &lv_font_montserrat_48, 0);
    lv_obj_align(p->lbl_time, LV_ALIGN_CENTER, 0, -30);

    p->menu_btn = create_menu_btn(p->root);
}

static void build_wind_down_panel(lv_obj_t *scr, tod_mode_panel_t *p, bool dim)
{
    const ui_theme_t *t = ui_theme_get();

    (void)dim;
    p->root = create_panel_root(scr);

    p->lbl_subtitle = lv_label_create(p->root);
    lv_label_set_text(p->lbl_subtitle, "Wind Down");
    lv_obj_set_style_text_color(p->lbl_subtitle, t->secondary, 0);
    lv_obj_set_style_text_font(p->lbl_subtitle, &lv_font_montserrat_20, 0);
    lv_obj_align(p->lbl_subtitle, LV_ALIGN_CENTER, 0, -80);

    p->lbl_time = lv_label_create(p->root);
    lv_obj_set_style_text_color(p->lbl_time, t->white, 0);
    lv_obj_set_style_text_font(p->lbl_time, &lv_font_montserrat_48, 0);
    lv_obj_align(p->lbl_time, LV_ALIGN_CENTER, 0, -20);

    p->lbl_remaining = lv_label_create(p->root);
    lv_obj_set_style_text_color(p->lbl_remaining, t->orange, 0);
    lv_obj_set_style_text_font(p->lbl_remaining, &lv_font_montserrat_20, 0);
    lv_obj_align(p->lbl_remaining, LV_ALIGN_CENTER, 0, 40);

    p->menu_btn = create_menu_btn(p->root);
}

static void build_sleep_style_panel(lv_obj_t *scr, tod_mode_panel_t *p, bool dim,
                                    const char *title, lv_color_t panel_bg, lv_color_t countdown_color)
{
    const ui_theme_t *t = ui_theme_get();

    (void)dim;
    p->root = create_panel_root(scr);

    lv_obj_t *box = lv_obj_create(p->root);
    lv_obj_set_size(box, TOD_SLEEP_PANEL_W, TOD_SLEEP_PANEL_H);
    lv_obj_align(box, LV_ALIGN_CENTER, 0, -30);
    lv_obj_set_style_bg_color(box, panel_bg, 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(box, 0, 0);
    lv_obj_set_style_radius(box, 16, 0);
    lv_obj_remove_flag(box, LV_OBJ_FLAG_SCROLLABLE);

    p->lbl_subtitle = lv_label_create(box);
    lv_label_set_text(p->lbl_subtitle, title);
    lv_obj_set_style_text_color(p->lbl_subtitle, t->white, 0);
    lv_obj_set_style_text_font(p->lbl_subtitle, &lv_font_montserrat_20, 0);
    lv_obj_align(p->lbl_subtitle, LV_ALIGN_TOP_MID, 0, 16);

    p->lbl_time = lv_label_create(box);
    lv_obj_set_style_text_color(p->lbl_time, t->white, 0);
    lv_obj_set_style_text_font(p->lbl_time, &lv_font_montserrat_48, 0);
    lv_obj_align(p->lbl_time, LV_ALIGN_CENTER, 0, -8);

    p->lbl_remaining = lv_label_create(box);
    lv_obj_set_style_text_color(p->lbl_remaining, countdown_color, 0);
    lv_obj_set_style_text_font(p->lbl_remaining, &lv_font_montserrat_20, 0);
    lv_obj_align(p->lbl_remaining, LV_ALIGN_BOTTOM_MID, 0, -16);

    p->menu_btn = create_menu_btn(p->root);
}

static void build_sleep_panel(lv_obj_t *scr, tod_mode_panel_t *p, bool dim)
{
    const ui_theme_t *t = ui_theme_get();
    build_sleep_style_panel(scr, p, dim, "Sleep", t->panel, t->white);
}

static void build_rest_panel(lv_obj_t *scr, tod_mode_panel_t *p, bool dim)
{
    const ui_theme_t *t = ui_theme_get();
    build_sleep_style_panel(scr, p, dim, "Rest", t->ring, t->secondary);
}

static void build_screen(lv_obj_t **scr, tod_mode_panel_t *panels, bool dim)
{
    *scr = ui_widgets_create_screen();

    build_wake_panel(*scr, &panels[APP_MODE_WAKE], dim);
    build_wind_down_panel(*scr, &panels[APP_MODE_WIND_DOWN], dim);
    build_sleep_panel(*scr, &panels[APP_MODE_SLEEP], dim);
    build_rest_panel(*scr, &panels[APP_MODE_REST], dim);

    for (int i = 0; i < TOD_MODE_COUNT; i++) {
        lv_obj_add_flag(panels[i].root, LV_OBJ_FLAG_HIDDEN);
    }

    lv_obj_add_event_cb(*scr, screen_tap_cb, LV_EVENT_CLICKED, NULL);
}

static void apply_mode(bool dim)
{
    app_runtime_t *rt = app_runtime_get();
    app_mode_t mode = rt->current_mode;
    if (mode >= TOD_MODE_COUNT) {
        mode = APP_MODE_WAKE;
    }

    tod_mode_panel_t *all = dim ? s_panels_dim : s_panels_bright;

    for (int i = 0; i < TOD_MODE_COUNT; i++) {
        if (all[i].root == NULL) {
            continue;
        }
        if ((app_mode_t)i == mode) {
            lv_obj_remove_flag(all[i].root, LV_OBJ_FLAG_HIDDEN);
            refresh_panel(&all[i], mode, dim);
        } else {
            lv_obj_add_flag(all[i].root, LV_OBJ_FLAG_HIDDEN);
        }
    }

    s_showing_dim = dim;
}

void ui_screen_tod_build(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    build_screen(&s_scr_bright, s_panels_bright, false);
    build_screen(&s_scr_dim, s_panels_dim, true);
    screens[UI_SCREEN_TOD_BRIGHT] = s_scr_bright;
    screens[UI_SCREEN_TOD_DIM] = s_scr_dim;
}

void ui_screen_tod_on_show(bool dim)
{
    apply_mode(dim);
    ui_nav_apply_dim(dim);
}

void ui_screen_tod_tick(void)
{
    app_runtime_t *rt = app_runtime_get();
    app_mode_t mode = rt->current_mode;
    if (mode >= TOD_MODE_COUNT) {
        mode = APP_MODE_WAKE;
    }
    tod_mode_panel_t *p = panel_for(mode, s_showing_dim);
    if (p->root == NULL || lv_obj_has_flag(p->root, LV_OBJ_FLAG_HIDDEN)) {
        return;
    }
    refresh_panel_time(p);
    refresh_panel_remaining(p, mode);
}

static void apply_theme_to_panel(tod_mode_panel_t *p, app_mode_t mode)
{
    const ui_theme_t *t = ui_theme_get();

    if (p->menu_btn != NULL) {
        lv_obj_set_style_bg_color(p->menu_btn, t->ring, 0);
    }
    switch (mode) {
    case APP_MODE_WIND_DOWN:
        if (p->lbl_subtitle != NULL) {
            lv_obj_set_style_text_color(p->lbl_subtitle, t->secondary, 0);
        }
        break;
    case APP_MODE_REST:
        if (p->root != NULL) {
            lv_obj_t *box = lv_obj_get_child(p->root, 0);
            if (box != NULL) {
                lv_obj_set_style_bg_color(box, t->ring, 0);
            }
        }
        if (p->lbl_remaining != NULL) {
            lv_obj_set_style_text_color(p->lbl_remaining, t->secondary, 0);
        }
        break;
    default:
        break;
    }
}

void ui_screen_tod_apply_theme(void)
{
    if (s_scr_bright != NULL) {
        ui_widgets_style_circle_panel(s_scr_bright);
    }
    if (s_scr_dim != NULL) {
        ui_widgets_style_circle_panel(s_scr_dim);
    }
    for (int i = 0; i < TOD_MODE_COUNT; i++) {
        apply_theme_to_panel(&s_panels_bright[i], (app_mode_t)i);
        apply_theme_to_panel(&s_panels_dim[i], (app_mode_t)i);
    }
}
