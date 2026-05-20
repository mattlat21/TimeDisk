/**
 * @file ui_screen_startup_timezone_wizard.c
 * @brief Boot-time timezone selection (docs/screen_flow.md startup_wizard_timezone).
 */

#include "ui_screens_registry.h"
#include "ui_layout.h"
#include "ui_widgets.h"
#include "ui_wedge.h"
#include "ui_theme.h"
#include "ui_format.h"
#include "ui_nav.h"
#include "app_config.h"
#include "app_time.h"
#include "timezone_catalog.h"
#include <stdio.h>
#include <string.h>

#define TZ_RING_BORDER      UI_RING_BORDER_WIFI
#define TZ_DROPDOWN_W       480
#define TZ_DROPDOWN_H       52
#define TZ_COUNTRY_Y_WF     132
#define TZ_LOCATION_Y_WF    208
#define TZ_TITLE_Y_OFFSET   28
#define TZ_OPTIONS_BUF_LEN  512

static lv_obj_t *s_scr;
static lv_obj_t *s_dd_country;
static lv_obj_t *s_dd_location;
static lv_obj_t *s_lbl_preview;
static lv_timer_t *s_preview_timer;
static char s_country_opts[TZ_OPTIONS_BUF_LEN];
static char s_location_opts[TZ_OPTIONS_BUF_LEN];
static size_t s_country_idx;
static size_t s_location_idx;

static void refresh_preview(void)
{
    if (s_lbl_preview == NULL) {
        return;
    }
    app_runtime_t *rt = app_runtime_get();
    char tbuf[20];
    if (!rt->time_valid) {
        snprintf(tbuf, sizeof(tbuf), "--:--");
    } else {
        ui_format_hh_mm_ampm_now(tbuf, sizeof(tbuf));
    }
    lv_label_set_text(s_lbl_preview, tbuf);
}

static void apply_selection_preview(void)
{
    const char *tz_id = timezone_catalog_timezone_id(s_country_idx, s_location_idx);
    if (tz_id[0] != '\0') {
        app_time_apply_timezone_id(tz_id);
    }
    refresh_preview();
}

static void sync_location_dropdown(size_t country_idx, size_t location_idx)
{
    if (timezone_catalog_format_location_options(country_idx, s_location_opts,
                                                 sizeof(s_location_opts)) == 0) {
        s_location_opts[0] = '\0';
    }
    lv_dropdown_set_options(s_dd_location, s_location_opts);
    if (location_idx >= timezone_catalog_location_count(country_idx)) {
        location_idx = 0;
    }
    lv_dropdown_set_selected(s_dd_location, location_idx);
    s_location_idx = location_idx;
}

static void select_default_dropdowns(void)
{
    size_t ci = 0;
    size_t li = 0;
    if (!timezone_catalog_default_selection(&ci, &li)) {
        ci = 0;
        li = 0;
    }
    s_country_idx = ci;
    s_location_idx = li;
    lv_dropdown_set_selected(s_dd_country, ci);
    sync_location_dropdown(ci, li);
}

static void location_changed_cb(lv_event_t *e)
{
    (void)e;
    ui_nav_reset_idle_timer();
    s_location_idx = lv_dropdown_get_selected(s_dd_location);
    apply_selection_preview();
}

static void country_changed_cb(lv_event_t *e)
{
    (void)e;
    ui_nav_reset_idle_timer();
    s_country_idx = lv_dropdown_get_selected(s_dd_country);
    sync_location_dropdown(s_country_idx, 0);
    apply_selection_preview();
}

static void preview_timer_cb(lv_timer_t *t)
{
    (void)t;
    refresh_preview();
}

static lv_obj_t *create_dropdown(lv_obj_t *parent, int y_wf)
{
    const ui_theme_t *th = ui_theme_get();
    lv_obj_t *dd = lv_dropdown_create(parent);
    lv_obj_set_size(dd, TZ_DROPDOWN_W, TZ_DROPDOWN_H);
    lv_obj_set_pos(dd,
                   UI_WF_X((UI_SCREEN_W - TZ_DROPDOWN_W) / 2, TZ_RING_BORDER),
                   UI_WF_Y(y_wf, TZ_RING_BORDER));
    lv_obj_set_style_bg_color(dd, th->ring, 0);
    lv_obj_set_style_text_color(dd, th->white, 0);
    lv_obj_set_style_border_width(dd, 0, 0);
    lv_obj_set_style_pad_all(dd, 10, 0);
    lv_obj_set_style_text_font(dd, &lv_font_montserrat_20, 0);
    return dd;
}

static void next_cb(lv_event_t *e)
{
    (void)e;
    const char *tz_id = timezone_catalog_timezone_id(s_country_idx, s_location_idx);
    if (tz_id[0] == '\0') {
        return;
    }

    app_config_t *cfg = app_config_get();
    snprintf(cfg->timezone_id, sizeof(cfg->timezone_id), "%s", tz_id);
    cfg->timezone_set = true;
    app_time_apply_timezone_id(tz_id);
    app_config_save_timezone();
    ui_nav_go(UI_SCREEN_TOD_BRIGHT);
}

static lv_obj_t *tz_create_screen(void)
{
    const ui_theme_t *t = ui_theme_get();
    lv_obj_t *scr = ui_widgets_create_screen();
    lv_obj_set_style_border_width(scr, TZ_RING_BORDER, 0);
    lv_obj_set_style_border_color(scr, t->ring, 0);
    return scr;
}

void ui_screen_startup_timezone_wizard_build(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    const ui_theme_t *th = ui_theme_get();

    s_scr = tz_create_screen();
    screens[UI_SCREEN_STARTUP_TIMEZONE] = s_scr;

    lv_obj_t *title = ui_widgets_create_title(s_scr, "Set Time Zone");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, TZ_TITLE_Y_OFFSET);

    s_dd_country = create_dropdown(s_scr, TZ_COUNTRY_Y_WF);
    s_dd_location = create_dropdown(s_scr, TZ_LOCATION_Y_WF);

    if (timezone_catalog_format_country_options(s_country_opts, sizeof(s_country_opts)) == 0) {
        s_country_opts[0] = '\0';
    }
    lv_dropdown_set_options(s_dd_country, s_country_opts);
    lv_obj_add_event_cb(s_dd_country, country_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(s_dd_location, location_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);

    s_lbl_preview = lv_label_create(s_scr);
    lv_label_set_text(s_lbl_preview, "--:--");
    lv_obj_set_style_text_color(s_lbl_preview, th->white, 0);
    lv_obj_set_style_text_font(s_lbl_preview, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_align(s_lbl_preview, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_lbl_preview, UI_CONTENT_W(TZ_RING_BORDER));
    lv_obj_align(s_lbl_preview, LV_ALIGN_CENTER, 0, 24);

    select_default_dropdowns();

    lv_obj_t *next = ui_wedge_create(
        s_scr, UI_WEDGE_CONFIRM,
        UI_WF_X(UI_WEDGE_CONFIRM_X_WF, TZ_RING_BORDER),
        UI_WF_Y(UI_WEDGE_CONFIRM_Y_WF, TZ_RING_BORDER));
    lv_obj_add_event_cb(next, next_cb, LV_EVENT_CLICKED, NULL);
}

void ui_screen_startup_timezone_wizard_on_show(void)
{
    select_default_dropdowns();
    apply_selection_preview();

    if (s_preview_timer == NULL) {
        s_preview_timer = lv_timer_create(preview_timer_cb, 1000, NULL);
    } else {
        lv_timer_reset(s_preview_timer);
    }
}

void ui_screen_startup_timezone_wizard_on_hide(void)
{
    if (s_preview_timer != NULL) {
        lv_timer_delete(s_preview_timer);
        s_preview_timer = NULL;
    }
}

void ui_screen_startup_timezone_wizard_apply_theme(void)
{
    const ui_theme_t *t = ui_theme_get();
    if (s_scr != NULL) {
        lv_obj_set_style_border_color(s_scr, t->ring, 0);
    }
    if (s_dd_country != NULL) {
        lv_obj_set_style_bg_color(s_dd_country, t->ring, 0);
    }
    if (s_dd_location != NULL) {
        lv_obj_set_style_bg_color(s_dd_location, t->ring, 0);
    }
}
