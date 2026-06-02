/**
 * @file ui_settings_schedule.c
 * @brief Settings -> Schedule sub-panel.
 */

#include "ui_screen_settings_internal.h"

#include "ui_duration_editor.h"
#include "ui_theme.h"
#include "ui_widgets.h"

#include <string.h>

#define SCHED_ROW_Y0_WF      120
#define SCHED_ROW_STEP_WF    140

static lv_obj_t *s_panel;
static ui_duration_editor_bundle_t s_sched_bundles[3];
static uint32_t s_sched_vals[3];
static const char *s_sched_labels[3] = {"Wind down", "Sleep", "Rest"};

void ui_settings_schedule_sync_from_draft(void)
{
    app_config_t *draft = ui_settings_draft();
    s_sched_vals[0] = draft->wind_down_sec;
    s_sched_vals[1] = draft->sleep_sec;
    s_sched_vals[2] = draft->rest_sec;
    for (int i = 0; i < 3; i++) {
        ui_duration_editor_refresh(&s_sched_bundles[i].editor, &s_sched_bundles[i].cfg);
    }
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

lv_obj_t *ui_settings_schedule_build(void)
{
    const ui_theme_t *t = ui_theme_get();
    static const int box_h = 72;

    s_panel = ui_settings_create_sub_panel("Schedule");

    for (int i = 0; i < 3; i++) {
        const int row_y_wf = SCHED_ROW_Y0_WF + i * SCHED_ROW_STEP_WF;
        lv_obj_t *lbl = lv_label_create(s_panel);
        lv_label_set_text(lbl, s_sched_labels[i]);
        lv_obj_set_style_text_color(lbl, t->white, 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0);
        lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, ui_settings_wf_y(s_panel, row_y_wf - 24));

        s_sched_bundles[i].cfg = (ui_duration_editor_cfg_t){
            .value_sec = &s_sched_vals[i],
            .box_y = row_y_wf,
            .box_w = 400,
            .box_h = box_h,
            .show_end_time = false,
            .on_change = ui_settings_idle_cb,
        };
        ui_duration_editor_create(s_panel, &s_sched_bundles[i]);
    }

    ui_settings_attach_panel_wedges(s_panel, PANEL_SCHEDULE, schedule_save_cb);
    return s_panel;
}

