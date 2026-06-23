/**
 * @file ui_settings_display.c
 * @brief Settings -> Display sub-panel (backlight bright/dim levels).
 */

#include "ui_screen_settings_internal.h"

#include "ui_duration_editor.h"
#include "ui_theme.h"
#include "ui_widgets.h"

#include <stdio.h>
#include <string.h>

void ui_settings_display_show_list(void);

#define HUB_BTN_W           248

#define DISPLAY_LIST_Y0_WF   180
#define DISPLAY_LIST_STEP_WF 76
#define DISPLAY_EDIT_TITLE_Y_WF 72

typedef enum {
    DISPLAY_VIEW_LIST = 0,
    DISPLAY_VIEW_EDIT,
} display_view_t;

static lv_obj_t *s_panel;
static display_view_t s_display_view;
static int s_display_edit_idx;
static lv_obj_t *s_display_row_btns[2];
static lv_obj_t *s_display_panel_title;
static lv_obj_t *s_display_edit_title;
static lv_obj_t *s_display_row_lbls[2];
static ui_duration_editor_bundle_t s_display_bundle;
static uint32_t s_display_edit_val;

static const char *s_display_names[2] = {
    "Bright level",
    "Dim level",
};

static uint8_t *display_field_ptr(int idx)
{
    app_config_t *draft = ui_settings_draft();
    switch (idx) {
    case 0:
        return &draft->backlight_bright_pct;
    case 1:
        return &draft->backlight_dim_pct;
    default:
        return &draft->backlight_bright_pct;
    }
}

static void display_refresh_list_labels(void)
{
    char line[64];
    for (int i = 0; i < 2; i++) {
        snprintf(line, sizeof(line), "%s: %u%%", s_display_names[i], (unsigned)*display_field_ptr(i));
        lv_label_set_text(s_display_row_lbls[i], line);
    }
}

static void display_commit_edit(void)
{
    if (s_display_view != DISPLAY_VIEW_EDIT) {
        return;
    }
    uint8_t val = (uint8_t)s_display_edit_val;
    if (val > 100) {
        val = 100;
    }
    *display_field_ptr(s_display_edit_idx) = val;
}

bool ui_settings_display_on_cancel(void)
{
    if (s_display_view != DISPLAY_VIEW_EDIT) {
        return false;
    }
    display_commit_edit();
    ui_settings_display_show_list();
    return true;
}

void ui_settings_display_show_list(void)
{
    s_display_view = DISPLAY_VIEW_LIST;
    for (int i = 0; i < 2; i++) {
        if (s_display_row_btns[i] != NULL) {
            lv_obj_clear_flag(s_display_row_btns[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    ui_duration_editor_set_visible(&s_display_bundle.editor, false);
    if (s_display_panel_title != NULL) {
        lv_obj_clear_flag(s_display_panel_title, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_display_edit_title != NULL) {
        lv_obj_add_flag(s_display_edit_title, LV_OBJ_FLAG_HIDDEN);
    }
    display_refresh_list_labels();
}

static void display_row_cb(lv_event_t *e)
{
    int idx = (int)(uintptr_t)lv_event_get_user_data(e);
    s_display_edit_idx = idx;
    s_display_edit_val = *display_field_ptr(idx);
    s_display_view = DISPLAY_VIEW_EDIT;

    for (int i = 0; i < 2; i++) {
        if (s_display_row_btns[i] != NULL) {
            lv_obj_add_flag(s_display_row_btns[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (s_display_panel_title != NULL) {
        lv_obj_add_flag(s_display_panel_title, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_display_edit_title != NULL) {
        lv_label_set_text(s_display_edit_title, s_display_names[idx]);
        lv_obj_clear_flag(s_display_edit_title, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_display_edit_title);
    }
    ui_duration_editor_refresh(&s_display_bundle.editor, &s_display_bundle.cfg);
    ui_duration_editor_set_visible(&s_display_bundle.editor, true);
    ui_settings_idle_cb(NULL);
}

static void display_save_cb(lv_event_t *e)
{
    (void)e;
    app_config_t *cfg = app_config_get();
    app_config_t *draft = ui_settings_draft();
    app_config_t *saved = ui_settings_saved();

    display_commit_edit();
    if (draft->backlight_bright_pct > 100) {
        draft->backlight_bright_pct = 100;
    }
    if (draft->backlight_dim_pct > 100) {
        draft->backlight_dim_pct = 100;
    }
    memcpy(cfg, draft, sizeof(*cfg));
    saved->backlight_bright_pct = draft->backlight_bright_pct;
    saved->backlight_dim_pct = draft->backlight_dim_pct;

    app_config_save_display();
    ui_settings_display_show_list();
    ui_settings_show_panel(PANEL_HUB);
}

lv_obj_t *ui_settings_display_build(void)
{
    const ui_theme_t *t = ui_theme_get();

    s_panel = lv_obj_create(ui_settings_screen());
    lv_obj_set_style_bg_opa(s_panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_panel, 0, 0);
    lv_obj_remove_flag(s_panel, LV_OBJ_FLAG_SCROLLABLE);
    ui_settings_panel_layout(s_panel);
    lv_obj_add_flag(s_panel, LV_OBJ_FLAG_HIDDEN);

    s_display_panel_title = ui_widgets_create_title(s_panel, "Display");

    for (int i = 0; i < 2; i++) {
        lv_obj_t *btn = lv_button_create(s_panel);
        lv_obj_set_size(btn, HUB_BTN_W, 64);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0,
                     ui_settings_wf_y(s_panel,
                                      DISPLAY_LIST_Y0_WF + i * DISPLAY_LIST_STEP_WF));
        lv_obj_set_style_radius(btn, 16, 0);
        lv_obj_set_style_bg_color(btn, t->menu_petal, 0);
        lv_obj_set_style_border_width(btn, 0, 0);

        s_display_row_lbls[i] = lv_label_create(btn);
        lv_label_set_text(s_display_row_lbls[i], s_display_names[i]);
        lv_obj_set_style_text_color(s_display_row_lbls[i], t->white, 0);
        lv_obj_set_style_text_font(s_display_row_lbls[i], &lv_font_montserrat_20, 0);
        lv_obj_center(s_display_row_lbls[i]);
        lv_obj_add_event_cb(btn, display_row_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
        s_display_row_btns[i] = btn;
    }

    s_display_edit_title = ui_widgets_create_title(s_panel, "Bright level");
    lv_obj_align(s_display_edit_title, LV_ALIGN_TOP_MID, 0,
                 ui_settings_wf_y(s_panel, DISPLAY_EDIT_TITLE_Y_WF));
    lv_obj_add_flag(s_display_edit_title, LV_OBJ_FLAG_HIDDEN);

    s_display_bundle.cfg = (ui_duration_editor_cfg_t){
        .value_sec = &s_display_edit_val,
        .box_y = UI_DURATION_EDITOR_BOX_Y,
        .show_end_time = false,
        .min_sec = 0,
        .max_sec = 100,
        .step_sec = 5,
        .display = UI_DURATION_DISPLAY_PERCENT,
        .on_change = ui_settings_idle_cb,
    };
    ui_duration_editor_create(s_panel, &s_display_bundle);
    ui_duration_editor_set_visible(&s_display_bundle.editor, false);

    ui_settings_attach_panel_wedges(s_panel, PANEL_DISPLAY, display_save_cb);
    s_display_view = DISPLAY_VIEW_LIST;
    return s_panel;
}
