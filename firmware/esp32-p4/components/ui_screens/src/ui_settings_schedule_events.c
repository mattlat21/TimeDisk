/**
 * @file ui_settings_schedule_events.c
 * @brief Settings -> Schedule -> wall-clock events list and editor.
 */

#include "ui_screen_settings_internal.h"

#include "ui_duration_editor.h"
#include "ui_format.h"
#include "ui_layout.h"
#include "ui_theme.h"
#include "ui_time_editor.h"
#include "ui_widgets.h"
#include "ui_wedge.h"

#include <stdio.h>
#include <string.h>

#define EVT_BTN_W           248
#define EVT_BTN_H           44
#define EVT_BTN_GAP_Y       6
#define EVT_BTN_RADIUS      16
#define EVT_LIST_Y0_WF      120
#define EVT_MANAGE_Y_WF     520
#define EVT_EDIT_TIME_Y_WF  150
#define EVT_EDIT_ACTION_Y0  260
#define EVT_EDIT_DUR_Y_WF   380
#define EVT_EDIT_ENABLED_Y  500
#define EVT_EDIT_DELETE_Y   560

typedef enum {
    SCHED_EVT_VIEW_LIST = 0,
    SCHED_EVT_VIEW_EDIT,
} sched_evt_view_t;

static lv_obj_t *s_panel;
static lv_obj_t *s_panel_title;
static lv_obj_t *s_list_btns[APP_SCHEDULE_EVENT_MAX];
static lv_obj_t *s_add_btn;
static lv_obj_t *s_action_btns[3];
static lv_obj_t *s_enabled_btn;
static lv_obj_t *s_delete_btn;
static lv_obj_t *s_edit_action_lbl;
static ui_wedge_t *s_evt_cancel_wedge;
static ui_wedge_t *s_evt_save_wedge;
static ui_time_editor_bundle_t s_time_bundle;
static ui_duration_editor_bundle_t s_dur_bundle;
static uint32_t s_edit_time_sec;
static uint32_t s_edit_duration_sec;
static sched_evt_view_t s_view = SCHED_EVT_VIEW_LIST;
static int s_sel = -1;
static bool s_adding = false;
static bool s_events_ui_open = false;
static app_schedule_event_t s_edit_event;

static const char *s_action_labels[3] = {"Wake", "Sleep", "Rest"};

static void sched_evt_show_list(void);
static void sched_evt_show_edit(int index, bool adding);
static void sched_evt_hide_list_rows(void);
static void sched_evt_hide_edit_rows(void);
static void sched_evt_refresh_action_ui(void);
static void sched_evt_refresh_duration_ui(void);
static void sched_evt_format_row(int index, char *buf, size_t len);

void ui_settings_schedule_events_init(lv_obj_t *panel, lv_obj_t *panel_title)
{
    const ui_theme_t *t = ui_theme_get();

    s_panel = panel;
    s_panel_title = panel_title;

    for (int i = 0; i < APP_SCHEDULE_EVENT_MAX; i++) {
        lv_obj_t *btn = lv_button_create(panel);
        lv_obj_set_size(btn, EVT_BTN_W, EVT_BTN_H);
        lv_obj_set_style_radius(btn, EVT_BTN_RADIUS, 0);
        lv_obj_set_style_bg_color(btn, t->menu_petal, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_HIDDEN);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, "");
        lv_obj_set_style_text_color(lbl, t->white, 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0);
        lv_obj_center(lbl);

        s_list_btns[i] = btn;
    }

    s_add_btn = lv_button_create(panel);
    lv_obj_set_size(s_add_btn, EVT_BTN_H, EVT_BTN_H);
    lv_obj_set_style_radius(s_add_btn, EVT_BTN_RADIUS, 0);
    lv_obj_set_style_bg_color(s_add_btn, t->menu_petal, 0);
    lv_obj_add_flag(s_add_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *plus = lv_label_create(s_add_btn);
    lv_label_set_text(plus, LV_SYMBOL_PLUS);
    lv_obj_set_style_text_color(plus, t->white, 0);
    lv_obj_center(plus);

    s_edit_action_lbl = lv_label_create(panel);
    lv_label_set_text(s_edit_action_lbl, "Action");
    lv_obj_set_style_text_color(s_edit_action_lbl, t->white, 0);
    lv_obj_set_style_text_font(s_edit_action_lbl, &lv_font_montserrat_18, 0);
    lv_obj_align(s_edit_action_lbl, LV_ALIGN_TOP_MID, 0, ui_settings_wf_y(panel, EVT_EDIT_ACTION_Y0 - 28));
    lv_obj_add_flag(s_edit_action_lbl, LV_OBJ_FLAG_HIDDEN);

    for (int i = 0; i < 3; i++) {
        lv_obj_t *btn = lv_button_create(panel);
        lv_obj_set_size(btn, 120, EVT_BTN_H);
        lv_obj_set_style_radius(btn, EVT_BTN_RADIUS, 0);
        lv_obj_set_style_bg_color(btn, t->menu_petal, 0);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, s_action_labels[i]);
        lv_obj_set_style_text_color(lbl, t->white, 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
        lv_obj_center(lbl);
        s_action_btns[i] = btn;
    }

    s_enabled_btn = lv_button_create(panel);
    lv_obj_set_size(s_enabled_btn, EVT_BTN_W, EVT_BTN_H);
    lv_obj_set_style_radius(s_enabled_btn, EVT_BTN_RADIUS, 0);
    lv_obj_set_style_bg_color(s_enabled_btn, t->menu_petal, 0);
    lv_obj_add_flag(s_enabled_btn, LV_OBJ_FLAG_HIDDEN);

    s_delete_btn = lv_button_create(panel);
    lv_obj_set_size(s_delete_btn, EVT_BTN_W, EVT_BTN_H);
    lv_obj_set_style_radius(s_delete_btn, EVT_BTN_RADIUS, 0);
    lv_obj_set_style_bg_color(s_delete_btn, t->menu_petal, 0);
    lv_obj_set_style_bg_color(s_delete_btn, t->white, 0);
    lv_obj_add_flag(s_delete_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *del_lbl = lv_label_create(s_delete_btn);
    lv_label_set_text(del_lbl, "Delete");
    lv_obj_set_style_text_color(del_lbl, t->menu_petal, 0);
    lv_obj_center(del_lbl);

    s_time_bundle.cfg = (ui_time_editor_cfg_t){
        .value_sec = &s_edit_time_sec,
        .box_y = EVT_EDIT_TIME_Y_WF,
        .box_w = 400,
        .box_h = 130,
        .max_sec = (23U * 3600U) + (59U * 60U),
        .on_change = ui_settings_idle_cb,
    };
    ui_time_editor_create(panel, &s_time_bundle);
    ui_time_editor_set_visible(&s_time_bundle.editor, false);

    s_dur_bundle.cfg = (ui_duration_editor_cfg_t){
        .value_sec = &s_edit_duration_sec,
        .box_y = EVT_EDIT_DUR_Y_WF,
        .box_w = 400,
        .box_h = 72,
        .show_end_time = false,
        .max_sec = UI_SCHEDULE_REST_MAX_SEC,
        .on_change = ui_settings_idle_cb,
    };
    ui_duration_editor_create(panel, &s_dur_bundle);
    ui_duration_editor_set_visible(&s_dur_bundle.editor, false);

    s_evt_cancel_wedge = ui_wedge_create_overlay(ui_settings_screen(), UI_WEDGE_CANCEL);
    s_evt_save_wedge = ui_wedge_create_overlay(ui_settings_screen(), UI_WEDGE_CONFIRM);
    if (s_evt_cancel_wedge != NULL) {
        ui_wedge_set_visible(s_evt_cancel_wedge, false);
    }
    if (s_evt_save_wedge != NULL) {
        ui_wedge_set_visible(s_evt_save_wedge, false);
    }
}

static void sched_evt_format_time_12h(uint16_t time_min, char *buf, size_t len)
{
    int h24 = (int)(time_min / 60);
    int min = (int)(time_min % 60);
    int h12 = h24 % 12;
    if (h12 == 0) {
        h12 = 12;
    }
    snprintf(buf, len, "%d:%02d %s", h12, min, (h24 >= 12) ? "PM" : "AM");
}

static void sched_evt_format_row(int index, char *buf, size_t len)
{
    app_config_t *draft = ui_settings_draft();
    if (index < 0 || index >= (int)draft->schedule_event_count) {
        snprintf(buf, len, "");
        return;
    }

    const app_schedule_event_t *ev = &draft->schedule_events[index];
    char time_buf[16];
    sched_evt_format_time_12h(ev->time_min, time_buf, sizeof(time_buf));

    const char *action = "Wake";
    if (ev->action == APP_SCHEDULE_ACTION_START_SLEEP) {
        action = "Sleep";
    } else if (ev->action == APP_SCHEDULE_ACTION_START_REST) {
        action = "Rest";
    }

    if (!ev->enabled) {
        snprintf(buf, len, "%s %s (off)", time_buf, action);
        return;
    }

    if (ev->action == APP_SCHEDULE_ACTION_WAKE || ev->duration_sec == 0) {
        snprintf(buf, len, "%s %s", time_buf, action);
        return;
    }

    char dur_buf[32];
    ui_format_duration_human(dur_buf, sizeof(dur_buf), ev->duration_sec);
    snprintf(buf, len, "%s %s (%s)", time_buf, action, dur_buf);
}

static void sched_evt_hide_list_rows(void)
{
    for (int i = 0; i < APP_SCHEDULE_EVENT_MAX; i++) {
        if (s_list_btns[i] != NULL) {
            lv_obj_add_flag(s_list_btns[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (s_add_btn != NULL) {
        lv_obj_add_flag(s_add_btn, LV_OBJ_FLAG_HIDDEN);
    }
}

static void sched_evt_hide_edit_rows(void)
{
    ui_time_editor_set_visible(&s_time_bundle.editor, false);
    ui_duration_editor_set_visible(&s_dur_bundle.editor, false);
    if (s_edit_action_lbl != NULL) {
        lv_obj_add_flag(s_edit_action_lbl, LV_OBJ_FLAG_HIDDEN);
    }
    for (int i = 0; i < 3; i++) {
        if (s_action_btns[i] != NULL) {
            lv_obj_add_flag(s_action_btns[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (s_enabled_btn != NULL) {
        lv_obj_add_flag(s_enabled_btn, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_delete_btn != NULL) {
        lv_obj_add_flag(s_delete_btn, LV_OBJ_FLAG_HIDDEN);
    }
}

static void sched_evt_list_row_cb(lv_event_t *e)
{
    int index = (int)(intptr_t)lv_event_get_user_data(e);
    sched_evt_show_edit(index, false);
    ui_settings_idle_cb(NULL);
}

static void sched_evt_add_cb(lv_event_t *e)
{
    (void)e;
    app_config_t *draft = ui_settings_draft();
    if (draft->schedule_event_count >= APP_SCHEDULE_EVENT_MAX) {
        return;
    }
    sched_evt_show_edit((int)draft->schedule_event_count, true);
    ui_settings_idle_cb(NULL);
}

static void sched_evt_action_cb(lv_event_t *e)
{
    int action = (int)(intptr_t)lv_event_get_user_data(e);
    if (action < 0 || action > APP_SCHEDULE_ACTION_START_REST) {
        return;
    }
    s_edit_event.action = (uint8_t)action;
    sched_evt_refresh_action_ui();
    sched_evt_refresh_duration_ui();
    ui_settings_idle_cb(NULL);
}

static void sched_evt_enabled_cb(lv_event_t *e)
{
    (void)e;
    s_edit_event.enabled = !s_edit_event.enabled;
    sched_evt_refresh_action_ui();
    ui_settings_idle_cb(NULL);
}

static void sched_evt_delete_cb(lv_event_t *e)
{
    (void)e;
    app_config_t *draft = ui_settings_draft();
    if (s_sel >= 0 && s_sel < (int)draft->schedule_event_count) {
        for (int i = s_sel; i < (int)draft->schedule_event_count - 1; i++) {
            draft->schedule_events[i] = draft->schedule_events[i + 1];
        }
        if (draft->schedule_event_count > 0) {
            draft->schedule_event_count--;
        }
        app_config_schedule_events_sort_buf(draft->schedule_events, draft->schedule_event_count);
    }
    sched_evt_show_list();
    ui_settings_idle_cb(NULL);
}

static void sched_evt_refresh_action_ui(void)
{
    const ui_theme_t *t = ui_theme_get();
    for (int i = 0; i < 3; i++) {
        if (s_action_btns[i] == NULL) {
            continue;
        }
        lv_obj_set_style_bg_color(s_action_btns[i],
                                  (s_edit_event.action == (uint8_t)i) ? t->secondary : t->menu_petal, 0);
    }
    if (s_enabled_btn != NULL) {
        lv_label_set_text(lv_obj_get_child(s_enabled_btn, 0),
                          s_edit_event.enabled ? "Enabled" : "Disabled");
    }
}

static void sched_evt_refresh_duration_ui(void)
{
    const bool show = s_edit_event.action != APP_SCHEDULE_ACTION_WAKE;
    ui_duration_editor_set_visible(&s_dur_bundle.editor, show);
    if (show) {
        ui_duration_editor_refresh(&s_dur_bundle.editor, &s_dur_bundle.cfg);
    }
}

static void sched_evt_cancel_cb(lv_event_t *e)
{
    (void)e;
    if (s_view == SCHED_EVT_VIEW_EDIT) {
        sched_evt_show_list();
        return;
    }

    ui_settings_schedule_events_restore_saved();
    ui_settings_schedule_events_close();
}

static void sched_evt_save_cb(lv_event_t *e)
{
    (void)e;
    app_config_t *cfg = app_config_get();
    app_config_t *draft = ui_settings_draft();
    app_config_t *saved = ui_settings_saved();

    if (s_view == SCHED_EVT_VIEW_EDIT) {
        s_edit_event.time_min = (uint16_t)(s_edit_time_sec / 60U);
        if (s_edit_event.action == APP_SCHEDULE_ACTION_WAKE) {
            s_edit_event.duration_sec = 0;
        } else {
            s_edit_event.duration_sec = s_edit_duration_sec;
        }

        if (s_adding) {
            if (draft->schedule_event_count < APP_SCHEDULE_EVENT_MAX) {
                draft->schedule_events[draft->schedule_event_count++] = s_edit_event;
            }
        } else if (s_sel >= 0 && s_sel < (int)draft->schedule_event_count) {
            draft->schedule_events[s_sel] = s_edit_event;
        }
        app_config_schedule_events_sort_buf(draft->schedule_events, draft->schedule_event_count);
        sched_evt_show_list();
        return;
    }

    app_config_schedule_events_sort_buf(draft->schedule_events, draft->schedule_event_count);
    memcpy(cfg->schedule_events, draft->schedule_events, sizeof(cfg->schedule_events));
    cfg->schedule_event_count = draft->schedule_event_count;
    memcpy(saved->schedule_events, cfg->schedule_events, sizeof(saved->schedule_events));
    saved->schedule_event_count = cfg->schedule_event_count;
    app_config_save_schedule();
    ui_settings_schedule_events_close();
}

static void sched_evt_bind_wedges(void)
{
    if (s_evt_cancel_wedge != NULL) {
        ui_wedge_bind(s_evt_cancel_wedge, UI_WEDGE_CANCEL, sched_evt_cancel_cb, NULL);
        ui_wedge_set_visible(s_evt_cancel_wedge, true);
    }
    if (s_evt_save_wedge != NULL) {
        ui_wedge_bind(s_evt_save_wedge, UI_WEDGE_CONFIRM, sched_evt_save_cb, NULL);
        ui_wedge_set_visible(s_evt_save_wedge, true);
    }
    ui_settings_set_panel_wedges_visible(PANEL_SCHEDULE, false);
}

static void sched_evt_show_list(void)
{
    app_config_t *draft = ui_settings_draft();
    app_config_schedule_events_sort_buf(draft->schedule_events, draft->schedule_event_count);
    s_view = SCHED_EVT_VIEW_LIST;
    s_sel = -1;
    s_adding = false;
    s_events_ui_open = true;

    sched_evt_hide_edit_rows();
    ui_settings_schedule_hide_durations(true);

    if (s_panel_title != NULL) {
        lv_label_set_text(s_panel_title, "Scheduled events");
    }

    char row_text[96];
    int row = 0;
    for (int i = 0; i < (int)draft->schedule_event_count; i++) {
        if (s_list_btns[i] == NULL) {
            continue;
        }
        sched_evt_format_row(i, row_text, sizeof(row_text));
        lv_label_set_text(lv_obj_get_child(s_list_btns[i], 0), row_text);
        lv_obj_set_pos(s_list_btns[i],
                       ui_layout_parent_center_x_wf(s_panel, EVT_BTN_W),
                       ui_settings_wf_y(s_panel, EVT_LIST_Y0_WF + row * (EVT_BTN_H + EVT_BTN_GAP_Y)));
        lv_obj_remove_event_cb(s_list_btns[i], sched_evt_list_row_cb);
        lv_obj_add_event_cb(s_list_btns[i], sched_evt_list_row_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        lv_obj_clear_flag(s_list_btns[i], LV_OBJ_FLAG_HIDDEN);
        row++;
    }
    for (int i = (int)draft->schedule_event_count; i < APP_SCHEDULE_EVENT_MAX; i++) {
        if (s_list_btns[i] != NULL) {
            lv_obj_add_flag(s_list_btns[i], LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (s_add_btn != NULL) {
        if (draft->schedule_event_count < APP_SCHEDULE_EVENT_MAX) {
            lv_obj_align(s_add_btn, LV_ALIGN_TOP_MID, 0,
                         ui_settings_wf_y(s_panel, EVT_LIST_Y0_WF + row * (EVT_BTN_H + EVT_BTN_GAP_Y) + 8));
            lv_obj_remove_event_cb(s_add_btn, sched_evt_add_cb);
            lv_obj_add_event_cb(s_add_btn, sched_evt_add_cb, LV_EVENT_CLICKED, NULL);
            lv_obj_clear_flag(s_add_btn, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_add_btn, LV_OBJ_FLAG_HIDDEN);
        }
    }

    sched_evt_bind_wedges();
    ui_settings_idle_cb(NULL);
}

static void sched_evt_show_edit(int index, bool adding)
{
    app_config_t *draft = ui_settings_draft();
    s_view = SCHED_EVT_VIEW_EDIT;
    s_sel = index;
    s_adding = adding;

    sched_evt_hide_list_rows();

    if (adding) {
        s_edit_event = (app_schedule_event_t){
            .time_min = 7U * 60U,
            .action = APP_SCHEDULE_ACTION_WAKE,
            .enabled = true,
            .duration_sec = 0,
        };
    } else if (index >= 0 && index < (int)draft->schedule_event_count) {
        s_edit_event = draft->schedule_events[index];
    }

    s_edit_time_sec = (uint32_t)s_edit_event.time_min * 60U;
    s_edit_duration_sec = s_edit_event.duration_sec;
    if (s_edit_duration_sec == 0 && s_edit_event.action == APP_SCHEDULE_ACTION_START_SLEEP) {
        s_edit_duration_sec = draft->sleep_sec;
    } else if (s_edit_duration_sec == 0 && s_edit_event.action == APP_SCHEDULE_ACTION_START_REST) {
        s_edit_duration_sec = draft->rest_sec;
    }

    if (s_panel_title != NULL) {
        lv_label_set_text(s_panel_title, adding ? "Add event" : "Edit event");
    }

    ui_time_editor_set_visible(&s_time_bundle.editor, true);
    ui_time_editor_refresh(&s_time_bundle.editor, &s_time_bundle.cfg);

    if (s_edit_action_lbl != NULL) {
        lv_obj_clear_flag(s_edit_action_lbl, LV_OBJ_FLAG_HIDDEN);
    }
    for (int i = 0; i < 3; i++) {
        if (s_action_btns[i] == NULL) {
            continue;
        }
        const int x_off = -130 + i * 130;
        lv_obj_align(s_action_btns[i], LV_ALIGN_TOP_MID, x_off,
                     ui_settings_wf_y(s_panel, EVT_EDIT_ACTION_Y0));
        lv_obj_remove_event_cb(s_action_btns[i], sched_evt_action_cb);
        lv_obj_add_event_cb(s_action_btns[i], sched_evt_action_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        lv_obj_clear_flag(s_action_btns[i], LV_OBJ_FLAG_HIDDEN);
    }

    if (s_enabled_btn != NULL) {
        if (lv_obj_get_child_cnt(s_enabled_btn) == 0) {
            const ui_theme_t *t = ui_theme_get();
            lv_obj_t *lbl = lv_label_create(s_enabled_btn);
            lv_label_set_text(lbl, "Enabled");
            lv_obj_set_style_text_color(lbl, t->white, 0);
            lv_obj_center(lbl);
        }
        lv_obj_align(s_enabled_btn, LV_ALIGN_TOP_MID, 0, ui_settings_wf_y(s_panel, EVT_EDIT_ENABLED_Y));
        lv_obj_remove_event_cb(s_enabled_btn, sched_evt_enabled_cb);
        lv_obj_add_event_cb(s_enabled_btn, sched_evt_enabled_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_clear_flag(s_enabled_btn, LV_OBJ_FLAG_HIDDEN);
    }

    if (s_delete_btn != NULL && !adding) {
        lv_obj_align(s_delete_btn, LV_ALIGN_TOP_MID, 0, ui_settings_wf_y(s_panel, EVT_EDIT_DELETE_Y));
        lv_obj_remove_event_cb(s_delete_btn, sched_evt_delete_cb);
        lv_obj_add_event_cb(s_delete_btn, sched_evt_delete_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_clear_flag(s_delete_btn, LV_OBJ_FLAG_HIDDEN);
    } else if (s_delete_btn != NULL) {
        lv_obj_add_flag(s_delete_btn, LV_OBJ_FLAG_HIDDEN);
    }

    sched_evt_refresh_action_ui();
    sched_evt_refresh_duration_ui();
    sched_evt_bind_wedges();
    ui_settings_idle_cb(NULL);
}

void ui_settings_schedule_events_open(void)
{
    sched_evt_show_list();
}

void ui_settings_schedule_events_close(void)
{
    s_view = SCHED_EVT_VIEW_LIST;
    s_events_ui_open = false;
    sched_evt_hide_list_rows();
    sched_evt_hide_edit_rows();
    ui_settings_schedule_hide_durations(false);
    if (s_evt_cancel_wedge != NULL) {
        ui_wedge_set_visible(s_evt_cancel_wedge, false);
    }
    if (s_evt_save_wedge != NULL) {
        ui_wedge_set_visible(s_evt_save_wedge, false);
    }
    ui_settings_set_panel_wedges_visible(PANEL_SCHEDULE, true);
    if (s_panel_title != NULL) {
        lv_label_set_text(s_panel_title, "Schedule");
    }
}

bool ui_settings_schedule_events_try_cancel(void)
{
    if (!s_events_ui_open) {
        return false;
    }
    sched_evt_cancel_cb(NULL);
    return true;
}

void ui_settings_schedule_events_restore_saved(void)
{
    app_config_t *draft = ui_settings_draft();
    app_config_t *saved = ui_settings_saved();
    app_config_schedule_events_copy(draft->schedule_events, &draft->schedule_event_count,
                                    saved->schedule_events, saved->schedule_event_count);
}
