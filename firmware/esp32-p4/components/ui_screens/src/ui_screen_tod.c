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

#define TOD_MODE_COUNT 4

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
    lv_image_set_src(img, ui_assets_spiffs_path("tod_wake"));
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
    lv_obj_align_to(clock->ampm, clock->hm, LV_ALIGN_OUT_RIGHT_BOTTOM, hm_w / 2 + gap, hm_h / 2);
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

static void apply_clock_dim_styles(tod_clock_t *clock, uint8_t blend)
{
    if (clock->row == NULL) {
        return;
    }
    lv_opa_t opa = dim_blend_opa(blend);
    if (clock->hm != NULL) {
        lv_obj_set_style_text_opa(clock->hm, opa, 0);
    }
    if (clock->ampm != NULL) {
        lv_obj_set_style_text_opa(clock->ampm, opa, 0);
    }
}

static void refresh_clock(tod_clock_t *clock, uint8_t blend)
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
    apply_clock_dim_styles(clock, blend);

    lv_obj_update_layout(clock->hm);
    lv_obj_set_style_transform_pivot_x(clock->hm, lv_obj_get_width(clock->hm) / 2, 0);
    lv_obj_set_style_transform_pivot_y(clock->hm, lv_obj_get_height(clock->hm) / 2, 0);
    layout_ampm(clock);
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
    }

    if (*bg != NULL) {
        lv_obj_move_background(*bg);
    }
    if (clock->row != NULL) {
        lv_obj_move_foreground(clock->row);
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
    refresh_clock(&s_clock_bright, blend);
    refresh_clock(&s_clock_dim, blend);

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

    tod_clock_t *clock = on_dim_screen ? &s_clock_dim : &s_clock_bright;
    apply_clock_dim_styles(clock, blend);

    if (on_dim_screen) {
        apply_mode_background(s_bg_dim, mode, blend);
    } else {
        apply_mode_background(s_bg_bright, mode, blend);
    }
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
    uint8_t blend = s_showing_dim ? 255U : 0U;
    refresh_clock(clock, blend);
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
    apply_mode(s_showing_dim);
}
