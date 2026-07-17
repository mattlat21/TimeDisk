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
#include "app_config.h"
#include "app_scheduled_button.h"

#define TOD_MODE_COUNT 4

/** Top-center scheduled action button (wireframe coords on 720 circle). */
#define SCHED_BTN_D_WF 180
#define SCHED_BTN_TOP_WF 36

typedef struct {
    lv_obj_t *row;
    lv_obj_t *hm;
    lv_obj_t *ampm;
} tod_clock_t;

static lv_obj_t *s_scr_bright;
static lv_obj_t *s_scr_dim;
static lv_obj_t *s_bg_bright;
static lv_obj_t *s_bg_dim;
static ui_wedge_t *s_menu_wedge_bright;
static lv_obj_t *s_sched_btn;
static lv_obj_t *s_sched_btn_lbl;
static uint8_t s_sched_btn_action;
static bool s_menu_idle_visible = true;
static tod_clock_t s_clock_bright;
static tod_clock_t s_clock_dim;
static bool s_showing_dim;

static const char *mode_image(app_mode_t mode)
{
    switch (mode) {
    case APP_MODE_WAKE:
        return ui_assets_spiffs_path("tod_wake");
    case APP_MODE_WIND_DOWN:
        return ui_assets_spiffs_path("tod_winddown");
    case APP_MODE_SLEEP:
        return ui_assets_spiffs_path("tod_sleep");
    case APP_MODE_REST:
        return ui_assets_spiffs_path("tod_rest");
    default:
        return ui_assets_spiffs_path("tod_wake");
    }
}

static void apply_mode_background(lv_obj_t *bg, app_mode_t mode)
{
    if (bg == NULL) {
        return;
    }
    lv_image_set_src(bg, mode_image(mode));
    lv_obj_set_style_opa(bg, LV_OPA_COVER, 0);
}

static lv_obj_t *create_mode_background(lv_obj_t *scr)
{
    lv_obj_t *img = lv_image_create(scr);
    lv_image_set_src(img, ui_assets_spiffs_path("tod_wake"));
    lv_obj_set_size(img, UI_DISP, UI_DISP);
    lv_image_set_inner_align(img, LV_IMAGE_ALIGN_STRETCH);
    lv_image_set_antialias(img, false);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);
    lv_obj_remove_flag(img, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(img, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_move_background(img);
    return img;
}

static void screen_tap_cb(lv_event_t *e)
{
    (void)e;
    ui_nav_tod_wake();
}

static void menu_btn_cb(lv_event_t *e)
{
    (void)e;
    ui_nav_start_aa(UI_SCREEN_TOD_BRIGHT, UI_SCREEN_MENU);
}

static void sched_btn_cb(lv_event_t *e)
{
    (void)e;
    ui_nav_start_aa_scheduled_button(s_sched_btn_action);
}

static void layout_ampm(tod_clock_t *clock)
{
    if (clock->hm == NULL || clock->ampm == NULL) {
        return;
    }

    lv_obj_update_layout(clock->hm);
    const int32_t hm_w = lv_obj_get_width(clock->hm);
    const int32_t hm_h = lv_obj_get_height(clock->hm);
    const int32_t gap = 18;

    /* HM uses 2x scale from center; visual bounds extend hm_w/2 past layout edges. */
    lv_obj_align_to(clock->ampm, clock->hm, LV_ALIGN_OUT_RIGHT_BOTTOM, hm_w / 2 + gap, hm_h / 2 - 12);
}

static void create_clock(lv_obj_t *scr, tod_clock_t *clock)
{
    const ui_theme_t *t = ui_theme_get();

    lv_obj_add_flag(scr, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

    clock->row = lv_obj_create(scr);
    lv_obj_remove_flag(clock->row, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(clock->row, LV_OBJ_FLAG_EVENT_BUBBLE | LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_set_size(clock->row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(clock->row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(clock->row, 0, 0);
    lv_obj_set_style_pad_all(clock->row, 0, 0);
    lv_obj_set_style_pad_top(clock->row, 28, 0);
    lv_obj_set_style_pad_bottom(clock->row, 28, 0);
    lv_obj_set_style_pad_left(clock->row, 72, 0);
    lv_obj_set_style_pad_right(clock->row, 72, 0);
    lv_obj_align(clock->row, LV_ALIGN_CENTER, 0, 0);

    clock->hm = lv_label_create(clock->row);
    lv_label_set_text(clock->hm, "--:--");
    lv_obj_set_style_text_color(clock->hm, t->white, 0);
    lv_obj_set_style_text_font(clock->hm, &lv_font_montserrat_48, 0);
    lv_obj_set_style_transform_scale_x(clock->hm, 512, 0);
    lv_obj_set_style_transform_scale_y(clock->hm, 512, 0);
    lv_obj_align(clock->hm, LV_ALIGN_CENTER, 0, 0);

    clock->ampm = lv_label_create(clock->row);
    lv_label_set_text(clock->ampm, "");
    lv_obj_set_style_text_color(clock->ampm, t->white, 0);
    lv_obj_set_style_text_font(clock->ampm, &lv_font_montserrat_34, 0);
    layout_ampm(clock);
}

static void refresh_clock(tod_clock_t *clock)
{
    if (clock->row == NULL) {
        return;
    }

    app_runtime_t *rt = app_runtime_get();
    if (!rt->time_valid) {
        lv_obj_add_flag(clock->row, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    char hm[16];
    char ampm[8];
    ui_format_hh_mm_ampm_parts_now(hm, sizeof(hm), ampm, sizeof(ampm));
    lv_label_set_text(clock->hm, hm);
    lv_label_set_text(clock->ampm, ampm);
    lv_obj_remove_flag(clock->row, LV_OBJ_FLAG_HIDDEN);
    if (clock->hm != NULL) {
        lv_obj_set_style_text_opa(clock->hm, LV_OPA_COVER, 0);
    }
    if (clock->ampm != NULL) {
        lv_obj_set_style_text_opa(clock->ampm, LV_OPA_COVER, 0);
    }

    lv_obj_update_layout(clock->hm);
    lv_obj_set_style_transform_pivot_x(clock->hm, lv_obj_get_width(clock->hm) / 2, 0);
    lv_obj_set_style_transform_pivot_y(clock->hm, lv_obj_get_height(clock->hm) / 2, 0);
    layout_ampm(clock);
}

static void create_scheduled_button(lv_obj_t *scr)
{
    const ui_theme_t *t = ui_theme_get();
    const int x_wf = (UI_DISP - SCHED_BTN_D_WF) / 2;
    int x = 0;
    int y = 0;

    ui_layout_screen_pos_from_wf(scr, x_wf, SCHED_BTN_TOP_WF, &x, &y);

    s_sched_btn = lv_button_create(scr);
    lv_obj_set_size(s_sched_btn, SCHED_BTN_D_WF, SCHED_BTN_D_WF);
    lv_obj_set_pos(s_sched_btn, x, y);
    lv_obj_set_style_radius(s_sched_btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_sched_btn, t->ring, 0);
    lv_obj_set_style_bg_opa(s_sched_btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_sched_btn, 0, 0);
    lv_obj_set_style_shadow_width(s_sched_btn, 0, 0);
    lv_obj_set_style_pad_all(s_sched_btn, 12, 0);
    lv_obj_add_flag(s_sched_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(s_sched_btn, sched_btn_cb, LV_EVENT_CLICKED, NULL);

    s_sched_btn_lbl = lv_label_create(s_sched_btn);
    lv_label_set_text(s_sched_btn_lbl, "");
    lv_label_set_long_mode(s_sched_btn_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_sched_btn_lbl, SCHED_BTN_D_WF - 24);
    lv_obj_set_style_text_color(s_sched_btn_lbl, t->white, 0);
    lv_obj_set_style_text_font(s_sched_btn_lbl, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_align(s_sched_btn_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(s_sched_btn_lbl);
}

void ui_screen_tod_refresh_scheduled_button(void)
{
    if (s_sched_btn == NULL) {
        return;
    }

    const app_scheduled_button_t *btn = NULL;
    if (s_menu_idle_visible && !s_showing_dim) {
        btn = app_scheduled_button_active();
    }

    if (btn == NULL) {
        lv_obj_add_flag(s_sched_btn, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    const char *label = app_scheduled_button_label(btn->action);
    if (label == NULL) {
        lv_obj_add_flag(s_sched_btn, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    s_sched_btn_action = btn->action;
    if (s_sched_btn_lbl != NULL) {
        if (btn->action == APP_SCHEDULE_ACTION_START_WIND_DOWN) {
            lv_label_set_text(s_sched_btn_lbl, "Start\nWind Down");
        } else {
            lv_label_set_text(s_sched_btn_lbl, label);
        }
    }
    lv_obj_remove_flag(s_sched_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_sched_btn);
}

static void build_screen(lv_obj_t **scr, lv_obj_t **bg, tod_clock_t *clock, bool dim)
{
    *scr = ui_widgets_create_screen_no_ring();
    *bg = create_mode_background(*scr);
    create_clock(*scr, clock);

    lv_obj_add_flag(*scr, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(*scr, screen_tap_cb, LV_EVENT_CLICKED, NULL);

    if (!dim) {
        s_menu_wedge_bright = ui_wedge_create_overlay(*scr, UI_WEDGE_MENU);
        if (s_menu_wedge_bright != NULL) {
            ui_wedge_bind(s_menu_wedge_bright, UI_WEDGE_MENU, menu_btn_cb, NULL);
            ui_wedge_set_label(s_menu_wedge_bright, "Menu");
        }
        create_scheduled_button(*scr);
    }

    if (*bg != NULL) {
        lv_obj_move_background(*bg);
    }
    if (clock->row != NULL) {
        lv_obj_move_foreground(clock->row);
    }
    if (!dim && s_sched_btn != NULL) {
        lv_obj_move_foreground(s_sched_btn);
    }
}

static void apply_mode(bool dim)
{
    app_runtime_t *rt = app_runtime_get();
    app_mode_t mode = rt->current_mode;
    if (mode >= TOD_MODE_COUNT) {
        mode = APP_MODE_WAKE;
    }

    apply_mode_background(s_bg_bright, mode);
    apply_mode_background(s_bg_dim, mode);
    refresh_clock(&s_clock_bright);
    refresh_clock(&s_clock_dim);

    s_showing_dim = dim;
    ui_screen_tod_refresh_scheduled_button();
}

void ui_screen_tod_set_menu_visible(bool visible)
{
    s_menu_idle_visible = visible;
    if (s_menu_wedge_bright != NULL) {
        ui_wedge_set_visible(s_menu_wedge_bright, visible);
    }
    ui_screen_tod_refresh_scheduled_button();
}

void ui_screen_tod_build(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    build_screen(&s_scr_bright, &s_bg_bright, &s_clock_bright, false);
    build_screen(&s_scr_dim, &s_bg_dim, &s_clock_dim, true);
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
    tod_clock_t *clock = s_showing_dim ? &s_clock_dim : &s_clock_bright;
    refresh_clock(clock);
    ui_screen_tod_refresh_scheduled_button();
}

static void apply_theme_to_clock(tod_clock_t *clock)
{
    const ui_theme_t *t = ui_theme_get();

    if (clock->hm != NULL) {
        lv_obj_set_style_text_color(clock->hm, t->white, 0);
    }
    if (clock->ampm != NULL) {
        lv_obj_set_style_text_color(clock->ampm, t->white, 0);
        lv_obj_set_style_text_font(clock->ampm, &lv_font_montserrat_34, 0);
    }
}

void ui_screen_tod_apply_theme(void)
{
    const ui_theme_t *t = ui_theme_get();

    if (s_scr_bright != NULL) {
        ui_widgets_style_circle_panel_no_ring(s_scr_bright);
    }
    if (s_scr_dim != NULL) {
        ui_widgets_style_circle_panel_no_ring(s_scr_dim);
    }
    apply_theme_to_clock(&s_clock_bright);
    apply_theme_to_clock(&s_clock_dim);
    if (s_menu_wedge_bright != NULL) {
        ui_wedge_refresh_theme(s_menu_wedge_bright);
    }
    if (s_sched_btn != NULL) {
        lv_obj_set_style_bg_color(s_sched_btn, t->ring, 0);
    }
    if (s_sched_btn_lbl != NULL) {
        lv_obj_set_style_text_color(s_sched_btn_lbl, t->white, 0);
    }
    apply_mode(s_showing_dim);
}
