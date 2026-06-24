/**
 * @file ui_end_time_picker.c
 */

#include "ui_end_time_picker.h"
#include "ui_duration_editor.h"
#include "ui_layout.h"
#include "ui_theme.h"
#include "ui_widgets.h"

#include <stdio.h>
#include <time.h>

static ui_end_time_picker_bundle_t *bundle_from_event(lv_event_t *e)
{
    return (ui_end_time_picker_bundle_t *)lv_event_get_user_data(e);
}

static void notify_change(ui_end_time_picker_bundle_t *bundle)
{
    if (bundle->cfg.on_change != NULL) {
        bundle->cfg.on_change(bundle->cfg.user_data);
    }
}

static void clamp_duration(ui_end_time_picker_cfg_t *cfg)
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

static time_t end_time_from_cfg(const ui_end_time_picker_cfg_t *cfg)
{
    return time(NULL) + (time_t)cfg->end_time_offset_sec + (time_t)*cfg->value_sec;
}

static void set_value_from_end_time(ui_end_time_picker_cfg_t *cfg, time_t end)
{
    time_t now = time(NULL);
    int64_t delta = (int64_t)end - (int64_t)now - (int64_t)cfg->end_time_offset_sec;
    if (delta < 0) {
        delta = 0;
    }
    *cfg->value_sec = (uint32_t)delta;
    clamp_duration(cfg);
}

static void end_time_parts(const ui_end_time_picker_cfg_t *cfg, int *h12_out, int *min_out, const char **ampm_out)
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
    *ampm_out = (h24 >= 12) ? "pm" : "am";
}

static void adjust_end_time(ui_end_time_picker_bundle_t *bundle, int delta_minutes)
{
    ui_end_time_picker_cfg_t *cfg = &bundle->cfg;
    time_t end = end_time_from_cfg(cfg);
    end += (time_t)delta_minutes * 60;
    set_value_from_end_time(cfg, end);
    ui_end_time_picker_refresh(&bundle->picker, cfg);
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

static lv_obj_t *make_stepper_btn(lv_obj_t *parent, const char *txt, int x, int y, int w, int h,
                                  lv_event_cb_t cb, ui_end_time_picker_bundle_t *bundle)
{
    const ui_theme_t *t = ui_theme_get();
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_radius(btn, 20, 0);
    lv_obj_set_style_bg_color(btn, t->keypad, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_t *l = lv_label_create(btn);
    lv_label_set_text(l, txt);
    lv_obj_set_style_text_color(l, t->white, 0);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_48, 0);
    lv_obj_center(l);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, bundle);
    return btn;
}

static void create_time_column(lv_obj_t *parent, int col_x, lv_obj_t **btn_plus, lv_obj_t **lbl,
                               lv_obj_t **btn_minus, lv_event_cb_t plus_cb, lv_event_cb_t minus_cb,
                               ui_end_time_picker_bundle_t *bundle, const ui_theme_t *t)
{
    const int col_w = UI_END_TIME_PICKER_COL_W;
    const int btn_h = UI_END_TIME_PICKER_STEP_H;
    const int val_h = UI_END_TIME_PICKER_VAL_H;
    int y = 0;

    *btn_plus = make_stepper_btn(parent, "+", col_x, y, col_w, btn_h, plus_cb, bundle);
    y += btn_h;

    lv_obj_t *val_panel = ui_widgets_create_purple_box(parent, col_w, val_h, col_x, y, false);
    lv_obj_set_style_radius(val_panel, 20, 0);
    *lbl = lv_label_create(val_panel);
    lv_label_set_text(*lbl, "0");
    lv_obj_set_style_text_color(*lbl, t->white, 0);
    lv_obj_set_style_text_font(*lbl, &lv_font_montserrat_48, 0);
    lv_obj_center(*lbl);

    y += val_h;
    *btn_minus = make_stepper_btn(parent, "-", col_x, y, col_w, btn_h, minus_cb, bundle);
}

void ui_end_time_picker_set_visible(const ui_end_time_picker_t *picker, bool visible)
{
    if (picker == NULL) {
        return;
    }
    set_obj_visible(picker->box, visible);
}

void ui_end_time_picker_refresh(const ui_end_time_picker_t *picker, const ui_end_time_picker_cfg_t *cfg)
{
    if (picker == NULL || cfg == NULL || cfg->value_sec == NULL) {
        return;
    }

    clamp_duration((ui_end_time_picker_cfg_t *)cfg);

    int h12;
    int min;
    const char *ampm;
    end_time_parts(cfg, &h12, &min, &ampm);

    char hour_buf[8];
    char min_buf[8];
    snprintf(hour_buf, sizeof(hour_buf), "%d", h12);
    snprintf(min_buf, sizeof(min_buf), "%02d", min);

    if (picker->lbl_hour != NULL) {
        lv_label_set_text(picker->lbl_hour, hour_buf);
    }
    if (picker->lbl_min != NULL) {
        lv_label_set_text(picker->lbl_min, min_buf);
    }
    if (picker->lbl_ampm != NULL) {
        lv_label_set_text(picker->lbl_ampm, ampm);
    }
}

void ui_end_time_picker_create(lv_obj_t *parent, ui_end_time_picker_bundle_t *bundle)
{
    const ui_theme_t *t = ui_theme_get();
    ui_end_time_picker_cfg_t *cfg = &bundle->cfg;
    ui_end_time_picker_t *out = &bundle->picker;

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
    if (cfg->box_y < 0) {
        cfg->box_y = UI_END_TIME_PICKER_BOX_Y;
    }

    clamp_duration(cfg);

    const int col_w = UI_END_TIME_PICKER_COL_W;
    const int btn_h = UI_END_TIME_PICKER_STEP_H;
    const int val_h = UI_END_TIME_PICKER_VAL_H;
    const int col_h = btn_h + val_h + btn_h;
    const int colon_w = UI_END_TIME_PICKER_COLON_W;
    const int ampm_w = UI_END_TIME_PICKER_AMPM_W;
    const int content_w = col_w + colon_w + col_w + ampm_w;
    const int box_x_wf = (int)UI_SCREEN_CX - content_w / 2;
    int box_x = 0;
    int box_y = 0;
    ui_layout_parent_pos_from_wf(parent, box_x_wf, cfg->box_y, &box_x, &box_y);

    out->box = lv_obj_create(parent);
    lv_obj_set_size(out->box, content_w, col_h);
    lv_obj_set_pos(out->box, box_x, box_y);
    lv_obj_set_style_bg_opa(out->box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(out->box, 0, 0);
    lv_obj_set_style_pad_all(out->box, 0, 0);
    lv_obj_remove_flag(out->box, LV_OBJ_FLAG_SCROLLABLE);

    const int hour_col_x = 0;
    const int min_col_x = col_w + colon_w;

    create_time_column(out->box, hour_col_x, &out->btn_h_plus, &out->lbl_hour, &out->btn_h_minus,
                       h_plus_cb, h_minus_cb, bundle, t);
    create_time_column(out->box, min_col_x, &out->btn_m_plus, &out->lbl_min, &out->btn_m_minus,
                       m_plus_cb, m_minus_cb, bundle, t);

    lv_obj_t *colon = lv_label_create(out->box);
    lv_label_set_text(colon, ":");
    lv_obj_set_style_text_color(colon, t->white, 0);
    lv_obj_set_style_text_font(colon, &lv_font_montserrat_48, 0);
    lv_obj_set_pos(colon, col_w + 8, btn_h + (val_h - 48) / 2);

    out->lbl_ampm = lv_label_create(out->box);
    lv_label_set_text(out->lbl_ampm, "am");
    lv_obj_set_style_text_color(out->lbl_ampm, t->white, 0);
    lv_obj_set_style_text_font(out->lbl_ampm, &lv_font_montserrat_34, 0);
    lv_obj_set_pos(out->lbl_ampm, min_col_x + col_w + 12, btn_h + (val_h - 34) / 2);

    ui_end_time_picker_refresh(out, cfg);
}
