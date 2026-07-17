/**
 * @file ui_settings_scheduled_buttons.c
 * @brief Settings -> Schedule -> TOD scheduled buttons list and editor.
 */

#include "ui_screen_settings_internal.h"
#include "ui_screens_registry.h"

#include "ui_layout.h"
#include "ui_theme.h"
#include "ui_time_editor.h"
#include "ui_widgets.h"
#include "ui_wedge.h"
#include "app_scheduled_button.h"

#include <stdio.h>
#include <string.h>

#define BTN_W              248
#define BTN_H              44
#define BTN_GAP_Y          6
#define BTN_RADIUS         16
#define LIST_Y0_WF         120
#define EDIT_START_Y_WF    120
#define EDIT_END_Y_WF      250
#define EDIT_ACTION_Y0     370
#define EDIT_MODES_Y0      440
#define EDIT_ENABLED_Y     510
#define EDIT_DELETE_Y      570

typedef enum {
    SCHED_BTN_VIEW_LIST = 0,
    SCHED_BTN_VIEW_EDIT,
} sched_btn_view_t;

static lv_obj_t *s_panel;
static lv_obj_t *s_panel_title;
static lv_obj_t *s_list_btns[APP_SCHEDULED_BUTTON_MAX];
static lv_obj_t *s_add_btn;
static lv_obj_t *s_action_btns[4];
static lv_obj_t *s_mode_btns[4];
static lv_obj_t *s_enabled_btn;
static lv_obj_t *s_delete_btn;
static lv_obj_t *s_edit_action_lbl;
static lv_obj_t *s_edit_modes_lbl;
static lv_obj_t *s_start_lbl;
static lv_obj_t *s_end_lbl;
static ui_wedge_t *s_cancel_wedge;
static ui_wedge_t *s_save_wedge;
static ui_time_editor_bundle_t s_start_bundle;
static ui_time_editor_bundle_t s_end_bundle;
static uint32_t s_edit_start_sec;
static uint32_t s_edit_end_sec;
static sched_btn_view_t s_view = SCHED_BTN_VIEW_LIST;
static int s_sel = -1;
static bool s_adding = false;
static bool s_ui_open = false;
static app_scheduled_button_t s_edit_btn;

static const char *s_action_labels[4] = {"Wake", "Sleep", "Rest", "Wind Down"};
static const uint8_t s_action_values[4] = {
    APP_SCHEDULE_ACTION_WAKE,
    APP_SCHEDULE_ACTION_START_SLEEP,
    APP_SCHEDULE_ACTION_START_REST,
    APP_SCHEDULE_ACTION_START_WIND_DOWN,
};
static const char *s_mode_labels[4] = {"Wake", "Wind Down", "Sleep", "Rest"};
static const uint8_t s_mode_bits[4] = {
    APP_MODE_BIT(APP_MODE_WAKE),
    APP_MODE_BIT(APP_MODE_WIND_DOWN),
    APP_MODE_BIT(APP_MODE_SLEEP),
    APP_MODE_BIT(APP_MODE_REST),
};

static void sched_btn_show_list(void);
static void sched_btn_show_edit(int index, bool adding);
static void sched_btn_hide_list_rows(void);
static void sched_btn_hide_edit_rows(void);
static void sched_btn_refresh_action_ui(void);
static void sched_btn_refresh_modes_ui(void);

void ui_settings_scheduled_buttons_init(lv_obj_t *panel, lv_obj_t *panel_title)
{
    const ui_theme_t *t = ui_theme_get();

    s_panel = panel;
    s_panel_title = panel_title;

    for (int i = 0; i < APP_SCHEDULED_BUTTON_MAX; i++) {
        lv_obj_t *btn = lv_button_create(panel);
        lv_obj_set_size(btn, BTN_W, BTN_H);
        lv_obj_set_style_radius(btn, BTN_RADIUS, 0);
        lv_obj_set_style_bg_color(btn, t->menu_petal, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_HIDDEN);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, "");
        lv_obj_set_style_text_color(lbl, t->white, 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
        lv_obj_center(lbl);
        s_list_btns[i] = btn;
    }

    s_add_btn = lv_button_create(panel);
    lv_obj_set_size(s_add_btn, BTN_H, BTN_H);
    lv_obj_set_style_radius(s_add_btn, BTN_RADIUS, 0);
    lv_obj_set_style_bg_color(s_add_btn, t->menu_petal, 0);
    lv_obj_add_flag(s_add_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *plus = lv_label_create(s_add_btn);
    lv_label_set_text(plus, LV_SYMBOL_PLUS);
    lv_obj_set_style_text_color(plus, t->white, 0);
    lv_obj_center(plus);

    s_start_lbl = lv_label_create(panel);
    lv_label_set_text(s_start_lbl, "Start");
    lv_obj_set_style_text_color(s_start_lbl, t->white, 0);
    lv_obj_set_style_text_font(s_start_lbl, &lv_font_montserrat_16, 0);
    lv_obj_align(s_start_lbl, LV_ALIGN_TOP_MID, 0, ui_settings_wf_y(panel, EDIT_START_Y_WF - 22));
    lv_obj_add_flag(s_start_lbl, LV_OBJ_FLAG_HIDDEN);

    s_end_lbl = lv_label_create(panel);
    lv_label_set_text(s_end_lbl, "End");
    lv_obj_set_style_text_color(s_end_lbl, t->white, 0);
    lv_obj_set_style_text_font(s_end_lbl, &lv_font_montserrat_16, 0);
    lv_obj_align(s_end_lbl, LV_ALIGN_TOP_MID, 0, ui_settings_wf_y(panel, EDIT_END_Y_WF - 22));
    lv_obj_add_flag(s_end_lbl, LV_OBJ_FLAG_HIDDEN);

    s_edit_action_lbl = lv_label_create(panel);
    lv_label_set_text(s_edit_action_lbl, "Start mode");
    lv_obj_set_style_text_color(s_edit_action_lbl, t->white, 0);
    lv_obj_set_style_text_font(s_edit_action_lbl, &lv_font_montserrat_16, 0);
    lv_obj_align(s_edit_action_lbl, LV_ALIGN_TOP_MID, 0, ui_settings_wf_y(panel, EDIT_ACTION_Y0 - 22));
    lv_obj_add_flag(s_edit_action_lbl, LV_OBJ_FLAG_HIDDEN);

    for (int i = 0; i < 4; i++) {
        lv_obj_t *btn = lv_button_create(panel);
        lv_obj_set_size(btn, 110, 36);
        lv_obj_set_style_radius(btn, BTN_RADIUS, 0);
        lv_obj_set_style_bg_color(btn, t->menu_petal, 0);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, s_action_labels[i]);
        lv_obj_set_style_text_color(lbl, t->white, 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
        lv_obj_center(lbl);
        s_action_btns[i] = btn;
    }

    s_edit_modes_lbl = lv_label_create(panel);
    lv_label_set_text(s_edit_modes_lbl, "Show in modes");
    lv_obj_set_style_text_color(s_edit_modes_lbl, t->white, 0);
    lv_obj_set_style_text_font(s_edit_modes_lbl, &lv_font_montserrat_16, 0);
    lv_obj_align(s_edit_modes_lbl, LV_ALIGN_TOP_MID, 0, ui_settings_wf_y(panel, EDIT_MODES_Y0 - 22));
    lv_obj_add_flag(s_edit_modes_lbl, LV_OBJ_FLAG_HIDDEN);

    for (int i = 0; i < 4; i++) {
        lv_obj_t *btn = lv_button_create(panel);
        lv_obj_set_size(btn, 110, 36);
        lv_obj_set_style_radius(btn, BTN_RADIUS, 0);
        lv_obj_set_style_bg_color(btn, t->menu_petal, 0);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, s_mode_labels[i]);
        lv_obj_set_style_text_color(lbl, t->white, 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
        lv_obj_center(lbl);
        s_mode_btns[i] = btn;
    }

    s_enabled_btn = lv_button_create(panel);
    lv_obj_set_size(s_enabled_btn, BTN_W, BTN_H);
    lv_obj_set_style_radius(s_enabled_btn, BTN_RADIUS, 0);
    lv_obj_set_style_bg_color(s_enabled_btn, t->menu_petal, 0);
    lv_obj_add_flag(s_enabled_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *en_lbl = lv_label_create(s_enabled_btn);
    lv_label_set_text(en_lbl, "Enabled");
    lv_obj_set_style_text_color(en_lbl, t->white, 0);
    lv_obj_center(en_lbl);

    s_delete_btn = lv_button_create(panel);
    lv_obj_set_size(s_delete_btn, BTN_W, BTN_H);
    lv_obj_set_style_radius(s_delete_btn, BTN_RADIUS, 0);
    lv_obj_set_style_bg_color(s_delete_btn, t->white, 0);
    lv_obj_add_flag(s_delete_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *del_lbl = lv_label_create(s_delete_btn);
    lv_label_set_text(del_lbl, "Delete");
    lv_obj_set_style_text_color(del_lbl, t->menu_petal, 0);
    lv_obj_center(del_lbl);

    s_start_bundle.cfg = (ui_time_editor_cfg_t){
        .value_sec = &s_edit_start_sec,
        .box_y = EDIT_START_Y_WF,
        .box_w = 360,
        .box_h = 100,
        .max_sec = (23U * 3600U) + (59U * 60U),
        .on_change = ui_settings_idle_cb,
    };
    ui_time_editor_create(panel, &s_start_bundle);
    ui_time_editor_set_visible(&s_start_bundle.editor, false);

    s_end_bundle.cfg = (ui_time_editor_cfg_t){
        .value_sec = &s_edit_end_sec,
        .box_y = EDIT_END_Y_WF,
        .box_w = 360,
        .box_h = 100,
        .max_sec = (23U * 3600U) + (59U * 60U),
        .on_change = ui_settings_idle_cb,
    };
    ui_time_editor_create(panel, &s_end_bundle);
    ui_time_editor_set_visible(&s_end_bundle.editor, false);

    s_cancel_wedge = ui_wedge_create_overlay(ui_settings_screen(), UI_WEDGE_CANCEL);
    s_save_wedge = ui_wedge_create_overlay(ui_settings_screen(), UI_WEDGE_CONFIRM);
    if (s_cancel_wedge != NULL) {
        ui_wedge_set_visible(s_cancel_wedge, false);
    }
    if (s_save_wedge != NULL) {
        ui_wedge_set_visible(s_save_wedge, false);
    }
}

static void format_time_12h(uint16_t time_min, char *buf, size_t len)
{
    int h24 = (int)(time_min / 60);
    int min = (int)(time_min % 60);
    int h12 = h24 % 12;
    if (h12 == 0) {
        h12 = 12;
    }
    snprintf(buf, len, "%d:%02d%s", h12, min, (h24 >= 12) ? "p" : "a");
}

static const char *action_short(uint8_t action)
{
    switch (action) {
    case APP_SCHEDULE_ACTION_WAKE:
        return "Wake";
    case APP_SCHEDULE_ACTION_START_SLEEP:
        return "Sleep";
    case APP_SCHEDULE_ACTION_START_REST:
        return "Rest";
    case APP_SCHEDULE_ACTION_START_WIND_DOWN:
        return "WindDn";
    default:
        return "?";
    }
}

static void format_row(int index, char *buf, size_t len)
{
    app_config_t *draft = ui_settings_draft();
    if (index < 0 || index >= (int)draft->scheduled_button_count) {
        snprintf(buf, len, "");
        return;
    }

    const app_scheduled_button_t *btn = &draft->scheduled_buttons[index];
    char start_buf[12];
    char end_buf[12];
    format_time_12h(btn->start_min, start_buf, sizeof(start_buf));
    format_time_12h(btn->end_min, end_buf, sizeof(end_buf));

    if (!btn->enabled) {
        snprintf(buf, len, "%s-%s %s (off)", start_buf, end_buf, action_short(btn->action));
    } else {
        snprintf(buf, len, "%s-%s %s", start_buf, end_buf, action_short(btn->action));
    }
}

static void sched_btn_hide_list_rows(void)
{
    for (int i = 0; i < APP_SCHEDULED_BUTTON_MAX; i++) {
        if (s_list_btns[i] != NULL) {
            lv_obj_add_flag(s_list_btns[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (s_add_btn != NULL) {
        lv_obj_add_flag(s_add_btn, LV_OBJ_FLAG_HIDDEN);
    }
}

static void sched_btn_hide_edit_rows(void)
{
    ui_time_editor_set_visible(&s_start_bundle.editor, false);
    ui_time_editor_set_visible(&s_end_bundle.editor, false);
    if (s_start_lbl != NULL) {
        lv_obj_add_flag(s_start_lbl, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_end_lbl != NULL) {
        lv_obj_add_flag(s_end_lbl, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_edit_action_lbl != NULL) {
        lv_obj_add_flag(s_edit_action_lbl, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_edit_modes_lbl != NULL) {
        lv_obj_add_flag(s_edit_modes_lbl, LV_OBJ_FLAG_HIDDEN);
    }
    for (int i = 0; i < 4; i++) {
        if (s_action_btns[i] != NULL) {
            lv_obj_add_flag(s_action_btns[i], LV_OBJ_FLAG_HIDDEN);
        }
        if (s_mode_btns[i] != NULL) {
            lv_obj_add_flag(s_mode_btns[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (s_enabled_btn != NULL) {
        lv_obj_add_flag(s_enabled_btn, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_delete_btn != NULL) {
        lv_obj_add_flag(s_delete_btn, LV_OBJ_FLAG_HIDDEN);
    }
}

static void list_row_cb(lv_event_t *e)
{
    int index = (int)(intptr_t)lv_event_get_user_data(e);
    sched_btn_show_edit(index, false);
    ui_settings_idle_cb(NULL);
}

static void add_cb(lv_event_t *e)
{
    (void)e;
    app_config_t *draft = ui_settings_draft();
    if (draft->scheduled_button_count >= APP_SCHEDULED_BUTTON_MAX) {
        return;
    }
    sched_btn_show_edit((int)draft->scheduled_button_count, true);
    ui_settings_idle_cb(NULL);
}

static void action_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (idx < 0 || idx >= 4) {
        return;
    }
    s_edit_btn.action = s_action_values[idx];
    sched_btn_refresh_action_ui();
    ui_settings_idle_cb(NULL);
}

static void mode_cb(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (idx < 0 || idx >= 4) {
        return;
    }
    s_edit_btn.show_modes ^= s_mode_bits[idx];
    if ((s_edit_btn.show_modes & (uint8_t)APP_MODE_BIT_ALL) == 0) {
        s_edit_btn.show_modes = s_mode_bits[idx];
    }
    sched_btn_refresh_modes_ui();
    ui_settings_idle_cb(NULL);
}

static void enabled_cb(lv_event_t *e)
{
    (void)e;
    s_edit_btn.enabled = !s_edit_btn.enabled;
    sched_btn_refresh_action_ui();
    ui_settings_idle_cb(NULL);
}

static void delete_cb(lv_event_t *e)
{
    (void)e;
    app_config_t *draft = ui_settings_draft();
    if (s_sel >= 0 && s_sel < (int)draft->scheduled_button_count) {
        for (int i = s_sel; i < (int)draft->scheduled_button_count - 1; i++) {
            draft->scheduled_buttons[i] = draft->scheduled_buttons[i + 1];
        }
        if (draft->scheduled_button_count > 0) {
            draft->scheduled_button_count--;
        }
    }
    sched_btn_show_list();
    ui_settings_idle_cb(NULL);
}

static void sched_btn_refresh_action_ui(void)
{
    const ui_theme_t *t = ui_theme_get();
    for (int i = 0; i < 4; i++) {
        if (s_action_btns[i] == NULL) {
            continue;
        }
        const bool sel = s_edit_btn.action == s_action_values[i];
        lv_obj_set_style_bg_color(s_action_btns[i], sel ? t->secondary : t->menu_petal, 0);
    }
    if (s_enabled_btn != NULL) {
        lv_label_set_text(lv_obj_get_child(s_enabled_btn, 0),
                          s_edit_btn.enabled ? "Enabled" : "Disabled");
    }
}

static void sched_btn_refresh_modes_ui(void)
{
    const ui_theme_t *t = ui_theme_get();
    for (int i = 0; i < 4; i++) {
        if (s_mode_btns[i] == NULL) {
            continue;
        }
        const bool on = (s_edit_btn.show_modes & s_mode_bits[i]) != 0;
        lv_obj_set_style_bg_color(s_mode_btns[i], on ? t->secondary : t->menu_petal, 0);
    }
}

static void cancel_cb(lv_event_t *e)
{
    (void)e;
    if (s_view == SCHED_BTN_VIEW_EDIT) {
        sched_btn_show_list();
        return;
    }

    ui_settings_scheduled_buttons_restore_saved();
    ui_settings_scheduled_buttons_close();
}

static void save_cb(lv_event_t *e)
{
    (void)e;
    app_config_t *cfg = app_config_get();
    app_config_t *draft = ui_settings_draft();
    app_config_t *saved = ui_settings_saved();

    if (s_view == SCHED_BTN_VIEW_EDIT) {
        s_edit_btn.start_min = (uint16_t)(s_edit_start_sec / 60U);
        s_edit_btn.end_min = (uint16_t)(s_edit_end_sec / 60U);
        s_edit_btn.show_modes &= (uint8_t)APP_MODE_BIT_ALL;
        if (s_edit_btn.show_modes == 0) {
            s_edit_btn.show_modes = (uint8_t)APP_MODE_BIT_ALL;
        }

        if (s_adding) {
            if (draft->scheduled_button_count < APP_SCHEDULED_BUTTON_MAX) {
                draft->scheduled_buttons[draft->scheduled_button_count++] = s_edit_btn;
            }
        } else if (s_sel >= 0 && s_sel < (int)draft->scheduled_button_count) {
            draft->scheduled_buttons[s_sel] = s_edit_btn;
        }
        sched_btn_show_list();
        return;
    }

    memcpy(cfg->scheduled_buttons, draft->scheduled_buttons, sizeof(cfg->scheduled_buttons));
    cfg->scheduled_button_count = draft->scheduled_button_count;
    memcpy(saved->scheduled_buttons, cfg->scheduled_buttons, sizeof(saved->scheduled_buttons));
    saved->scheduled_button_count = cfg->scheduled_button_count;
    app_config_save_schedule();
    ui_settings_scheduled_buttons_close();
    ui_screen_tod_refresh_scheduled_button();
}

static void bind_wedges(void)
{
    if (s_cancel_wedge != NULL) {
        ui_wedge_bind(s_cancel_wedge, UI_WEDGE_CANCEL, cancel_cb, NULL);
        ui_wedge_set_visible(s_cancel_wedge, true);
    }
    if (s_save_wedge != NULL) {
        ui_wedge_bind(s_save_wedge, UI_WEDGE_CONFIRM, save_cb, NULL);
        ui_wedge_set_visible(s_save_wedge, true);
    }
    ui_settings_set_panel_wedges_visible(PANEL_SCHEDULE, false);
}

static void sched_btn_show_list(void)
{
    app_config_t *draft = ui_settings_draft();
    s_view = SCHED_BTN_VIEW_LIST;
    s_sel = -1;
    s_adding = false;
    s_ui_open = true;

    sched_btn_hide_edit_rows();
    ui_settings_schedule_hide_durations(true);

    if (s_panel_title != NULL) {
        lv_label_set_text(s_panel_title, "Scheduled buttons");
    }

    char row_text[96];
    int row = 0;
    for (int i = 0; i < (int)draft->scheduled_button_count; i++) {
        if (s_list_btns[i] == NULL) {
            continue;
        }
        format_row(i, row_text, sizeof(row_text));
        lv_label_set_text(lv_obj_get_child(s_list_btns[i], 0), row_text);
        lv_obj_set_pos(s_list_btns[i],
                       ui_layout_parent_center_x_wf(s_panel, BTN_W),
                       ui_settings_wf_y(s_panel, LIST_Y0_WF + row * (BTN_H + BTN_GAP_Y)));
        lv_obj_remove_event_cb(s_list_btns[i], list_row_cb);
        lv_obj_add_event_cb(s_list_btns[i], list_row_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        lv_obj_clear_flag(s_list_btns[i], LV_OBJ_FLAG_HIDDEN);
        row++;
    }
    for (int i = (int)draft->scheduled_button_count; i < APP_SCHEDULED_BUTTON_MAX; i++) {
        if (s_list_btns[i] != NULL) {
            lv_obj_add_flag(s_list_btns[i], LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (s_add_btn != NULL) {
        if (draft->scheduled_button_count < APP_SCHEDULED_BUTTON_MAX) {
            lv_obj_align(s_add_btn, LV_ALIGN_TOP_MID, 0,
                         ui_settings_wf_y(s_panel, LIST_Y0_WF + row * (BTN_H + BTN_GAP_Y) + 8));
            lv_obj_remove_event_cb(s_add_btn, add_cb);
            lv_obj_add_event_cb(s_add_btn, add_cb, LV_EVENT_CLICKED, NULL);
            lv_obj_clear_flag(s_add_btn, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_add_btn, LV_OBJ_FLAG_HIDDEN);
        }
    }

    bind_wedges();
    ui_settings_idle_cb(NULL);
}

static void sched_btn_show_edit(int index, bool adding)
{
    app_config_t *draft = ui_settings_draft();
    s_view = SCHED_BTN_VIEW_EDIT;
    s_sel = index;
    s_adding = adding;

    sched_btn_hide_list_rows();

    if (adding) {
        s_edit_btn = (app_scheduled_button_t){
            .start_min = 19U * 60U,
            .end_min = 21U * 60U,
            .action = APP_SCHEDULE_ACTION_START_SLEEP,
            .show_modes = (uint8_t)APP_MODE_BIT(APP_MODE_WAKE),
            .enabled = true,
        };
    } else if (index >= 0 && index < (int)draft->scheduled_button_count) {
        s_edit_btn = draft->scheduled_buttons[index];
    }

    s_edit_start_sec = (uint32_t)s_edit_btn.start_min * 60U;
    s_edit_end_sec = (uint32_t)s_edit_btn.end_min * 60U;

    if (s_panel_title != NULL) {
        lv_label_set_text(s_panel_title, adding ? "Add button" : "Edit button");
    }

    if (s_start_lbl != NULL) {
        lv_obj_clear_flag(s_start_lbl, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_end_lbl != NULL) {
        lv_obj_clear_flag(s_end_lbl, LV_OBJ_FLAG_HIDDEN);
    }
    ui_time_editor_set_visible(&s_start_bundle.editor, true);
    ui_time_editor_refresh(&s_start_bundle.editor, &s_start_bundle.cfg);
    ui_time_editor_set_visible(&s_end_bundle.editor, true);
    ui_time_editor_refresh(&s_end_bundle.editor, &s_end_bundle.cfg);

    if (s_edit_action_lbl != NULL) {
        lv_obj_clear_flag(s_edit_action_lbl, LV_OBJ_FLAG_HIDDEN);
    }
    for (int i = 0; i < 4; i++) {
        if (s_action_btns[i] == NULL) {
            continue;
        }
        const int col = i % 2;
        const int row = i / 2;
        const int x_off = col == 0 ? -60 : 60;
        lv_obj_align(s_action_btns[i], LV_ALIGN_TOP_MID, x_off,
                     ui_settings_wf_y(s_panel, EDIT_ACTION_Y0 + row * 40));
        lv_obj_remove_event_cb(s_action_btns[i], action_cb);
        lv_obj_add_event_cb(s_action_btns[i], action_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        lv_obj_clear_flag(s_action_btns[i], LV_OBJ_FLAG_HIDDEN);
    }

    if (s_edit_modes_lbl != NULL) {
        lv_obj_clear_flag(s_edit_modes_lbl, LV_OBJ_FLAG_HIDDEN);
    }
    for (int i = 0; i < 4; i++) {
        if (s_mode_btns[i] == NULL) {
            continue;
        }
        const int col = i % 2;
        const int row = i / 2;
        const int x_off = col == 0 ? -60 : 60;
        lv_obj_align(s_mode_btns[i], LV_ALIGN_TOP_MID, x_off,
                     ui_settings_wf_y(s_panel, EDIT_MODES_Y0 + row * 40));
        lv_obj_remove_event_cb(s_mode_btns[i], mode_cb);
        lv_obj_add_event_cb(s_mode_btns[i], mode_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        lv_obj_clear_flag(s_mode_btns[i], LV_OBJ_FLAG_HIDDEN);
    }

    if (s_enabled_btn != NULL) {
        lv_obj_align(s_enabled_btn, LV_ALIGN_TOP_MID, 0, ui_settings_wf_y(s_panel, EDIT_ENABLED_Y));
        lv_obj_remove_event_cb(s_enabled_btn, enabled_cb);
        lv_obj_add_event_cb(s_enabled_btn, enabled_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_clear_flag(s_enabled_btn, LV_OBJ_FLAG_HIDDEN);
    }

    if (s_delete_btn != NULL && !adding) {
        lv_obj_align(s_delete_btn, LV_ALIGN_TOP_MID, 0, ui_settings_wf_y(s_panel, EDIT_DELETE_Y));
        lv_obj_remove_event_cb(s_delete_btn, delete_cb);
        lv_obj_add_event_cb(s_delete_btn, delete_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_clear_flag(s_delete_btn, LV_OBJ_FLAG_HIDDEN);
    } else if (s_delete_btn != NULL) {
        lv_obj_add_flag(s_delete_btn, LV_OBJ_FLAG_HIDDEN);
    }

    sched_btn_refresh_action_ui();
    sched_btn_refresh_modes_ui();
    bind_wedges();
    ui_settings_idle_cb(NULL);
}

void ui_settings_scheduled_buttons_open(void)
{
    sched_btn_show_list();
}

void ui_settings_scheduled_buttons_close(void)
{
    s_view = SCHED_BTN_VIEW_LIST;
    s_ui_open = false;
    sched_btn_hide_list_rows();
    sched_btn_hide_edit_rows();
    ui_settings_schedule_hide_durations(false);
    if (s_cancel_wedge != NULL) {
        ui_wedge_set_visible(s_cancel_wedge, false);
    }
    if (s_save_wedge != NULL) {
        ui_wedge_set_visible(s_save_wedge, false);
    }
    ui_settings_set_panel_wedges_visible(PANEL_SCHEDULE, true);
    if (s_panel_title != NULL) {
        lv_label_set_text(s_panel_title, "Schedule");
    }
}

bool ui_settings_scheduled_buttons_try_cancel(void)
{
    if (!s_ui_open) {
        return false;
    }
    cancel_cb(NULL);
    return true;
}

void ui_settings_scheduled_buttons_restore_saved(void)
{
    app_config_t *draft = ui_settings_draft();
    app_config_t *saved = ui_settings_saved();
    app_config_scheduled_buttons_copy(draft->scheduled_buttons, &draft->scheduled_button_count,
                                      saved->scheduled_buttons, saved->scheduled_button_count);
}
