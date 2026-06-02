/**
 * @file ui_settings_timeouts.c
 * @brief Settings -> Timeouts sub-panel.
 */

#include "ui_screen_settings_internal.h"

#include "ui_duration_editor.h"
#include "ui_format.h"
#include "ui_theme.h"
#include "ui_widgets.h"

#include <stdio.h>
#include <string.h>

void ui_settings_timeouts_show_list(void);

#define HUB_BTN_W           248

#define TIMEOUT_LIST_Y0_WF   100
#define TIMEOUT_LIST_STEP_WF 76
#define TIMEOUT_EDIT_TITLE_Y_WF 72

typedef enum {
    TIMEOUT_VIEW_LIST = 0,
    TIMEOUT_VIEW_EDIT,
} timeout_view_t;

static lv_obj_t *s_panel;
static timeout_view_t s_timeout_view;
static int s_timeout_edit_idx;
static lv_obj_t *s_timeout_row_btns[5];
static lv_obj_t *s_timeout_panel_title;
static lv_obj_t *s_timeout_edit_title;
static lv_obj_t *s_timeout_row_lbls[5];
static ui_duration_editor_bundle_t s_timeout_bundle;
static uint32_t s_timeout_edit_val;

static const char *s_timeout_names[5] = {
    "Splash",
    "TOD dim",
    "Adult auth",
    "Main menu",
    "Timer dim",
};

static uint32_t *timeout_field_ptr(int idx)
{
    app_config_t *draft = ui_settings_draft();
    switch (idx) {
    case 0:
        return &draft->timeout_splash_sec;
    case 1:
        return &draft->timeout_tod_dim_sec;
    case 2:
        return &draft->timeout_aa_sec;
    case 3:
        return &draft->timeout_main_menu_sec;
    case 4:
        return &draft->timeout_timer_dim_sec;
    default:
        return &draft->timeout_splash_sec;
    }
}

static void timeout_refresh_list_labels(void)
{
    char buf[48];
    for (int i = 0; i < 5; i++) {
        ui_format_duration_minutes(buf, sizeof(buf), *timeout_field_ptr(i));
        char line[64];
        snprintf(line, sizeof(line), "%s: %s", s_timeout_names[i], buf);
        lv_label_set_text(s_timeout_row_lbls[i], line);
    }
}

static void timeout_commit_edit(void)
{
    if (s_timeout_view != TIMEOUT_VIEW_EDIT) {
        return;
    }
    *timeout_field_ptr(s_timeout_edit_idx) = s_timeout_edit_val;
}

bool ui_settings_timeouts_on_cancel(void)
{
    if (s_timeout_view != TIMEOUT_VIEW_EDIT) {
        return false;
    }
    timeout_commit_edit();
    ui_settings_timeouts_show_list();
    return true;
}

void ui_settings_timeouts_show_list(void)
{
    s_timeout_view = TIMEOUT_VIEW_LIST;
    for (int i = 0; i < 5; i++) {
        if (s_timeout_row_btns[i] != NULL) {
            lv_obj_clear_flag(s_timeout_row_btns[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    ui_duration_editor_set_visible(&s_timeout_bundle.editor, false);
    if (s_timeout_panel_title != NULL) {
        lv_obj_clear_flag(s_timeout_panel_title, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_timeout_edit_title != NULL) {
        lv_obj_add_flag(s_timeout_edit_title, LV_OBJ_FLAG_HIDDEN);
    }
    timeout_refresh_list_labels();
}

static void timeout_row_cb(lv_event_t *e)
{
    int idx = (int)(uintptr_t)lv_event_get_user_data(e);
    s_timeout_edit_idx = idx;
    s_timeout_edit_val = *timeout_field_ptr(idx);
    s_timeout_view = TIMEOUT_VIEW_EDIT;

    for (int i = 0; i < 5; i++) {
        if (s_timeout_row_btns[i] != NULL) {
            lv_obj_add_flag(s_timeout_row_btns[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (s_timeout_panel_title != NULL) {
        lv_obj_add_flag(s_timeout_panel_title, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_timeout_edit_title != NULL) {
        lv_label_set_text(s_timeout_edit_title, s_timeout_names[idx]);
        lv_obj_clear_flag(s_timeout_edit_title, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_timeout_edit_title);
    }
    ui_duration_editor_refresh(&s_timeout_bundle.editor, &s_timeout_bundle.cfg);
    ui_duration_editor_set_visible(&s_timeout_bundle.editor, true);
    ui_settings_idle_cb(NULL);
}

static void timeouts_save_cb(lv_event_t *e)
{
    (void)e;
    app_config_t *cfg = app_config_get();
    app_config_t *draft = ui_settings_draft();
    app_config_t *saved = ui_settings_saved();

    timeout_commit_edit();
    memcpy(cfg, draft, sizeof(*cfg));
    saved->timeout_splash_sec = draft->timeout_splash_sec;
    saved->timeout_tod_dim_sec = draft->timeout_tod_dim_sec;
    saved->timeout_aa_sec = draft->timeout_aa_sec;
    saved->timeout_main_menu_sec = draft->timeout_main_menu_sec;
    saved->timeout_timer_dim_sec = draft->timeout_timer_dim_sec;

    app_config_save_timeouts();
    ui_settings_timeouts_show_list();
    ui_settings_show_panel(PANEL_HUB);
}

lv_obj_t *ui_settings_timeouts_build(void)
{
    const ui_theme_t *t = ui_theme_get();

    s_panel = lv_obj_create(ui_settings_screen());
    lv_obj_set_style_bg_opa(s_panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_panel, 0, 0);
    lv_obj_remove_flag(s_panel, LV_OBJ_FLAG_SCROLLABLE);
    ui_settings_panel_layout(s_panel);
    lv_obj_add_flag(s_panel, LV_OBJ_FLAG_HIDDEN);

    s_timeout_panel_title = ui_widgets_create_title(s_panel, "Timeouts");

    for (int i = 0; i < 5; i++) {
        lv_obj_t *btn = lv_button_create(s_panel);
        lv_obj_set_size(btn, HUB_BTN_W, 64);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0,
                     ui_settings_wf_y(s_panel,
                                      TIMEOUT_LIST_Y0_WF + i * TIMEOUT_LIST_STEP_WF));
        lv_obj_set_style_radius(btn, 16, 0);
        lv_obj_set_style_bg_color(btn, t->menu_petal, 0);
        lv_obj_set_style_border_width(btn, 0, 0);

        s_timeout_row_lbls[i] = lv_label_create(btn);
        lv_label_set_text(s_timeout_row_lbls[i], s_timeout_names[i]);
        lv_obj_set_style_text_color(s_timeout_row_lbls[i], t->white, 0);
        lv_obj_set_style_text_font(s_timeout_row_lbls[i], &lv_font_montserrat_20, 0);
        lv_obj_center(s_timeout_row_lbls[i]);
        lv_obj_add_event_cb(btn, timeout_row_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
        s_timeout_row_btns[i] = btn;
    }

    s_timeout_edit_title = ui_widgets_create_title(s_panel, "Splash");
    lv_obj_align(s_timeout_edit_title, LV_ALIGN_TOP_MID, 0,
                 ui_settings_wf_y(s_panel, TIMEOUT_EDIT_TITLE_Y_WF));
    lv_obj_add_flag(s_timeout_edit_title, LV_OBJ_FLAG_HIDDEN);

    s_timeout_bundle.cfg = (ui_duration_editor_cfg_t){
        .value_sec = &s_timeout_edit_val,
        .box_y = UI_DURATION_EDITOR_BOX_Y,
        .show_end_time = false,
        .on_change = ui_settings_idle_cb,
    };
    ui_duration_editor_create(s_panel, &s_timeout_bundle);
    ui_duration_editor_set_visible(&s_timeout_bundle.editor, false);

    ui_settings_attach_panel_wedges(s_panel, PANEL_TIMEOUTS, timeouts_save_cb);
    s_timeout_view = TIMEOUT_VIEW_LIST;
    return s_panel;
}

