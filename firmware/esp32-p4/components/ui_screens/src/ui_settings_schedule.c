/**
 * @file ui_settings_schedule.c
 * @brief Settings -> Schedule sub-panel (cycle durations + scheduled events/buttons).
 */

#include "ui_screen_settings_internal.h"

#include "ui_duration_editor.h"
#include "ui_layout.h"
#include "ui_theme.h"
#include "ui_widgets.h"

#include <string.h>

#define SCHED_ROW_Y0_WF      120
#define SCHED_ROW_STEP_WF    130
#define SCHED_EVENTS_Y_WF    500
#define SCHED_BUTTONS_Y_WF   556
#define SCHED_BTN_W          248
#define SCHED_BTN_H          44
#define SCHED_BTN_RADIUS     16

static lv_obj_t *s_panel;
static lv_obj_t *s_panel_title;
static lv_obj_t *s_events_btn;
static lv_obj_t *s_buttons_btn;
static lv_obj_t *s_row_labels[3];
static ui_duration_editor_bundle_t s_sched_bundles[3];
static uint32_t s_sched_vals[3];
static const char *s_sched_labels[3] = {"Wind down", "Sleep", "Rest"};

void ui_settings_schedule_hide_durations(bool hide)
{
    for (int i = 0; i < 3; i++) {
        if (s_row_labels[i] != NULL) {
            if (hide) {
                lv_obj_add_flag(s_row_labels[i], LV_OBJ_FLAG_HIDDEN);
            } else {
                lv_obj_clear_flag(s_row_labels[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
        ui_duration_editor_set_visible(&s_sched_bundles[i].editor, !hide);
    }
    if (s_events_btn != NULL) {
        if (hide) {
            lv_obj_add_flag(s_events_btn, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(s_events_btn, LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (s_buttons_btn != NULL) {
        if (hide) {
            lv_obj_add_flag(s_buttons_btn, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_clear_flag(s_buttons_btn, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void ui_settings_schedule_sync_from_draft(void)
{
    app_config_t *draft = ui_settings_draft();
    s_sched_vals[0] = draft->wind_down_sec;
    s_sched_vals[1] = draft->sleep_sec;
    s_sched_vals[2] = draft->rest_sec;
    for (int i = 0; i < 3; i++) {
        ui_duration_editor_refresh(&s_sched_bundles[i].editor, &s_sched_bundles[i].cfg);
    }
    ui_settings_schedule_events_close();
    ui_settings_scheduled_buttons_close();
}

static void schedule_events_btn_cb(lv_event_t *e)
{
    (void)e;
    ui_settings_schedule_events_open();
    ui_settings_idle_cb(NULL);
}

static void schedule_buttons_btn_cb(lv_event_t *e)
{
    (void)e;
    ui_settings_scheduled_buttons_open();
    ui_settings_idle_cb(NULL);
}

static void schedule_save_cb(lv_event_t *e)
{
    (void)e;
    app_config_t *cfg = app_config_get();
    app_config_t *draft = ui_settings_draft();
    app_config_t *saved = ui_settings_saved();

    draft->wind_down_sec = s_sched_vals[0];
    draft->sleep_sec = s_sched_vals[1];
    draft->rest_sec = s_sched_vals[2];
    cfg->wind_down_sec = draft->wind_down_sec;
    cfg->sleep_sec = draft->sleep_sec;
    cfg->rest_sec = draft->rest_sec;
    saved->wind_down_sec = draft->wind_down_sec;
    saved->sleep_sec = draft->sleep_sec;
    saved->rest_sec = draft->rest_sec;

    app_config_save_schedule();
    ui_settings_show_panel(PANEL_HUB);
}

bool ui_settings_schedule_on_cancel(void)
{
    if (ui_settings_scheduled_buttons_try_cancel()) {
        return true;
    }
    if (ui_settings_schedule_events_try_cancel()) {
        return true;
    }
    return false;
}

lv_obj_t *ui_settings_schedule_build(void)
{
    const ui_theme_t *t = ui_theme_get();
    static const int box_h = 72;

    s_panel = lv_obj_create(ui_settings_screen());
    lv_obj_set_style_bg_opa(s_panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_panel, 0, 0);
    lv_obj_remove_flag(s_panel, LV_OBJ_FLAG_SCROLLABLE);
    ui_settings_panel_layout(s_panel);
    lv_obj_add_flag(s_panel, LV_OBJ_FLAG_HIDDEN);

    s_panel_title = ui_widgets_create_title(s_panel, "Schedule");

    for (int i = 0; i < 3; i++) {
        const int row_y_wf = SCHED_ROW_Y0_WF + i * SCHED_ROW_STEP_WF;
        lv_obj_t *lbl = lv_label_create(s_panel);
        lv_label_set_text(lbl, s_sched_labels[i]);
        lv_obj_set_style_text_color(lbl, t->white, 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0);
        lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, ui_settings_wf_y(s_panel, row_y_wf - 24));
        s_row_labels[i] = lbl;

        s_sched_bundles[i].cfg = (ui_duration_editor_cfg_t){
            .value_sec = &s_sched_vals[i],
            .box_y = row_y_wf,
            .box_w = 400,
            .box_h = box_h,
            .show_end_time = false,
            .max_sec = (i == 2) ? UI_SCHEDULE_REST_MAX_SEC : UI_DURATION_EDITOR_MAX_SEC,
            .on_change = ui_settings_idle_cb,
        };
        ui_duration_editor_create(s_panel, &s_sched_bundles[i]);
    }

    s_events_btn = lv_button_create(s_panel);
    lv_obj_set_size(s_events_btn, SCHED_BTN_W, SCHED_BTN_H);
    lv_obj_set_style_radius(s_events_btn, SCHED_BTN_RADIUS, 0);
    lv_obj_set_style_bg_color(s_events_btn, t->menu_petal, 0);
    lv_obj_align(s_events_btn, LV_ALIGN_TOP_MID, 0, ui_settings_wf_y(s_panel, SCHED_EVENTS_Y_WF));
    lv_obj_t *evt_lbl = lv_label_create(s_events_btn);
    lv_label_set_text(evt_lbl, "Scheduled events");
    lv_obj_set_style_text_color(evt_lbl, t->white, 0);
    lv_obj_set_style_text_font(evt_lbl, &lv_font_montserrat_18, 0);
    lv_obj_center(evt_lbl);
    lv_obj_add_event_cb(s_events_btn, schedule_events_btn_cb, LV_EVENT_CLICKED, NULL);

    s_buttons_btn = lv_button_create(s_panel);
    lv_obj_set_size(s_buttons_btn, SCHED_BTN_W, SCHED_BTN_H);
    lv_obj_set_style_radius(s_buttons_btn, SCHED_BTN_RADIUS, 0);
    lv_obj_set_style_bg_color(s_buttons_btn, t->menu_petal, 0);
    lv_obj_align(s_buttons_btn, LV_ALIGN_TOP_MID, 0, ui_settings_wf_y(s_panel, SCHED_BUTTONS_Y_WF));
    lv_obj_t *btn_lbl = lv_label_create(s_buttons_btn);
    lv_label_set_text(btn_lbl, "Scheduled buttons");
    lv_obj_set_style_text_color(btn_lbl, t->white, 0);
    lv_obj_set_style_text_font(btn_lbl, &lv_font_montserrat_18, 0);
    lv_obj_center(btn_lbl);
    lv_obj_add_event_cb(s_buttons_btn, schedule_buttons_btn_cb, LV_EVENT_CLICKED, NULL);

    ui_settings_schedule_events_init(s_panel, s_panel_title);
    ui_settings_scheduled_buttons_init(s_panel, s_panel_title);
    ui_settings_attach_panel_wedges(s_panel, PANEL_SCHEDULE, schedule_save_cb);
    return s_panel;
}
