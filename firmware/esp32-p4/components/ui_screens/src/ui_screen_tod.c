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
#include "ui_spiffs_pixelart.h"
#include "app_config.h"
#include "app_scheduled_button.h"

#include <stdio.h>
#include <time.h>

#define TOD_MODE_COUNT 4

/** Top-center scheduled action button (wireframe coords on 720 circle). */
#define SCHED_BTN_D_WF 180
#define SCHED_BTN_TOP_WF 36
#define SCHED_BTN_BORDER_PX 10
/** Offset of remaining subtitle below the scaled clock (content coords). */
#define TOD_REMAINING_Y_OFFSET 78

typedef struct {
    lv_obj_t *row;
    lv_obj_t *hm;
    lv_obj_t *ampm;
    lv_obj_t *remaining;
} tod_clock_t;

static lv_obj_t *s_scr_bright;
static lv_obj_t *s_scr_dim;
static lv_obj_t *s_bg_bright;
static lv_obj_t *s_bg_dim;
static ui_wedge_t *s_menu_wedge_bright;
static lv_obj_t *s_sched_btn;
static lv_obj_t *s_sched_btn_img;
static lv_obj_t *s_sched_btn_ring;
static uint8_t s_sched_btn_action;
static bool s_menu_idle_visible = true;
static tod_clock_t s_clock_bright;
static tod_clock_t s_clock_dim;
static bool s_showing_dim;
/** Cached local minute (hour*60+min), or -1 when time is invalid / unknown. */
static int s_last_clock_min = -1;
static bool s_last_time_valid;
static char s_bg_path_bright[48];
static char s_bg_path_dim[48];
static char s_sched_btn_img_path[48];

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

/** Target schedule action → TOD art for the scheduled button preview. */
static const char *action_image(uint8_t action)
{
    switch ((app_schedule_action_t)action) {
    case APP_SCHEDULE_ACTION_WAKE:
        return ui_assets_spiffs_path("tod_wake");
    case APP_SCHEDULE_ACTION_START_WIND_DOWN:
        return ui_assets_spiffs_path("tod_winddown");
    case APP_SCHEDULE_ACTION_START_SLEEP:
        return ui_assets_spiffs_path("tod_sleep");
    case APP_SCHEDULE_ACTION_START_REST:
        return ui_assets_spiffs_path("tod_rest");
    default:
        return NULL;
    }
}

static void spiffs_img_draw_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_DRAW_MAIN) {
        return;
    }

    const char *path = lv_obj_get_user_data(lv_event_get_target(e));
    if (path == NULL || path[0] == '\0') {
        return;
    }

    lv_layer_t *layer = lv_event_get_layer(e);
    lv_obj_t *obj = lv_event_get_target(e);
    lv_area_t coords;
    lv_obj_get_coords(obj, &coords);
    ui_spiffs_pixelart_draw(layer, &coords, path);
}

static void spiffs_img_draw_cb_contain(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_DRAW_MAIN) {
        return;
    }

    const char *path = lv_obj_get_user_data(lv_event_get_target(e));
    if (path == NULL || path[0] == '\0') {
        return;
    }

    lv_layer_t *layer = lv_event_get_layer(e);
    lv_obj_t *obj = lv_event_get_target(e);
    lv_area_t coords;
    lv_obj_get_coords(obj, &coords);
    ui_spiffs_pixelart_draw_contain(layer, &coords, path);
}

static void apply_mode_background(lv_obj_t *bg, char *path_buf, app_mode_t mode)
{
    if (bg == NULL || path_buf == NULL) {
        return;
    }

    const char *path = mode_image(mode);
    snprintf(path_buf, 48, "%s", path);
    lv_obj_set_user_data(bg, path_buf);
    ui_spiffs_pixelart_cache_preload(path);
    lv_obj_invalidate(bg);
}

static lv_obj_t *create_mode_background(lv_obj_t *scr, char *path_buf)
{
    lv_obj_t *bg = lv_obj_create(scr);
    lv_obj_remove_style_all(bg);
    lv_obj_set_size(bg, UI_DISP, UI_DISP);
    lv_obj_align(bg, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(bg, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(bg, 0, 0);
    lv_obj_set_style_pad_all(bg, 0, 0);
    lv_obj_remove_flag(bg, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(bg, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(bg, spiffs_img_draw_cb, LV_EVENT_DRAW_MAIN, NULL);
    snprintf(path_buf, 48, "%s", ui_assets_spiffs_path("tod_wake"));
    lv_obj_set_user_data(bg, path_buf);
    lv_obj_move_background(bg);
    return bg;
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

    clock->remaining = lv_label_create(scr);
    lv_label_set_text(clock->remaining, "");
    lv_obj_set_style_text_color(clock->remaining, t->white, 0);
    lv_obj_set_style_text_font(clock->remaining, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_align(clock->remaining, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(clock->remaining, UI_CONTENT_W(0));
    lv_obj_align(clock->remaining, LV_ALIGN_CENTER, 0, TOD_REMAINING_Y_OFFSET);
    lv_obj_add_flag(clock->remaining, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(clock->remaining, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(clock->remaining, LV_OBJ_FLAG_EVENT_BUBBLE);
}

static const char *mode_display_name(app_mode_t mode)
{
    switch (mode) {
    case APP_MODE_WIND_DOWN:
        return "Wind Down";
    case APP_MODE_SLEEP:
        return "Sleep";
    case APP_MODE_REST:
        return "Rest";
    case APP_MODE_WAKE:
    default:
        return "Wake";
    }
}

static void refresh_remaining(tod_clock_t *clock)
{
    if (clock == NULL || clock->remaining == NULL) {
        return;
    }

    const app_config_t *cfg = app_config_get();
    if (!cfg->tod_remaining_enabled) {
        lv_obj_add_flag(clock->remaining, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    if (s_showing_dim && !cfg->tod_remaining_dim_enabled) {
        lv_obj_add_flag(clock->remaining, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    uint8_t next_mode = APP_MODE_WAKE;
    uint32_t remaining_sec = 0;
    if (!ui_nav_tod_next_transition(&next_mode, &remaining_sec) || remaining_sec == 0) {
        lv_obj_add_flag(clock->remaining, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    if (cfg->tod_remaining_threshold_enabled &&
        remaining_sec >= cfg->tod_remaining_threshold_sec) {
        lv_obj_add_flag(clock->remaining, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    char time_buf[16];
    char line[48];
    ui_format_countdown_xx_yy(time_buf, sizeof(time_buf), remaining_sec);
    snprintf(line, sizeof(line), "%s in %s", mode_display_name((app_mode_t)next_mode), time_buf);
    lv_label_set_text(clock->remaining, line);
    lv_obj_clear_flag(clock->remaining, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align(clock->remaining, LV_ALIGN_CENTER, 0, TOD_REMAINING_Y_OFFSET);
}

static void refresh_clock(tod_clock_t *clock)
{
    if (clock->row == NULL) {
        return;
    }

    app_runtime_t *rt = app_runtime_get();
    if (!rt->time_valid) {
        lv_obj_add_flag(clock->row, LV_OBJ_FLAG_HIDDEN);
        if (clock->remaining != NULL) {
            lv_obj_add_flag(clock->remaining, LV_OBJ_FLAG_HIDDEN);
        }
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
    refresh_remaining(clock);
}

static void create_scheduled_button(lv_obj_t *scr)
{
    const ui_theme_t *t = ui_theme_get();
    const int x_wf = (UI_DISP - SCHED_BTN_D_WF) / 2;
    int x = 0;
    int y = 0;

    ui_layout_screen_pos_from_wf(scr, x_wf, SCHED_BTN_TOP_WF, &x, &y);

    /* Image fills the full circle (parent clip). Ring is a border overlay on top — no inset clip. */
    s_sched_btn = lv_obj_create(scr);
    lv_obj_remove_style_all(s_sched_btn);
    lv_obj_set_size(s_sched_btn, SCHED_BTN_D_WF, SCHED_BTN_D_WF);
    lv_obj_set_pos(s_sched_btn, x, y);
    lv_obj_set_style_radius(s_sched_btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_clip_corner(s_sched_btn, true, 0);
    lv_obj_set_style_bg_opa(s_sched_btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(s_sched_btn, 0, 0);
    lv_obj_add_flag(s_sched_btn, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(s_sched_btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s_sched_btn, sched_btn_cb, LV_EVENT_CLICKED, NULL);

    s_sched_btn_img = lv_obj_create(s_sched_btn);
    lv_obj_remove_style_all(s_sched_btn_img);
    lv_obj_set_size(s_sched_btn_img, SCHED_BTN_D_WF, SCHED_BTN_D_WF);
    lv_obj_set_pos(s_sched_btn_img, 0, 0);
    lv_obj_set_style_bg_opa(s_sched_btn_img, LV_OPA_TRANSP, 0);
    lv_obj_remove_flag(s_sched_btn_img, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_sched_btn_img, spiffs_img_draw_cb_contain, LV_EVENT_DRAW_MAIN, NULL);
    snprintf(s_sched_btn_img_path, sizeof(s_sched_btn_img_path), "%s", ui_assets_spiffs_path("tod_wake"));
    lv_obj_set_user_data(s_sched_btn_img, s_sched_btn_img_path);

    s_sched_btn_ring = lv_obj_create(s_sched_btn);
    lv_obj_remove_style_all(s_sched_btn_ring);
    lv_obj_set_size(s_sched_btn_ring, SCHED_BTN_D_WF, SCHED_BTN_D_WF);
    lv_obj_set_pos(s_sched_btn_ring, 0, 0);
    lv_obj_set_style_radius(s_sched_btn_ring, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(s_sched_btn_ring, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_sched_btn_ring, SCHED_BTN_BORDER_PX, 0);
    lv_obj_set_style_border_color(s_sched_btn_ring, t->ring, 0);
    lv_obj_set_style_border_opa(s_sched_btn_ring, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(s_sched_btn_ring, 0, 0);
    lv_obj_remove_flag(s_sched_btn_ring, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_move_foreground(s_sched_btn_ring);
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

    const char *img = action_image(btn->action);
    if (img == NULL) {
        lv_obj_add_flag(s_sched_btn, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    s_sched_btn_action = btn->action;
    if (s_sched_btn_img != NULL) {
        snprintf(s_sched_btn_img_path, sizeof(s_sched_btn_img_path), "%s", img);
        lv_obj_set_user_data(s_sched_btn_img, s_sched_btn_img_path);
        ui_spiffs_pixelart_cache_preload(img);
        lv_obj_invalidate(s_sched_btn_img);
    }
    lv_obj_remove_flag(s_sched_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(s_sched_btn);
}

static void build_screen(lv_obj_t **scr, lv_obj_t **bg, char *path_buf, tod_clock_t *clock, bool dim)
{
    *scr = ui_widgets_create_screen_no_ring();
    *bg = create_mode_background(*scr, path_buf);
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
    if (clock->remaining != NULL) {
        lv_obj_move_foreground(clock->remaining);
    }
    if (!dim && s_sched_btn != NULL) {
        lv_obj_move_foreground(s_sched_btn);
    }
}

static void note_clock_minute(void)
{
    const app_runtime_t *rt = app_runtime_get();
    s_last_time_valid = rt->time_valid;
    if (!s_last_time_valid) {
        s_last_clock_min = -1;
        return;
    }

    const time_t now = time(NULL);
    struct tm tm_local;
    localtime_r(&now, &tm_local);
    s_last_clock_min = tm_local.tm_hour * 60 + tm_local.tm_min;
}

static void apply_mode(bool dim)
{
    app_runtime_t *rt = app_runtime_get();
    app_mode_t mode = rt->current_mode;
    if (mode >= TOD_MODE_COUNT) {
        mode = APP_MODE_WAKE;
    }

    apply_mode_background(s_bg_bright, s_bg_path_bright, mode);
    apply_mode_background(s_bg_dim, s_bg_path_dim, mode);
    refresh_clock(&s_clock_bright);
    refresh_clock(&s_clock_dim);

    s_showing_dim = dim;
    ui_screen_tod_refresh_scheduled_button();
    note_clock_minute();
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
    build_screen(&s_scr_bright, &s_bg_bright, s_bg_path_bright, &s_clock_bright, false);
    build_screen(&s_scr_dim, &s_bg_dim, s_bg_path_dim, &s_clock_dim, true);
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
    /* Clock and scheduled button are minute-granular; remaining subtitle updates every tick. */
    const app_runtime_t *rt = app_runtime_get();
    const bool valid = rt->time_valid;
    int cur_min = -1;
    if (valid) {
        const time_t now = time(NULL);
        struct tm tm_local;
        localtime_r(&now, &tm_local);
        cur_min = tm_local.tm_hour * 60 + tm_local.tm_min;
    }

    tod_clock_t *clock = s_showing_dim ? &s_clock_dim : &s_clock_bright;
    const bool minute_changed = !(valid == s_last_time_valid && cur_min == s_last_clock_min);
    if (minute_changed) {
        s_last_time_valid = valid;
        s_last_clock_min = cur_min;
        refresh_clock(clock);
        ui_screen_tod_refresh_scheduled_button();
    } else {
        refresh_remaining(clock);
    }
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
    if (clock->remaining != NULL) {
        lv_obj_set_style_text_color(clock->remaining, t->white, 0);
        lv_obj_set_style_text_font(clock->remaining, &lv_font_montserrat_22, 0);
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
    if (s_sched_btn_ring != NULL) {
        lv_obj_set_style_border_color(s_sched_btn_ring, t->ring, 0);
    }
    apply_mode(s_showing_dim);
}
