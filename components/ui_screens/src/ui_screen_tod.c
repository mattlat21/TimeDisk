/**
 * @file ui_screen_tod.c
 * @brief Time of Day bright/dim screens with per-mode layouts (Wake, Wind Down, Sleep, Rest).
 */

#include "ui_screens_registry.h"
#include "ui_layout.h"
#include "ui_widgets.h"
#include "ui_wedge.h"
#include "ui_format.h"
#include "ui_theme.h"
#include "ui_nav.h"
#include "ui_assets.h"
#include <stdio.h>
#include <string.h>

#define TOD_MODE_COUNT 4

typedef struct {
    lv_obj_t *root;
    lv_obj_t *lbl_time;
    lv_obj_t *lbl_remaining;
} tod_mode_panel_t;

static lv_obj_t *s_scr_bright;
static lv_obj_t *s_scr_dim;
static lv_obj_t *s_bg_bright;
static lv_obj_t *s_bg_dim;
static ui_wedge_t *s_menu_wedge_bright;
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

static const lv_image_dsc_t *mode_image(app_mode_t mode)
{
    switch (mode) {
    case APP_MODE_WAKE:
        return &tod_wake;
    case APP_MODE_WIND_DOWN:
        return &tod_winddown;
    case APP_MODE_SLEEP:
        return &tod_sleep;
    case APP_MODE_REST:
        return &tod_rest;
    default:
        return &tod_wake;
    }
}

static lv_opa_t dim_blend_opa(uint8_t blend)
{
    return (lv_opa_t)(LV_OPA_COVER - (uint32_t)(LV_OPA_COVER - LV_OPA_60) * blend / 255U);
}

static void apply_mode_background(lv_obj_t *bg, app_mode_t mode, uint8_t blend)
{
    if (bg == NULL) {
        return;
    }
    lv_image_set_src(bg, mode_image(mode));
    lv_obj_set_style_opa(bg, dim_blend_opa(blend), 0);
}

static lv_obj_t *create_mode_background(lv_obj_t *scr)
{
    lv_obj_t *img = lv_image_create(scr);
    lv_image_set_src(img, &tod_wake);
    lv_obj_set_size(img, UI_DISP, UI_DISP);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
    lv_obj_remove_flag(img, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(img, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_move_background(img);
    return img;
}

static void screen_tap_cb(lv_event_t *e)
{
    (void)e;
    ui_screen_id_t cur = ui_nav_current();
    if (cur == UI_SCREEN_TOD_DIM) {
        ui_nav_tod_fade_to_bright();
    } else if (cur == UI_SCREEN_TOD_BRIGHT) {
        ui_nav_reset_idle_timer();
    }
}

static void menu_btn_cb(lv_event_t *e)
{
    (void)e;
    ui_nav_start_aa(UI_SCREEN_TOD_BRIGHT, UI_SCREEN_MENU);
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
    lv_obj_remove_flag(root, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(root, LV_OBJ_FLAG_EVENT_BUBBLE);
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
    if (!rt->cycle_active || rt->mode_remaining_sec == 0) {
        lv_obj_add_flag(p->lbl_remaining, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    char time_buf[16];

    ui_format_mm_ss(time_buf, sizeof(time_buf), rt->mode_remaining_sec);
    lv_label_set_text(p->lbl_remaining, time_buf);
    lv_obj_remove_flag(p->lbl_remaining, LV_OBJ_FLAG_HIDDEN);
}

static void apply_dim_styles(const tod_mode_panel_t *p, uint8_t blend)
{
    lv_opa_t opa = dim_blend_opa(blend);
    if (p->lbl_time != NULL) {
        lv_obj_set_style_text_opa(p->lbl_time, opa, 0);
    }
    if (p->lbl_remaining != NULL) {
        lv_obj_set_style_text_opa(p->lbl_remaining, opa, 0);
    }
}

static void refresh_panel(const tod_mode_panel_t *p, app_mode_t mode, uint8_t blend)
{
    refresh_panel_time(p);
    refresh_panel_remaining(p, mode);
    apply_dim_styles(p, blend);
}

static void build_mode_panel(lv_obj_t *scr, tod_mode_panel_t *p)
{
    const ui_theme_t *t = ui_theme_get();

    p->root = create_panel_root(scr);

    p->lbl_time = lv_label_create(p->root);
    lv_obj_set_style_text_color(p->lbl_time, t->white, 0);
    lv_obj_set_style_text_font(p->lbl_time, &lv_font_montserrat_48, 0);
    lv_obj_align(p->lbl_time, LV_ALIGN_CENTER, 0, -20);

    p->lbl_remaining = lv_label_create(p->root);
    lv_obj_set_style_text_color(p->lbl_remaining, t->orange, 0);
    lv_obj_set_style_text_font(p->lbl_remaining, &lv_font_montserrat_20, 0);
    lv_obj_align(p->lbl_remaining, LV_ALIGN_CENTER, 0, 40);
}

static void build_screen(lv_obj_t **scr, lv_obj_t **bg, tod_mode_panel_t *panels, bool dim)
{
    *scr = ui_widgets_create_screen_no_ring();
    *bg = create_mode_background(*scr);

    for (int i = 0; i < TOD_MODE_COUNT; i++) {
        build_mode_panel(*scr, &panels[i]);
    }

    for (int i = 0; i < TOD_MODE_COUNT; i++) {
        lv_obj_add_flag(panels[i].root, LV_OBJ_FLAG_HIDDEN);
    }

    lv_obj_add_flag(*scr, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(*scr, screen_tap_cb, LV_EVENT_CLICKED, NULL);

    if (!dim) {
        s_menu_wedge_bright = ui_wedge_create_overlay(*scr, UI_WEDGE_MENU);
        if (s_menu_wedge_bright != NULL) {
            ui_wedge_bind(s_menu_wedge_bright, UI_WEDGE_MENU, menu_btn_cb, NULL);
            ui_wedge_set_label(s_menu_wedge_bright, "Menu");
        }
    }

    if (*bg != NULL) {
        lv_obj_move_background(*bg);
    }
}

static void apply_mode(bool dim)
{
    app_runtime_t *rt = app_runtime_get();
    app_mode_t mode = rt->current_mode;
    if (mode >= TOD_MODE_COUNT) {
        mode = APP_MODE_WAKE;
    }

    uint8_t blend = dim ? 255U : 0U;

    apply_mode_background(s_bg_bright, mode, blend);
    apply_mode_background(s_bg_dim, mode, blend);

    tod_mode_panel_t *all = dim ? s_panels_dim : s_panels_bright;

    for (int i = 0; i < TOD_MODE_COUNT; i++) {
        if (all[i].root == NULL) {
            continue;
        }
        if ((app_mode_t)i == mode) {
            lv_obj_remove_flag(all[i].root, LV_OBJ_FLAG_HIDDEN);
            refresh_panel(&all[i], mode, blend);
        } else {
            lv_obj_add_flag(all[i].root, LV_OBJ_FLAG_HIDDEN);
        }
    }

    s_showing_dim = dim;
}

void ui_screen_tod_set_menu_visible(bool visible)
{
    if (s_menu_wedge_bright != NULL) {
        ui_wedge_set_visible(s_menu_wedge_bright, visible);
    }
}

void ui_screen_tod_apply_dim_blend(uint8_t blend, bool on_dim_screen)
{
    app_runtime_t *rt = app_runtime_get();
    app_mode_t mode = rt->current_mode;
    if (mode >= TOD_MODE_COUNT) {
        mode = APP_MODE_WAKE;
    }

    tod_mode_panel_t *p = panel_for(mode, on_dim_screen);
    if (p->root != NULL && !lv_obj_has_flag(p->root, LV_OBJ_FLAG_HIDDEN)) {
        apply_dim_styles(p, blend);
    }

    if (on_dim_screen) {
        apply_mode_background(s_bg_dim, mode, blend);
    } else {
        apply_mode_background(s_bg_bright, mode, blend);
    }
}

void ui_screen_tod_build(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    build_screen(&s_scr_bright, &s_bg_bright, s_panels_bright, false);
    build_screen(&s_scr_dim, &s_bg_dim, s_panels_dim, true);
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

static void apply_theme_to_panel(tod_mode_panel_t *p)
{
    const ui_theme_t *t = ui_theme_get();

    if (p->lbl_time != NULL) {
        lv_obj_set_style_text_color(p->lbl_time, t->white, 0);
    }
    if (p->lbl_remaining != NULL) {
        lv_obj_set_style_text_color(p->lbl_remaining, t->orange, 0);
    }
}

void ui_screen_tod_apply_theme(void)
{
    if (s_scr_bright != NULL) {
        ui_widgets_style_circle_panel_no_ring(s_scr_bright);
    }
    if (s_scr_dim != NULL) {
        ui_widgets_style_circle_panel_no_ring(s_scr_dim);
    }
    for (int i = 0; i < TOD_MODE_COUNT; i++) {
        apply_theme_to_panel(&s_panels_bright[i]);
        apply_theme_to_panel(&s_panels_dim[i]);
    }
    if (s_menu_wedge_bright != NULL) {
        ui_wedge_refresh_theme(s_menu_wedge_bright);
    }
    apply_mode(s_showing_dim);
}
