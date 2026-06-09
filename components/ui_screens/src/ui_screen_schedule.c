#include "ui_screens_registry.h"
#include "ui_layout.h"
#include "ui_widgets.h"
#include "ui_duration_editor.h"
#include "ui_time_editor.h"
#include "ui_wedge.h"
#include "ui_theme.h"
#include "ui_nav.h"
#include "app_config.h"

#include <stdint.h>

#define SCHEDULE_EDITOR_BOX_W  400
#define SCHEDULE_EDITOR_BOX_H  80
#define SCHEDULE_EDITOR_BOX_Y  210
#define REST_TIME_BOX_Y        315

typedef struct {
    lv_obj_t *scr;
    ui_screen_id_t id;
    ui_duration_editor_bundle_t bundle;
    ui_time_editor_bundle_t time_bundle;
    bool has_time_editor;
} schedule_screen_t;

static schedule_screen_t s_screens[5];
static uint32_t s_wizard_vals[5];

static ui_screen_id_t s_sleep_ids[3] = {
    UI_SCREEN_SLEEP_WAKE,
    UI_SCREEN_SLEEP_REST_END,
    UI_SCREEN_SLEEP_WIND_DOWN,
};

static ui_screen_id_t s_rest_ids[2] = {
    UI_SCREEN_REST_REST_END,
    UI_SCREEN_REST_WIND_DOWN,
};

static const char *s_sleep_titles[3] = {
    "Sleep: Wake up Time",
    "Sleep: Rest End Time",
    "Sleep: Set Wind Down Time",
};

static const char *s_rest_titles[2] = {
    "Start a Rest",
    "Start a Rest",
};

static const char *s_rest_subtitles[2] = {
    "End Time",
    "Wind Down Time",
};

static int screen_index(ui_screen_id_t id)
{
    for (int i = 0; i < 3; i++) {
        if (s_sleep_ids[i] == id) {
            return i;
        }
    }
    for (int i = 0; i < 2; i++) {
        if (s_rest_ids[i] == id) {
            return i + 3;
        }
    }
    return 0;
}

static uint32_t wind_down_max_sec(uint32_t gross_next_sec)
{
    if (gross_next_sec == 0) {
        return 0;
    }
    return gross_next_sec - 1;
}

static void clamp_wizard_val(int idx)
{
    ui_duration_editor_cfg_t *cfg = &s_screens[idx].bundle.cfg;
    uint32_t *val = cfg->value_sec;

    if (val == NULL) {
        return;
    }
    if (cfg->min_sec > 0 && *val < cfg->min_sec) {
        *val = cfg->min_sec;
    }
    if (cfg->max_sec > 0 && *val > cfg->max_sec) {
        *val = cfg->max_sec;
    }
}

static void apply_editor_constraints(int idx)
{
    ui_duration_editor_cfg_t *dcfg = &s_screens[idx].bundle.cfg;
    ui_time_editor_cfg_t *tcfg = s_screens[idx].has_time_editor ? &s_screens[idx].time_bundle.cfg : NULL;

    dcfg->end_time_offset_sec = 0;
    dcfg->max_sec = UI_DURATION_EDITOR_MAX_SEC;
    dcfg->min_sec = 0;

    switch (idx) {
    case 0:
        dcfg->min_sec = UI_DURATION_EDITOR_STEP_SEC;
        break;
    case 1:
        dcfg->end_time_offset_sec = s_wizard_vals[0];
        dcfg->max_sec = UI_SCHEDULE_REST_MAX_SEC;
        break;
    case 2:
        dcfg->max_sec = wind_down_max_sec(s_wizard_vals[0]);
        break;
    case 3:
        dcfg->min_sec = UI_DURATION_EDITOR_STEP_SEC;
        dcfg->max_sec = UI_SCHEDULE_REST_MAX_SEC;
        break;
    case 4:
        dcfg->max_sec = wind_down_max_sec(s_wizard_vals[3]);
        break;
    default:
        break;
    }

    clamp_wizard_val(idx);

    if (tcfg != NULL) {
        tcfg->end_time_offset_sec = dcfg->end_time_offset_sec;
        tcfg->max_sec = dcfg->max_sec;
        tcfg->min_sec = dcfg->min_sec;
    }
}

static void refresh_schedule_editors(int idx)
{
    apply_editor_constraints(idx);
    ui_duration_editor_refresh(&s_screens[idx].bundle.editor, &s_screens[idx].bundle.cfg);
    if (s_screens[idx].has_time_editor) {
        ui_time_editor_refresh(&s_screens[idx].time_bundle.editor, &s_screens[idx].time_bundle.cfg);
    }
}

static bool sleep_wake_step_valid(void)
{
    return s_wizard_vals[0] > 0;
}

static bool rest_rest_step_valid(void)
{
    return s_wizard_vals[3] > 0;
}

static bool sleep_wind_down_step_valid(void)
{
    return s_wizard_vals[2] < s_wizard_vals[0];
}

static bool rest_wind_down_step_valid(void)
{
    return s_wizard_vals[4] < s_wizard_vals[3];
}

static void schedule_editor_change_cb(void *user_data)
{
    ui_nav_reset_idle_timer();
    refresh_schedule_editors((int)(intptr_t)user_data);
}

static void schedule_time_change_cb(void *user_data)
{
    ui_nav_reset_idle_timer();
    refresh_schedule_editors((int)(intptr_t)user_data);
}

static void finish_sleep_wizard(void)
{
    if (!sleep_wind_down_step_valid()) {
        return;
    }

    app_config_t *cfg = app_config_get();
    const uint32_t gross_wd = s_wizard_vals[2];

    cfg->wind_down_sec = gross_wd;
    cfg->sleep_sec = s_wizard_vals[0] - gross_wd;
    cfg->rest_sec = s_wizard_vals[1];
    app_config_save_schedule();
    mode_engine_start_cycle();
    ui_nav_go(UI_SCREEN_TOD_BRIGHT);
}

static void finish_rest_wizard(void)
{
    if (!rest_wind_down_step_valid()) {
        return;
    }

    app_config_t *cfg = app_config_get();
    const uint32_t gross_wd = s_wizard_vals[4];

    cfg->wind_down_sec = gross_wd;
    cfg->rest_sec = s_wizard_vals[3] - gross_wd;
    cfg->sleep_sec = 0;
    app_config_save_schedule();
    mode_engine_start_cycle();
    ui_nav_go(UI_SCREEN_TOD_BRIGHT);
}

static void next_cb(lv_event_t *e)
{
    ui_screen_id_t id = (ui_screen_id_t)(uintptr_t)lv_event_get_user_data(e);

    if (id == UI_SCREEN_SLEEP_WAKE) {
        if (!sleep_wake_step_valid()) {
            return;
        }
        ui_nav_go(UI_SCREEN_SLEEP_REST_END);
    } else if (id == UI_SCREEN_SLEEP_REST_END) {
        ui_nav_go(UI_SCREEN_SLEEP_WIND_DOWN);
    } else if (id == UI_SCREEN_SLEEP_WIND_DOWN) {
        finish_sleep_wizard();
    } else if (id == UI_SCREEN_REST_REST_END) {
        if (!rest_rest_step_valid()) {
            return;
        }
        ui_nav_go(UI_SCREEN_REST_WIND_DOWN);
    } else if (id == UI_SCREEN_REST_WIND_DOWN) {
        finish_rest_wizard();
    }
}

static void back_cb(lv_event_t *e)
{
    ui_screen_id_t id = (ui_screen_id_t)(uintptr_t)lv_event_get_user_data(e);
    if (id == UI_SCREEN_SLEEP_WAKE) {
        ui_nav_go(UI_SCREEN_MENU);
    } else if (id == UI_SCREEN_SLEEP_REST_END) {
        ui_nav_go(UI_SCREEN_SLEEP_WAKE);
    } else if (id == UI_SCREEN_SLEEP_WIND_DOWN) {
        ui_nav_go(UI_SCREEN_SLEEP_REST_END);
    } else if (id == UI_SCREEN_REST_REST_END) {
        ui_nav_go(UI_SCREEN_MENU);
    } else if (id == UI_SCREEN_REST_WIND_DOWN) {
        ui_nav_go(UI_SCREEN_REST_REST_END);
    }
}

static void attach_wedges(lv_obj_t *scr, ui_screen_id_t id)
{
    lv_obj_t *cancel = ui_wedge_create(scr, UI_WEDGE_CANCEL);
    lv_obj_add_event_cb(cancel, back_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)id);

    lv_obj_t *next = ui_wedge_create(scr, UI_WEDGE_NEXT);
    lv_obj_add_event_cb(next, next_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)id);

    lv_obj_move_foreground(cancel);
    lv_obj_move_foreground(next);
}

static void build_wizard_screen(lv_obj_t *screens[UI_SCREEN_COUNT], ui_screen_id_t id,
                                const char *title, const char *subtitle, bool show_time_editor,
                                int idx)
{
    schedule_screen_t *ss = &s_screens[idx];

    ss->id = id;
    ss->has_time_editor = show_time_editor;
    ss->scr = ui_widgets_create_screen();
    screens[id] = ss->scr;

    ui_widgets_create_title(ss->scr, title);
    if (subtitle != NULL) {
        ui_widgets_create_subtitle(ss->scr, subtitle);
    }

    const int box_y = show_time_editor ? SCHEDULE_EDITOR_BOX_Y : 220;
    const int box_h = show_time_editor ? SCHEDULE_EDITOR_BOX_H : 100;

    ss->bundle.cfg = (ui_duration_editor_cfg_t){
        .value_sec = &s_wizard_vals[idx],
        .box_y = box_y,
        .box_w = SCHEDULE_EDITOR_BOX_W,
        .box_h = box_h,
        .show_end_time = !show_time_editor,
        .on_change = schedule_editor_change_cb,
        .user_data = (void *)(intptr_t)idx,
    };
    ui_duration_editor_create(ss->scr, &ss->bundle);

    if (show_time_editor) {
        ss->time_bundle.cfg = (ui_time_editor_cfg_t){
            .value_sec = &s_wizard_vals[idx],
            .box_y = REST_TIME_BOX_Y,
            .box_w = UI_TIME_EDITOR_BOX_W,
            .box_h = UI_TIME_EDITOR_BOX_H,
            .on_change = schedule_time_change_cb,
            .user_data = (void *)(intptr_t)idx,
        };
        ui_time_editor_create(ss->scr, &ss->time_bundle);
    }

    attach_wedges(ss->scr, id);
}

void ui_screen_schedule_build(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    build_wizard_screen(screens, UI_SCREEN_SLEEP_WAKE, s_sleep_titles[0], NULL, false, 0);
    build_wizard_screen(screens, UI_SCREEN_SLEEP_REST_END, s_sleep_titles[1], NULL, false, 1);
    build_wizard_screen(screens, UI_SCREEN_SLEEP_WIND_DOWN, s_sleep_titles[2], NULL, false, 2);
    build_wizard_screen(screens, UI_SCREEN_REST_REST_END, s_rest_titles[0], s_rest_subtitles[0], true, 3);
    build_wizard_screen(screens, UI_SCREEN_REST_WIND_DOWN, s_rest_titles[1], s_rest_subtitles[1], true, 4);
}

void ui_screen_schedule_on_show(ui_screen_id_t id)
{
    app_config_t *cfg = app_config_get();
    int idx = screen_index(id);

    /* Load NVS only when entering the first step of each wizard; mid-wizard
     * navigation must keep in-progress edits in s_wizard_vals. Gross values
     * reverse the wind-down subtraction applied at save time. */
    if (id == UI_SCREEN_SLEEP_WAKE) {
        s_wizard_vals[0] = cfg->sleep_sec + cfg->wind_down_sec;
        s_wizard_vals[1] = cfg->rest_sec;
        s_wizard_vals[2] = cfg->wind_down_sec;
    } else if (id == UI_SCREEN_REST_REST_END) {
        s_wizard_vals[3] = cfg->rest_sec + cfg->wind_down_sec;
        s_wizard_vals[4] = cfg->wind_down_sec;
    }

    if (idx >= 0 && idx < 5) {
        refresh_schedule_editors(idx);
    }
}

uint32_t ui_screen_schedule_get_sec(void)
{
    return s_wizard_vals[0];
}

void ui_screen_schedule_set_sec(uint32_t sec)
{
    s_wizard_vals[0] = sec;
}

void ui_screen_schedule_apply_theme(void)
{
    for (int i = 0; i < 5; i++) {
        if (s_screens[i].scr != NULL) {
            ui_widgets_style_circle_panel(s_screens[i].scr);
        }
    }
}
