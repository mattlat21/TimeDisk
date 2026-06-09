/**
 * @file ui_time_editor.c
 */

#include "ui_time_editor.h"
#include "ui_duration_editor.h"
#include "ui_layout.h"
#include "ui_theme.h"
#include "ui_widgets.h"

#include <stdio.h>
#include <time.h>

static ui_time_editor_bundle_t *bundle_from_event(lv_event_t *e)
{
    return (ui_time_editor_bundle_t *)lv_event_get_user_data(e);
}

static void notify_change(ui_time_editor_bundle_t *bundle)
{
    if (bundle->cfg.on_change != NULL) {
        bundle->cfg.on_change(bundle->cfg.user_data);
    }
}

static void clamp_duration(ui_time_editor_cfg_t *cfg)
{
    if (cfg == NULL || cfg->value_sec == NULL) {
        return;
    }
    const uint32_t max_sec = cfg->max_sec > 0 ? cfg->max_sec : UI_SCHEDULE_REST_MAX_SEC;
    if (*cfg->value_sec > max_sec) {
        *cfg->value_sec = max_sec;
    }
    if (cfg->min_sec > 0 && *cfg->value_sec < cfg->min_sec) {
        *cfg->value_sec = cfg->min_sec;
    }
}

static time_t end_time_from_cfg(const ui_time_editor_cfg_t *cfg)
{
    return time(NULL) + (time_t)cfg->end_time_offset_sec + (time_t)*cfg->value_sec;
}

static void set_value_from_end_time(ui_time_editor_cfg_t *cfg, time_t end)
{
    time_t now = time(NULL);
    int64_t delta = (int64_t)end - (int64_t)now - (int64_t)cfg->end_time_offset_sec;
    if (delta < 0) {
        delta = 0;
    }
    *cfg->value_sec = (uint32_t)delta;
    clamp_duration(cfg);
}

static void end_time_parts(const ui_time_editor_cfg_t *cfg, int *h12_out, int *min_out, const char **ampm_out)
{
    time_t end = end_time_from_cfg(cfg);
    struct tm tm_info;
    localtime_r(&end, &tm_info);
    int h24 = tm_info.tm_hour;
    int h12 = h24 % 12;
    if (h12 == 0) {
        h12 = 12;
    }
    *h12_out = h12;
    *min_out = tm_info.tm_min;
    *ampm_out = (h24 >= 12) ? "PM" : "AM";
}

static void adjust_end_time(ui_time_editor_bundle_t *bundle, int delta_minutes)
{
    ui_time_editor_cfg_t *cfg = &bundle->cfg;
    time_t end = end_time_from_cfg(cfg);
    end += (time_t)delta_minutes * 60;
    set_value_from_end_time(cfg, end);
    ui_time_editor_refresh(&bundle->editor, cfg);
    notify_change(bundle);
}

static void h_minus_cb(lv_event_t *e)
{
    adjust_end_time(bundle_from_event(e), -60);
}

static void h_plus_cb(lv_event_t *e)
{
    adjust_end_time(bundle_from_event(e), 60);
}

static void m_minus_cb(lv_event_t *e)
{
    adjust_end_time(bundle_from_event(e), -1);
}

static void m_plus_cb(lv_event_t *e)
{
    adjust_end_time(bundle_from_event(e), 1);
}

static lv_obj_t *make_stepper_btn(lv_obj_t *parent, const char *txt, int x, int y,
                                  lv_event_cb_t cb, ui_time_editor_bundle_t *bundle)
{
    const ui_theme_t *t = ui_theme_get();
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, UI_TIME_EDITOR_STEPPER, UI_TIME_EDITOR_STEPPER);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(btn, t->keypad, 0);
    lv_obj_t *l = lv_label_create(btn);
    lv_label_set_text(l, txt);
    lv_obj_set_style_text_color(l, t->white, 0);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_20, 0);
    lv_obj_center(l);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, bundle);
    return btn;
}

static void set_obj_visible(lv_obj_t *obj, bool visible)
{
    if (obj == NULL) {
        return;
    }
    if (visible) {
        lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    }
}

static void create_time_column(lv_obj_t *box, int col_x, lv_obj_t **btn_plus, lv_obj_t **lbl,
                               lv_obj_t **btn_minus, lv_event_cb_t plus_cb, lv_event_cb_t minus_cb,
                               ui_time_editor_bundle_t *bundle, const ui_theme_t *t)
{
    const int step = UI_TIME_EDITOR_STEPPER;
    const int btn_x = col_x + (UI_TIME_EDITOR_COL_W - step) / 2;
    const int lbl_y = 4 + step + 6;

    *btn_plus = make_stepper_btn(box, "+", btn_x, 4, plus_cb, bundle);
    *lbl = lv_label_create(box);
    lv_label_set_text(*lbl, "0");
    lv_obj_set_style_text_color(*lbl, t->white, 0);
    lv_obj_set_style_text_font(*lbl, &lv_font_montserrat_26, 0);
    lv_obj_set_pos(*lbl, col_x + (UI_TIME_EDITOR_COL_W - 32) / 2, lbl_y);
    lv_obj_set_width(*lbl, 32);
    lv_obj_set_style_text_align(*lbl, LV_TEXT_ALIGN_CENTER, 0);
    *btn_minus = make_stepper_btn(box, "-", btn_x, lbl_y + 30, minus_cb, bundle);
}

void ui_time_editor_set_visible(const ui_time_editor_t *ed, bool visible)
{
    if (ed == NULL) {
        return;
    }
    set_obj_visible(ed->box, visible);
}

void ui_time_editor_refresh(const ui_time_editor_t *ed, const ui_time_editor_cfg_t *cfg)
{
    if (ed == NULL || cfg == NULL || cfg->value_sec == NULL) {
        return;
    }

    clamp_duration((ui_time_editor_cfg_t *)cfg);

    int h12;
    int min;
    const char *ampm;
    end_time_parts(cfg, &h12, &min, &ampm);

    char hour_buf[8];
    char min_buf[8];
    snprintf(hour_buf, sizeof(hour_buf), "%d", h12);
    snprintf(min_buf, sizeof(min_buf), "%02d", min);

    if (ed->lbl_hour != NULL) {
        lv_label_set_text(ed->lbl_hour, hour_buf);
    }
    if (ed->lbl_min != NULL) {
        lv_label_set_text(ed->lbl_min, min_buf);
    }
    if (ed->lbl_ampm != NULL) {
        lv_label_set_text(ed->lbl_ampm, ampm);
    }
}

void ui_time_editor_create(lv_obj_t *parent, ui_time_editor_bundle_t *bundle)
{
    const ui_theme_t *t = ui_theme_get();
    ui_time_editor_cfg_t *cfg = &bundle->cfg;
    ui_time_editor_t *out = &bundle->editor;

    out->box = NULL;
    out->lbl_hour = NULL;
    out->lbl_min = NULL;
    out->lbl_ampm = NULL;
    out->btn_h_minus = NULL;
    out->btn_h_plus = NULL;
    out->btn_m_minus = NULL;
    out->btn_m_plus = NULL;

    if (cfg->value_sec == NULL) {
        return;
    }
    if (cfg->box_w <= 0) {
        cfg->box_w = UI_TIME_EDITOR_BOX_W;
    }
    if (cfg->box_h <= 0) {
        cfg->box_h = UI_TIME_EDITOR_BOX_H;
    }
    if (cfg->box_y < 0) {
        cfg->box_y = UI_TIME_EDITOR_BOX_Y;
    }

    int box_x = 0;
    int box_y = 0;
    {
        const int box_x_wf = (int)UI_SCREEN_CX - cfg->box_w / 2;
        ui_layout_parent_pos_from_wf(parent, box_x_wf, cfg->box_y, &box_x, &box_y);
    }

    clamp_duration(cfg);

    out->box = ui_widgets_create_purple_box(parent, cfg->box_w, cfg->box_h, box_x, box_y, false);

    const int hour_col_x = 72;
    const int min_col_x = hour_col_x + UI_TIME_EDITOR_COL_W + 36;

    create_time_column(out->box, hour_col_x, &out->btn_h_plus, &out->lbl_hour, &out->btn_h_minus,
                       h_plus_cb, h_minus_cb, bundle, t);
    create_time_column(out->box, min_col_x, &out->btn_m_plus, &out->lbl_min, &out->btn_m_minus,
                       m_plus_cb, m_minus_cb, bundle, t);

    lv_obj_t *colon = lv_label_create(out->box);
    lv_label_set_text(colon, ":");
    lv_obj_set_style_text_color(colon, t->white, 0);
    lv_obj_set_style_text_font(colon, &lv_font_montserrat_26, 0);
    lv_obj_set_pos(colon, hour_col_x + UI_TIME_EDITOR_COL_W + 10, 4 + UI_TIME_EDITOR_STEPPER + 10);

    out->lbl_ampm = lv_label_create(out->box);
    lv_label_set_text(out->lbl_ampm, "AM");
    lv_obj_set_style_text_color(out->lbl_ampm, t->secondary, 0);
    lv_obj_set_style_text_font(out->lbl_ampm, &lv_font_montserrat_20, 0);
    lv_obj_align(out->lbl_ampm, LV_ALIGN_RIGHT_MID, -20, 8);

    ui_time_editor_refresh(out, cfg);
}
