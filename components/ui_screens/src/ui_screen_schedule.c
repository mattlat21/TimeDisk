#include "ui_screens_registry.h"
#include "ui_layout.h"
#include "ui_widgets.h"
#include "ui_duration_editor.h"
#include "ui_wedge.h"
#include "ui_theme.h"
#include "ui_nav.h"
#include "app_config.h"

#define SCHEDULE_EDITOR_BOX_W  400
#define SCHEDULE_EDITOR_BOX_H  100
#define SCHEDULE_EDITOR_BOX_X  160
#define SCHEDULE_EDITOR_BOX_Y  220

typedef struct {
    lv_obj_t *scr;
    ui_screen_id_t id;
    ui_duration_editor_bundle_t bundle;
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
    "Rest: Rest End Time",
    "Rest: Set Wind Down Time",
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

static void schedule_idle_cb(void *user_data)
{
    (void)user_data;
    ui_nav_reset_idle_timer();
}

static void finish_sleep_wizard(void)
{
    app_config_t *cfg = app_config_get();
    cfg->sleep_sec = s_wizard_vals[0];
    cfg->rest_sec = s_wizard_vals[1];
    cfg->wind_down_sec = s_wizard_vals[2];
    app_config_save_schedule();
    mode_engine_start_cycle();
    ui_nav_go(UI_SCREEN_TOD_BRIGHT);
}

static void finish_rest_wizard(void)
{
    app_config_t *cfg = app_config_get();
    cfg->rest_sec = s_wizard_vals[3];
    cfg->wind_down_sec = s_wizard_vals[4];
    cfg->sleep_sec = 0;
    app_config_save_schedule();
    mode_engine_start_cycle();
    ui_nav_go(UI_SCREEN_TOD_BRIGHT);
}

static void next_cb(lv_event_t *e)
{
    ui_screen_id_t id = (ui_screen_id_t)(uintptr_t)lv_event_get_user_data(e);

    if (id == UI_SCREEN_SLEEP_WAKE) {
        ui_nav_go(UI_SCREEN_SLEEP_REST_END);
    } else if (id == UI_SCREEN_SLEEP_REST_END) {
        ui_nav_go(UI_SCREEN_SLEEP_WIND_DOWN);
    } else if (id == UI_SCREEN_SLEEP_WIND_DOWN) {
        finish_sleep_wizard();
    } else if (id == UI_SCREEN_REST_REST_END) {
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
    const int border = UI_RING_BORDER_DEFAULT;

    lv_obj_t *cancel = ui_wedge_create(
        scr, UI_WEDGE_CANCEL,
        UI_WF_X(UI_WEDGE_CANCEL_X_WF, border),
        UI_WF_Y(UI_WEDGE_CANCEL_Y_WF, border));
    lv_obj_add_event_cb(cancel, back_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)id);

    lv_obj_t *next = ui_wedge_create(
        scr, UI_WEDGE_NEXT,
        UI_WF_X(UI_WEDGE_CONFIRM_X_WF, border),
        UI_WF_Y(UI_WEDGE_CONFIRM_Y_WF, border));
    lv_obj_add_event_cb(next, next_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)id);

    lv_obj_move_foreground(cancel);
    lv_obj_move_foreground(next);
}

static void build_wizard_screen(lv_obj_t *screens[UI_SCREEN_COUNT], ui_screen_id_t id,
                                const char *title, int idx)
{
    schedule_screen_t *ss = &s_screens[idx];

    ss->id = id;
    ss->scr = ui_widgets_create_screen();
    screens[id] = ss->scr;

    ui_widgets_create_title(ss->scr, title);

    ss->bundle.cfg = (ui_duration_editor_cfg_t){
        .value_sec = &s_wizard_vals[idx],
        .box_x = SCHEDULE_EDITOR_BOX_X,
        .box_y = SCHEDULE_EDITOR_BOX_Y,
        .box_w = SCHEDULE_EDITOR_BOX_W,
        .box_h = SCHEDULE_EDITOR_BOX_H,
        .show_end_time = true,
        .on_change = schedule_idle_cb,
    };
    ui_duration_editor_create(ss->scr, &ss->bundle);
    attach_wedges(ss->scr, id);
}

void ui_screen_schedule_build(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    build_wizard_screen(screens, UI_SCREEN_SLEEP_WAKE, s_sleep_titles[0], 0);
    build_wizard_screen(screens, UI_SCREEN_SLEEP_REST_END, s_sleep_titles[1], 1);
    build_wizard_screen(screens, UI_SCREEN_SLEEP_WIND_DOWN, s_sleep_titles[2], 2);
    build_wizard_screen(screens, UI_SCREEN_REST_REST_END, s_rest_titles[0], 3);
    build_wizard_screen(screens, UI_SCREEN_REST_WIND_DOWN, s_rest_titles[1], 4);
}

void ui_screen_schedule_on_show(ui_screen_id_t id)
{
    app_config_t *cfg = app_config_get();
    int idx = screen_index(id);

    s_wizard_vals[0] = cfg->sleep_sec;
    s_wizard_vals[1] = cfg->rest_sec;
    s_wizard_vals[2] = cfg->wind_down_sec;
    s_wizard_vals[3] = cfg->rest_sec;
    s_wizard_vals[4] = cfg->wind_down_sec;

    if (idx >= 0 && idx < 5) {
        ui_duration_editor_refresh(&s_screens[idx].bundle.editor, &s_screens[idx].bundle.cfg);
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
