/**
 * @file ui_settings_live_timer.c
 * @brief Settings -> Live Timer Values debug panel (checkpoint ends and status).
 */

#include "ui_screen_settings_internal.h"

#include "app_checkpoint.h"
#include "ui_format.h"
#include "ui_theme.h"
#include "ui_widgets.h"

#include <esp_err.h>
#include <stdio.h>
#include <time.h>

#define LIVE_ROW_COUNT  16
#define LIVE_ROW_Y0_WF  90
#define LIVE_ROW_STEP   28
#define LIVE_LABEL_W    560

static lv_obj_t *s_panel;
static lv_obj_t *s_rows[LIVE_ROW_COUNT];
static lv_timer_t *s_refresh_timer;

static const char *mode_name(app_mode_t mode)
{
    switch (mode) {
    case APP_MODE_WAKE:
        return "Wake";
    case APP_MODE_WIND_DOWN:
        return "WindDown";
    case APP_MODE_SLEEP:
        return "Sleep";
    case APP_MODE_REST:
        return "Rest";
    default:
        return "?";
    }
}

static void format_end_line(char *buf, size_t len, const char *name, time_t end, bool time_valid)
{
    if (end == 0) {
        snprintf(buf, len, "%s: -", name);
        return;
    }

    if (time_valid) {
        struct tm tm_info;
        localtime_r(&end, &tm_info);
        int h24 = tm_info.tm_hour;
        int h12 = h24 % 12;
        if (h12 == 0) {
            h12 = 12;
        }
        const char *ampm = (h24 >= 12) ? "PM" : "AM";
        snprintf(buf, len, "%s: %ld (%d:%02d %s)", name, (long)end, h12, tm_info.tm_min, ampm);
    } else {
        snprintf(buf, len, "%s: %ld", name, (long)end);
    }
}

static void live_timer_refresh(void)
{
    app_checkpoint_ends_t ends;
    app_checkpoint_status_t status;
    app_runtime_t *rt = app_runtime_get();
    const bool time_ok = rt->time_valid;
    time_t now = time_ok ? time(NULL) : 0;

    if (app_checkpoint_load_ends(&ends) != ESP_OK) {
        ends.winddown_end = 0;
        ends.sleep_end = 0;
        ends.rest_end = 0;
        ends.timer_start = 0;
        ends.timer_end = 0;
    }
    app_checkpoint_status_at(&ends, now, &status);

    char buf[LIVE_LABEL_W];
    char rem[16];
    char now_buf[64];

    format_end_line(buf, sizeof(buf), "winddown_end", ends.winddown_end, time_ok);
    lv_label_set_text(s_rows[0], buf);

    format_end_line(buf, sizeof(buf), "sleep_end", ends.sleep_end, time_ok);
    lv_label_set_text(s_rows[1], buf);

    format_end_line(buf, sizeof(buf), "rest_end", ends.rest_end, time_ok);
    lv_label_set_text(s_rows[2], buf);

    format_end_line(buf, sizeof(buf), "timer_start", ends.timer_start, time_ok);
    lv_label_set_text(s_rows[3], buf);

    format_end_line(buf, sizeof(buf), "timer_end", ends.timer_end, time_ok);
    lv_label_set_text(s_rows[4], buf);

    ui_format_mm_ss(rem, sizeof(rem), status.cycle_remaining_sec);
    snprintf(buf, sizeof(buf), "cycle_active: %s  mode: %s  rem: %s",
             status.cycle_active ? "yes" : "no",
             mode_name(status.cycle_mode),
             time_ok ? rem : "--");
    lv_label_set_text(s_rows[5], buf);

    ui_format_mm_ss(rem, sizeof(rem), status.timer_remaining_sec);
    snprintf(buf, sizeof(buf), "timer_running: %s  rem: %s",
             status.timer_running ? "yes" : "no",
             time_ok ? rem : "--");
    lv_label_set_text(s_rows[6], buf);

    ui_format_mm_ss(rem, sizeof(rem), rt->mode_remaining_sec);
    snprintf(buf, sizeof(buf), "rt.cycle: %s  mode: %s  rem: %s",
             rt->cycle_active ? "yes" : "no",
             mode_name(rt->current_mode),
             rem);
    lv_label_set_text(s_rows[7], buf);

    ui_format_mm_ss(rem, sizeof(rem), rt->active_timer_remaining_sec);
    snprintf(buf, sizeof(buf), "rt.timer: %s  rem: %s",
             rt->timer_running ? "yes" : "no",
             rem);
    lv_label_set_text(s_rows[8], buf);

    if (time_ok) {
        char local[16];
        char ampm[8];
        ui_format_hh_mm_ampm_parts_now(local, sizeof(local), ampm, sizeof(ampm));
        snprintf(now_buf, sizeof(now_buf), "now: %ld  local: %s %s  time_valid: yes",
                 (long)now, local, ampm);
    } else {
        snprintf(now_buf, sizeof(now_buf), "now: --  time_valid: no");
    }
    lv_label_set_text(s_rows[9], now_buf);
}

static void refresh_timer_cb(lv_timer_t *t)
{
    (void)t;
    live_timer_refresh();
}

static void live_timer_noop_save_cb(lv_event_t *e)
{
    (void)e;
}

lv_obj_t *ui_settings_live_timer_build(void)
{
    const ui_theme_t *t = ui_theme_get();

    s_panel = ui_settings_create_sub_panel("Live Timer Values");
    lv_obj_add_flag(s_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(s_panel, LV_SCROLLBAR_MODE_AUTO);

    for (int i = 0; i < LIVE_ROW_COUNT; i++) {
        if (i >= 10) {
            break;
        }
        s_rows[i] = lv_label_create(s_panel);
        lv_label_set_text(s_rows[i], "--");
        lv_obj_set_style_text_color(s_rows[i], t->white, 0);
        lv_obj_set_style_text_font(s_rows[i], &lv_font_montserrat_14, 0);
        lv_obj_set_width(s_rows[i], LIVE_LABEL_W);
        lv_obj_align(s_rows[i], LV_ALIGN_TOP_MID, 0, ui_settings_wf_y(s_panel, LIVE_ROW_Y0_WF + i * LIVE_ROW_STEP));
    }

    ui_settings_attach_panel_wedges(s_panel, PANEL_LIVE_TIMER, live_timer_noop_save_cb);
    return s_panel;
}

void ui_settings_live_timer_on_show(void)
{
    live_timer_refresh();
    if (s_refresh_timer == NULL) {
        s_refresh_timer = lv_timer_create(refresh_timer_cb, 1000, NULL);
    } else {
        lv_timer_reset(s_refresh_timer);
        lv_timer_resume(s_refresh_timer);
    }
}

void ui_settings_live_timer_on_hide(void)
{
    if (s_refresh_timer != NULL) {
        lv_timer_pause(s_refresh_timer);
    }
}

void ui_settings_live_timer_apply_theme(void)
{
    const ui_theme_t *t = ui_theme_get();
    for (int i = 0; i < 10; i++) {
        if (s_rows[i] != NULL) {
            lv_obj_set_style_text_color(s_rows[i], t->white, 0);
        }
    }
}
