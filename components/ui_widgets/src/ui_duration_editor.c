/**
 * @file ui_duration_editor.c
 */

#include "ui_duration_editor.h"
#include "ui_layout.h"
#include "ui_format.h"
#include "ui_theme.h"
#include "ui_widgets.h"

static ui_duration_editor_bundle_t *bundle_from_event(lv_event_t *e)
{
    return (ui_duration_editor_bundle_t *)lv_event_get_user_data(e);
}

static void notify_change(ui_duration_editor_bundle_t *bundle)
{
    if (bundle->cfg.on_change != NULL) {
        bundle->cfg.on_change(bundle->cfg.user_data);
    }
}

static void minus_cb(lv_event_t *e)
{
    ui_duration_editor_bundle_t *bundle = bundle_from_event(e);
    uint32_t *sec = bundle->cfg.value_sec;
    uint32_t step = bundle->cfg.step_sec > 0 ? bundle->cfg.step_sec : UI_DURATION_EDITOR_STEP_SEC;

    if (*sec > step) {
        *sec -= step;
    } else {
        *sec = 0;
    }
    ui_duration_editor_refresh(&bundle->editor, &bundle->cfg);
    notify_change(bundle);
}

static void plus_cb(lv_event_t *e)
{
    ui_duration_editor_bundle_t *bundle = bundle_from_event(e);
    uint32_t *sec = bundle->cfg.value_sec;
    uint32_t step = bundle->cfg.step_sec > 0 ? bundle->cfg.step_sec : UI_DURATION_EDITOR_STEP_SEC;
    uint32_t max_sec = bundle->cfg.max_sec > 0 ? bundle->cfg.max_sec : UI_DURATION_EDITOR_MAX_SEC;

    *sec += step;
    if (*sec > max_sec) {
        *sec = max_sec;
    }
    ui_duration_editor_refresh(&bundle->editor, &bundle->cfg);
    notify_change(bundle);
}

static lv_obj_t *make_stepper_btn(lv_obj_t *parent, const char *txt, int x, int y,
                                  lv_event_cb_t cb, ui_duration_editor_bundle_t *bundle)
{
    const ui_theme_t *t = ui_theme_get();
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, UI_DURATION_EDITOR_STEPPER, UI_DURATION_EDITOR_STEPPER);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(btn, t->keypad, 0);
    lv_obj_t *l = lv_label_create(btn);
    lv_label_set_text(l, txt);
    lv_obj_set_style_text_color(l, t->white, 0);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_26, 0);
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
    char dur[32];

    ui_format_duration_minutes(dur, sizeof(dur), *cfg->value_sec);
    if (ed->lbl_value != NULL) {
        lv_label_set_text(ed->lbl_value, dur);
    }
    if (cfg->show_end_time && ed->lbl_subtitle != NULL) {
        char end[32];
        ui_format_hh_mm_ampm_after_sec(end, sizeof(end), *cfg->value_sec);
        lv_label_set_text(ed->lbl_subtitle, end);
        lv_obj_remove_flag(ed->lbl_subtitle, LV_OBJ_FLAG_HIDDEN);
    } else if (ed->lbl_subtitle != NULL) {
        lv_obj_add_flag(ed->lbl_subtitle, LV_OBJ_FLAG_HIDDEN);
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
    out->lbl_subtitle = NULL;

    if (cfg->value_sec == NULL) {
        return;
    }
    if (cfg->box_w <= 0) {
        cfg->box_w = UI_DURATION_EDITOR_BOX_W;
    }
    if (cfg->box_h <= 0) {
        cfg->box_h = UI_DURATION_EDITOR_BOX_H;
    }
    if (cfg->box_y < 0) {
        cfg->box_y = UI_DURATION_EDITOR_BOX_Y;
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
    if (cfg->step_sec == 0) {
        cfg->step_sec = UI_DURATION_EDITOR_STEP_SEC;
    }

    out->box = ui_widgets_create_purple_box(parent, cfg->box_w, cfg->box_h, cfg->box_x, cfg->box_y, false);
    out->lbl_value = lv_label_create(out->box);
    lv_obj_set_style_text_color(out->lbl_value, t->white, 0);
    lv_obj_set_style_text_font(out->lbl_value, &lv_font_montserrat_26, 0);
    lv_obj_center(out->lbl_value);

    if (cfg->show_end_time) {
        out->lbl_subtitle = lv_label_create(parent);
        lv_obj_set_style_text_color(out->lbl_subtitle, t->secondary, 0);
        lv_obj_set_style_text_font(out->lbl_subtitle, &lv_font_montserrat_20, 0);
        lv_obj_set_pos(out->lbl_subtitle, cfg->box_x, cfg->box_y + cfg->box_h + 12);
        lv_obj_set_width(out->lbl_subtitle, cfg->box_w);
        lv_obj_set_style_text_align(out->lbl_subtitle, LV_TEXT_ALIGN_CENTER, 0);
    }

    const int stepper_y = cfg->box_y + (cfg->box_h - UI_DURATION_EDITOR_STEPPER) / 2;
    const int minus_x = cfg->box_x - UI_DURATION_EDITOR_GAP - UI_DURATION_EDITOR_STEPPER;
    const int plus_x = cfg->box_x + cfg->box_w + UI_DURATION_EDITOR_GAP;

    out->btn_minus = make_stepper_btn(parent, "-", minus_x, stepper_y, minus_cb, bundle);
    out->btn_plus = make_stepper_btn(parent, "+", plus_x, stepper_y, plus_cb, bundle);

    ui_duration_editor_refresh(out, cfg);
}
