/**
 * @file ui_settings_timezone.c
 * @brief Settings -> Timezone sub-panel.
 */

#include "ui_screen_settings_internal.h"

#include "ui_format.h"
#include "ui_theme.h"
#include "ui_widgets.h"

#include "app_time.h"
#include "timezone_catalog.h"

#include <stdio.h>
#include <string.h>

/* --- Timezone --- */
#define TZ_DROPDOWN_W       480
#define TZ_DROPDOWN_H       52
#define TZ_COUNTRY_Y_WF     140
#define TZ_LOCATION_Y_WF    210
#define TZ_OPTIONS_BUF_LEN  512
#define TZ_PREVIEW_Y_WF      380

static lv_obj_t *s_panel;
static lv_obj_t *s_dd_country;
static lv_obj_t *s_dd_location;
static lv_obj_t *s_lbl_tz_preview;
static char s_country_opts[TZ_OPTIONS_BUF_LEN];
static char s_location_opts[TZ_OPTIONS_BUF_LEN];
static size_t s_country_idx;
static size_t s_location_idx;

static void tz_refresh_preview(void)
{
    if (s_lbl_tz_preview == NULL) {
        return;
    }
    app_runtime_t *rt = app_runtime_get();
    char tbuf[20];
    if (!rt->time_valid) {
        snprintf(tbuf, sizeof(tbuf), "--:--");
    } else {
        ui_format_hh_mm_ampm_now(tbuf, sizeof(tbuf));
    }
    lv_label_set_text(s_lbl_tz_preview, tbuf);
}

static void tz_apply_preview(void)
{
    const char *tz_id = timezone_catalog_timezone_id(s_country_idx, s_location_idx);
    if (tz_id[0] != '\0') {
        app_time_apply_timezone_id(tz_id);
    }
    tz_refresh_preview();
}

static void tz_sync_location_dropdown(size_t country_idx, size_t location_idx)
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

void ui_settings_timezone_select_from_draft(void)
{
    app_config_t *draft = ui_settings_draft();
    size_t ci = 0;
    size_t li = 0;
    if (draft->timezone_id[0] != '\0'
        && timezone_catalog_find_by_id(draft->timezone_id, &ci, &li)) {
        s_country_idx = ci;
        s_location_idx = li;
    } else if (!timezone_catalog_default_selection(&ci, &li)) {
        ci = 0;
        li = 0;
    } else {
        s_country_idx = ci;
        s_location_idx = li;
    }
    lv_dropdown_set_selected(s_dd_country, s_country_idx);
    tz_sync_location_dropdown(s_country_idx, s_location_idx);
    tz_apply_preview();
}

static void tz_country_changed_cb(lv_event_t *e)
{
    (void)e;
    s_country_idx = lv_dropdown_get_selected(s_dd_country);
    tz_sync_location_dropdown(s_country_idx, 0);
    tz_apply_preview();
    ui_settings_idle_cb(NULL);
}

static void tz_location_changed_cb(lv_event_t *e)
{
    (void)e;
    s_location_idx = lv_dropdown_get_selected(s_dd_location);
    tz_apply_preview();
    ui_settings_idle_cb(NULL);
}

static lv_obj_t *tz_create_dropdown(lv_obj_t *parent, int y_wf)
{
    const ui_theme_t *th = ui_theme_get();
    lv_obj_t *dd = lv_dropdown_create(parent);
    lv_obj_set_size(dd, TZ_DROPDOWN_W, TZ_DROPDOWN_H);
    lv_obj_align(dd, LV_ALIGN_TOP_MID, 0, ui_settings_wf_y(parent, y_wf));
    lv_obj_set_style_bg_color(dd, th->ring, 0);
    lv_obj_set_style_text_color(dd, th->white, 0);
    lv_obj_set_style_border_width(dd, 0, 0);
    lv_obj_set_style_pad_all(dd, 10, 0);
    lv_obj_set_style_text_font(dd, &lv_font_montserrat_20, 0);
    return dd;
}

static void timezone_save_cb(lv_event_t *e)
{
    (void)e;
    const char *tz_id = timezone_catalog_timezone_id(s_country_idx, s_location_idx);
    if (tz_id[0] == '\0') {
        return;
    }

    app_config_t *cfg = app_config_get();
    app_config_t *draft = ui_settings_draft();
    app_config_t *saved = ui_settings_saved();

    snprintf(draft->timezone_id, sizeof(draft->timezone_id), "%s", tz_id);
    draft->timezone_set = true;
    cfg->timezone_set = true;
    snprintf(cfg->timezone_id, sizeof(cfg->timezone_id), "%s", tz_id);
    memcpy(saved->timezone_id, draft->timezone_id, sizeof(saved->timezone_id));
    saved->timezone_set = true;

    app_time_apply_timezone_id(tz_id);
    app_config_save_timezone();
    ui_settings_show_panel(PANEL_HUB);
}

lv_obj_t *ui_settings_timezone_build(void)
{
    const ui_theme_t *th = ui_theme_get();

    s_panel = ui_settings_create_sub_panel("Timezone");

    s_dd_country = tz_create_dropdown(s_panel, TZ_COUNTRY_Y_WF);
    s_dd_location = tz_create_dropdown(s_panel, TZ_LOCATION_Y_WF);

    if (timezone_catalog_format_country_options(s_country_opts, sizeof(s_country_opts)) == 0) {
        s_country_opts[0] = '\0';
    }
    lv_dropdown_set_options(s_dd_country, s_country_opts);
    lv_obj_add_event_cb(s_dd_country, tz_country_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(s_dd_location, tz_location_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);

    s_lbl_tz_preview = lv_label_create(s_panel);
    lv_label_set_text(s_lbl_tz_preview, "--:--");
    lv_obj_set_style_text_color(s_lbl_tz_preview, th->white, 0);
    lv_obj_set_style_text_font(s_lbl_tz_preview, &lv_font_montserrat_48, 0);
    lv_obj_align(s_lbl_tz_preview, LV_ALIGN_TOP_MID, 0, ui_settings_wf_y(s_panel, TZ_PREVIEW_Y_WF));

    ui_settings_attach_panel_wedges(s_panel, PANEL_TIMEZONE, timezone_save_cb);
    return s_panel;
}

void ui_settings_timezone_apply_theme(void)
{
    const ui_theme_t *t = ui_theme_get();
    if (s_dd_country != NULL) {
        lv_obj_set_style_bg_color(s_dd_country, t->ring, 0);
    }
    if (s_dd_location != NULL) {
        lv_obj_set_style_bg_color(s_dd_location, t->ring, 0);
    }
}

