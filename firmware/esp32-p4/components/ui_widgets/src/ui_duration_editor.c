/**
 * @file ui_duration_editor.c
 */

#include "ui_duration_editor.h"
#include "ui_fonts.h"
#include "ui_layout.h"
#include "ui_format.h"
#include "ui_theme.h"
#include "ui_widgets.h"

#include <stdio.h>

static ui_duration_editor_bundle_t *bundle_from_event(lv_event_t *e)
{
    return (ui_duration_editor_bundle_t *)lv_event_get_user_data(e);
}

static bool s_slider_sync;

static bool is_wind_down_style(const ui_duration_editor_cfg_t *cfg)
{
    return cfg != NULL && cfg->style == UI_DURATION_EDITOR_STYLE_WIND_DOWN;
}

static int editor_stepper_size(const ui_duration_editor_cfg_t *cfg)
{
    return is_wind_down_style(cfg) ? UI_DURATION_EDITOR_WD_STEPPER : UI_DURATION_EDITOR_STEPPER;
}

static int editor_stepper_gap(const ui_duration_editor_cfg_t *cfg)
{
    return is_wind_down_style(cfg) ? UI_DURATION_EDITOR_WD_GAP : UI_DURATION_EDITOR_GAP;
}

static int editor_slider_gap(const ui_duration_editor_cfg_t *cfg)
{
    return is_wind_down_style(cfg) ? UI_DURATION_EDITOR_WD_SLIDER_GAP : UI_DURATION_EDITOR_SLIDER_GAP;
}

static int editor_slider_w(const ui_duration_editor_cfg_t *cfg)
{
    return is_wind_down_style(cfg) ? UI_DURATION_EDITOR_WD_SLIDER_W : UI_DURATION_EDITOR_SLIDER_W;
}

static void notify_change(ui_duration_editor_bundle_t *bundle)
{
    if (bundle->cfg.on_change != NULL) {
        bundle->cfg.on_change(bundle->cfg.user_data);
    }
}

static uint32_t step_for_value(const ui_duration_editor_cfg_t *cfg)
{
    if (cfg == NULL || cfg->value_sec == NULL) {
        return UI_DURATION_EDITOR_STEP_SEC;
    }
    if (cfg->get_step_sec != NULL) {
        return cfg->get_step_sec(*cfg->value_sec, cfg->user_data);
    }
    return cfg->step_sec > 0 ? cfg->step_sec : UI_DURATION_EDITOR_STEP_SEC;
}

static void clamp_duration(ui_duration_editor_cfg_t *cfg)
{
    if (cfg == NULL || cfg->value_sec == NULL) {
        return;
    }
    const uint32_t max_sec = cfg->max_sec > 0 ? cfg->max_sec : UI_DURATION_EDITOR_MAX_SEC;
    if (*cfg->value_sec > max_sec) {
        *cfg->value_sec = max_sec;
    }
    if (cfg->min_sec > 0 && *cfg->value_sec < cfg->min_sec) {
        *cfg->value_sec = cfg->min_sec;
    }
}

static int slider_max_pos(const ui_duration_editor_cfg_t *cfg)
{
    const uint32_t step = step_for_value(cfg);
    const uint32_t max_sec = cfg->max_sec > 0 ? cfg->max_sec : UI_DURATION_EDITOR_MAX_SEC;
    const uint32_t min_sec = cfg->min_sec;

    if (step == 0 || max_sec <= min_sec) {
        return 0;
    }
    return (int)((max_sec - min_sec) / step);
}

static int value_to_slider_pos(const ui_duration_editor_cfg_t *cfg, uint32_t val)
{
    const uint32_t step = step_for_value(cfg);
    const uint32_t min_sec = cfg->min_sec;

    if (step == 0 || val <= min_sec) {
        return 0;
    }
    return (int)((val - min_sec) / step);
}

static uint32_t slider_pos_to_value(const ui_duration_editor_cfg_t *cfg, int pos)
{
    const uint32_t step = step_for_value(cfg);

    if (pos < 0) {
        pos = 0;
    }
    return cfg->min_sec + (uint32_t)pos * step;
}

static void sync_slider_from_value(const ui_duration_editor_t *ed, const ui_duration_editor_cfg_t *cfg)
{
    if (ed == NULL || ed->slider == NULL || cfg == NULL) {
        return;
    }

    const int max_pos = slider_max_pos(cfg);
    int pos = value_to_slider_pos(cfg, *cfg->value_sec);
    if (pos > max_pos) {
        pos = max_pos;
    }

    s_slider_sync = true;
    lv_slider_set_range(ed->slider, 0, max_pos);
    lv_slider_set_value(ed->slider, pos, LV_ANIM_OFF);
    s_slider_sync = false;
}

static void slider_cb(lv_event_t *e)
{
    if (s_slider_sync) {
        return;
    }

    ui_duration_editor_bundle_t *bundle = bundle_from_event(e);
    ui_duration_editor_cfg_t *cfg = &bundle->cfg;
    if (cfg->value_sec == NULL || bundle->editor.slider == NULL) {
        return;
    }

    const int pos = (int)lv_slider_get_value(bundle->editor.slider);
    *cfg->value_sec = slider_pos_to_value(cfg, pos);
    clamp_duration(cfg);
    ui_duration_editor_refresh(&bundle->editor, cfg);
    notify_change(bundle);
}

static void minus_cb(lv_event_t *e)
{
    ui_duration_editor_bundle_t *bundle = bundle_from_event(e);
    ui_duration_editor_cfg_t *cfg = &bundle->cfg;
    uint32_t *sec = cfg->value_sec;
    const uint32_t step = step_for_value(cfg);
    const uint32_t min_sec = cfg->min_sec;

    if (min_sec > 0) {
        if (*sec <= min_sec) {
            *sec = min_sec;
        } else if (*sec - step < min_sec) {
            *sec = min_sec;
        } else {
            *sec -= step;
        }
    } else if (*sec > step) {
        *sec -= step;
    } else {
        *sec = 0;
    }
    clamp_duration(cfg);
    ui_duration_editor_refresh(&bundle->editor, cfg);
    notify_change(bundle);
}

static void plus_cb(lv_event_t *e)
{
    ui_duration_editor_bundle_t *bundle = bundle_from_event(e);
    ui_duration_editor_cfg_t *cfg = &bundle->cfg;
    uint32_t *sec = cfg->value_sec;
    const uint32_t step = step_for_value(cfg);

    *sec += step;
    clamp_duration(cfg);
    ui_duration_editor_refresh(&bundle->editor, cfg);
    notify_change(bundle);
}

static lv_obj_t *make_stepper_btn(lv_obj_t *parent, const char *txt, int x, int y, int size,
                                  lv_event_cb_t cb, ui_duration_editor_bundle_t *bundle,
                                  const ui_duration_editor_cfg_t *cfg)
{
    const ui_theme_t *t = ui_theme_get();
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, size, size);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(btn, t->keypad, 0);
    lv_obj_t *l = lv_label_create(btn);
    lv_label_set_text(l, txt);
    lv_obj_set_style_text_color(l, t->white, 0);
    if (is_wind_down_style(cfg)) {
        lv_obj_set_style_text_font(l, &lv_font_montserrat_48, 0);
    } else {
        lv_obj_set_style_text_font(l, &lv_font_montserrat_26, 0);
    }
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

void ui_duration_editor_set_visible(const ui_duration_editor_t *ed, bool visible)
{
    if (ed == NULL) {
        return;
    }
    set_obj_visible(ed->box, visible);
    set_obj_visible(ed->btn_minus, visible);
    set_obj_visible(ed->btn_plus, visible);
    if (ed->lbl_unit != NULL) {
        set_obj_visible(ed->lbl_unit, visible);
    }
    if (ed->slider != NULL && !visible) {
        set_obj_visible(ed->slider, false);
    }
    if (ed->lbl_subtitle != NULL) {
        set_obj_visible(ed->lbl_subtitle, visible);
    }
}

void ui_duration_editor_apply_theme(const ui_duration_editor_t *ed)
{
    if (ed == NULL || ed->lbl_subtitle == NULL) {
        return;
    }
    const ui_theme_t *t = ui_theme_get();
    lv_obj_set_style_text_color(ed->lbl_subtitle, t->secondary, 0);
}

void ui_duration_editor_refresh(const ui_duration_editor_t *ed, const ui_duration_editor_cfg_t *cfg)
{
    if (ed == NULL || cfg == NULL || cfg->value_sec == NULL) {
        return;
    }

    if (cfg->min_sec > 0 && *cfg->value_sec < cfg->min_sec) {
        *cfg->value_sec = cfg->min_sec;
    }

    char dur[32];

    if (cfg->display == UI_DURATION_DISPLAY_WIND_DOWN) {
        const uint32_t sec = *cfg->value_sec;
        const char *unit = "min";

        if (sec < 60) {
            snprintf(dur, sizeof(dur), "%u", (unsigned)sec);
            unit = "sec";
        } else if (sec % 60 != 0) {
            snprintf(dur, sizeof(dur), "%u:%02u", (unsigned)(sec / 60), (unsigned)(sec % 60));
        } else {
            snprintf(dur, sizeof(dur), "%u", (unsigned)(sec / 60));
        }
        if (ed->lbl_value != NULL) {
            lv_label_set_text(ed->lbl_value, dur);
        }
        if (ed->lbl_unit != NULL) {
            lv_label_set_text(ed->lbl_unit, unit);
        }
    } else if (cfg->display == UI_DURATION_DISPLAY_HUMAN) {
        ui_format_duration_human(dur, sizeof(dur), *cfg->value_sec);
    } else if (cfg->display == UI_DURATION_DISPLAY_PERCENT) {
        snprintf(dur, sizeof(dur), "%lu%%", (unsigned long)*cfg->value_sec);
    } else {
        ui_format_duration_minutes(dur, sizeof(dur), *cfg->value_sec);
    }
    if (cfg->display != UI_DURATION_DISPLAY_WIND_DOWN && ed->lbl_value != NULL) {
        lv_label_set_text(ed->lbl_value, dur);
    }
    if (cfg->show_end_time && ed->lbl_subtitle != NULL) {
        char end[32];
        ui_format_hh_mm_ampm_after_sec(end, sizeof(end),
                                       cfg->end_time_offset_sec + *cfg->value_sec);
        lv_label_set_text(ed->lbl_subtitle, end);
        lv_obj_remove_flag(ed->lbl_subtitle, LV_OBJ_FLAG_HIDDEN);
    } else if (ed->lbl_subtitle != NULL) {
        lv_obj_add_flag(ed->lbl_subtitle, LV_OBJ_FLAG_HIDDEN);
    }

    if (cfg->show_slider && ed->slider != NULL) {
        set_obj_visible(ed->slider, true);
        sync_slider_from_value(ed, cfg);
    } else if (ed->slider != NULL) {
        set_obj_visible(ed->slider, false);
    }
}

void ui_duration_editor_create(lv_obj_t *parent, ui_duration_editor_bundle_t *bundle)
{
    const ui_theme_t *t = ui_theme_get();
    ui_duration_editor_cfg_t *cfg = &bundle->cfg;
    ui_duration_editor_t *out = &bundle->editor;

    out->box = NULL;
    out->btn_minus = NULL;
    out->btn_plus = NULL;
    out->lbl_value = NULL;
    out->lbl_unit = NULL;
    out->lbl_subtitle = NULL;
    out->slider = NULL;

    if (cfg->value_sec == NULL) {
        return;
    }
    if (is_wind_down_style(cfg)) {
        if (cfg->box_w <= 0) {
            cfg->box_w = UI_DURATION_EDITOR_WD_BOX_W;
        }
        if (cfg->box_h <= 0) {
            cfg->box_h = UI_DURATION_EDITOR_WD_BOX_H;
        }
        if (cfg->box_y < 0) {
            cfg->box_y = UI_DURATION_EDITOR_WD_BOX_Y_WF;
        }
    } else {
        if (cfg->box_w <= 0) {
            cfg->box_w = UI_DURATION_EDITOR_BOX_W;
        }
        if (cfg->box_h <= 0) {
            cfg->box_h = UI_DURATION_EDITOR_BOX_H;
        }
        if (cfg->box_y < 0) {
            cfg->box_y = UI_DURATION_EDITOR_BOX_Y;
        }
    }

    {
        int box_x_wf = (int)UI_SCREEN_CX - cfg->box_w / 2;
        int box_y_wf = cfg->box_y;
        int bx = 0;
        int by = 0;
        ui_layout_parent_pos_from_wf(parent, box_x_wf, box_y_wf, &bx, &by);
        cfg->box_x = bx;
        cfg->box_y = by;
    }
    if (cfg->max_sec == 0) {
        cfg->max_sec = UI_DURATION_EDITOR_MAX_SEC;
    }
    if (cfg->step_sec == 0 && cfg->get_step_sec == NULL) {
        cfg->step_sec = UI_DURATION_EDITOR_STEP_SEC;
    }
    clamp_duration(cfg);

    out->box = ui_widgets_create_purple_box(parent, cfg->box_w, cfg->box_h, cfg->box_x, cfg->box_y, false);
    out->lbl_value = lv_label_create(out->box);
    lv_obj_set_style_text_color(out->lbl_value, t->white, 0);
    if (is_wind_down_style(cfg)) {
        lv_obj_set_style_text_font(out->lbl_value, &lv_font_montserrat_80, 0);
        lv_obj_align(out->lbl_value, LV_ALIGN_TOP_MID, 0, UI_DURATION_EDITOR_WD_BOX_PAD);
        out->lbl_unit = lv_label_create(out->box);
        lv_label_set_text(out->lbl_unit, "min");
        lv_obj_set_style_text_color(out->lbl_unit, t->white, 0);
        lv_obj_set_style_text_font(out->lbl_unit, &lv_font_montserrat_48, 0);
        lv_obj_align(out->lbl_unit, LV_ALIGN_BOTTOM_MID, 0, -UI_DURATION_EDITOR_WD_BOX_PAD);
    } else {
        lv_obj_set_style_text_font(out->lbl_value, &lv_font_montserrat_26, 0);
        lv_obj_center(out->lbl_value);
    }

    if (cfg->show_end_time) {
        out->lbl_subtitle = lv_label_create(parent);
        lv_obj_set_style_text_color(out->lbl_subtitle, t->secondary, 0);
        lv_obj_set_style_text_font(out->lbl_subtitle, &lv_font_montserrat_20, 0);
        lv_obj_set_pos(out->lbl_subtitle, cfg->box_x, cfg->box_y + cfg->box_h + 12);
        lv_obj_set_width(out->lbl_subtitle, cfg->box_w);
        lv_obj_set_style_text_align(out->lbl_subtitle, LV_TEXT_ALIGN_CENTER, 0);
    }

    const int stepper = editor_stepper_size(cfg);
    const int gap = editor_stepper_gap(cfg);
    const int stepper_y = cfg->box_y + (cfg->box_h - stepper) / 2;
    const int minus_x = cfg->box_x - gap - stepper;
    const int plus_x = cfg->box_x + cfg->box_w + gap;

    out->btn_minus = make_stepper_btn(parent, "-", minus_x, stepper_y, stepper, minus_cb, bundle, cfg);
    out->btn_plus = make_stepper_btn(parent, "+", plus_x, stepper_y, stepper, plus_cb, bundle, cfg);

    if (cfg->show_slider) {
        const int slider_w = editor_slider_w(cfg);
        const int slider_x = cfg->box_x + (cfg->box_w - slider_w) / 2;
        const int slider_y = cfg->box_y + cfg->box_h + editor_slider_gap(cfg);

        out->slider = lv_slider_create(parent);
        if (is_wind_down_style(cfg)) {
            lv_obj_set_size(out->slider, slider_w, UI_DURATION_EDITOR_WD_SLIDER_TRACK);
            lv_obj_set_style_radius(out->slider, UI_DURATION_EDITOR_WD_SLIDER_TRACK / 2, LV_PART_MAIN);
            lv_obj_set_style_radius(out->slider, UI_DURATION_EDITOR_WD_SLIDER_TRACK / 2, LV_PART_INDICATOR);
            lv_obj_set_style_width(out->slider, UI_DURATION_EDITOR_WD_SLIDER_KNOB, LV_PART_KNOB);
            lv_obj_set_style_height(out->slider, UI_DURATION_EDITOR_WD_SLIDER_KNOB, LV_PART_KNOB);
        } else {
            lv_obj_set_size(out->slider, slider_w, UI_DURATION_EDITOR_SLIDER_H);
            lv_obj_set_style_radius(out->slider, UI_DURATION_EDITOR_SLIDER_H / 2, 0);
            lv_obj_set_style_radius(out->slider, UI_DURATION_EDITOR_SLIDER_H / 2, LV_PART_INDICATOR);
            lv_obj_set_style_radius(out->slider, LV_RADIUS_CIRCLE, LV_PART_KNOB);
        }
        lv_obj_set_pos(out->slider, slider_x, slider_y);
        lv_obj_set_style_bg_color(out->slider, t->white, LV_PART_MAIN);
        lv_obj_set_style_bg_color(out->slider, t->keypad, LV_PART_INDICATOR);
        lv_obj_set_style_bg_color(out->slider, t->keypad, LV_PART_KNOB);
        lv_obj_set_style_radius(out->slider, LV_RADIUS_CIRCLE, LV_PART_KNOB);
        lv_obj_add_event_cb(out->slider, slider_cb, LV_EVENT_VALUE_CHANGED, bundle);
        sync_slider_from_value(out, cfg);
    }

    ui_duration_editor_refresh(out, cfg);
}
