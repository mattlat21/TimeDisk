/**
 * @file ui_settings_adult_auth.c
 * @brief Settings -> Adult Authentication sub-panel.
 */

#include "ui_screen_settings_internal.h"

#include "ui_layout.h"
#include "ui_theme.h"
#include "ui_widgets.h"
#include "ui_numeric_keypad.h"

#include <stdio.h>
#include <string.h>

#define AA_CHK_W             140
#define AA_TITLE_Y_WF        64
#define AA_CHK_PIN_Y_WF      90
#define AA_CHK_MATHS_Y_WF    130
#define AA_KEYPAD_BTN        76
#define AA_KEYPAD_GAP        8
#define AA_KEYPAD_COLS       3
#define AA_KEYPAD_GRID_W     (AA_KEYPAD_COLS * AA_KEYPAD_BTN + (AA_KEYPAD_COLS - 1) * AA_KEYPAD_GAP)
#define AA_PIN_BAR_W         AA_KEYPAD_GRID_W
#define AA_PIN_BAR_H         72
#define AA_PIN_BAR_Y_WF      160
#define AA_KEYPAD_Y_WF       252
#define AA_RIGHT_COL_GAP     22

static lv_obj_t *s_panel;
static lv_obj_t *s_chk_pin;
static lv_obj_t *s_chk_maths;
static lv_obj_t *s_lbl_aa_pin;
static char s_aa_pin_buf[APP_AA_PIN_LEN];
static ui_numeric_keypad_t *s_keypad;

static void aa_refresh_pin_label(void)
{
    lv_label_set_text(s_lbl_aa_pin, s_aa_pin_buf);
}

static void aa_pin_digit_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    const char *digit = btn ? (const char *)lv_obj_get_user_data(btn) : NULL;
    if (digit == NULL) {
        return;
    }
    size_t len = strlen(s_aa_pin_buf);
    if (len >= 4) {
        return;
    }
    s_aa_pin_buf[len] = digit[0];
    s_aa_pin_buf[len + 1] = '\0';
    aa_refresh_pin_label();
    ui_settings_idle_cb(NULL);
}

static void aa_pin_back_cb(lv_event_t *e)
{
    (void)e;
    size_t len = strlen(s_aa_pin_buf);
    if (len > 0) {
        s_aa_pin_buf[len - 1] = '\0';
        aa_refresh_pin_label();
    }
    ui_settings_idle_cb(NULL);
}

static void aa_method_changed_cb(lv_event_t *e)
{
    (void)e;
    uint8_t methods = 0;
    if (lv_obj_has_state(s_chk_pin, LV_STATE_CHECKED)) {
        methods |= AA_METHOD_PIN;
    }
    if (lv_obj_has_state(s_chk_maths, LV_STATE_CHECKED)) {
        methods |= AA_METHOD_MATHS;
    }
    ui_settings_draft()->aa_methods = methods;
    ui_settings_idle_cb(NULL);
}

void ui_settings_adult_auth_sync_from_draft(void)
{
    app_config_t *draft = ui_settings_draft();
    snprintf(s_aa_pin_buf, sizeof(s_aa_pin_buf), "%s", draft->aa_pin);
    aa_refresh_pin_label();
    if (s_keypad != NULL) {
        ui_numeric_keypad_set_visible(s_keypad, true);
    }

    if (draft->aa_methods & AA_METHOD_PIN) {
        lv_obj_add_state(s_chk_pin, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(s_chk_pin, LV_STATE_CHECKED);
    }
    if (draft->aa_methods & AA_METHOD_MATHS) {
        lv_obj_add_state(s_chk_maths, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(s_chk_maths, LV_STATE_CHECKED);
    }
}

static void aa_save_cb(lv_event_t *e)
{
    (void)e;
    app_config_t *cfg = app_config_get();
    app_config_t *draft = ui_settings_draft();
    app_config_t *saved = ui_settings_saved();

    if (strlen(s_aa_pin_buf) == 4) {
        snprintf(draft->aa_pin, sizeof(draft->aa_pin), "%s", s_aa_pin_buf);
    }
    cfg->aa_methods = draft->aa_methods;
    snprintf(cfg->aa_pin, sizeof(cfg->aa_pin), "%s", draft->aa_pin);
    saved->aa_methods = draft->aa_methods;
    memcpy(saved->aa_pin, draft->aa_pin, sizeof(saved->aa_pin));

    app_config_save_aa();
    ui_settings_show_panel(PANEL_HUB);
}

lv_obj_t *ui_settings_adult_auth_build(void)
{
    const ui_theme_t *t = ui_theme_get();

    s_panel = ui_settings_create_sub_panel("Adult Authentication");
    lv_obj_align(lv_obj_get_child(s_panel, 0), LV_ALIGN_TOP_MID, 0,
                 ui_settings_wf_y(s_panel, AA_TITLE_Y_WF));

    const int keypad_x = ui_layout_parent_center_x_wf(s_panel, AA_KEYPAD_GRID_W);
    const int keypad_y = ui_settings_wf_y(s_panel, AA_KEYPAD_Y_WF);
    const int aa_chk_x = keypad_x + AA_KEYPAD_GRID_W + AA_RIGHT_COL_GAP;

    s_chk_pin = lv_checkbox_create(s_panel);
    lv_checkbox_set_text(s_chk_pin, "PIN");
    lv_obj_set_pos(s_chk_pin, aa_chk_x, keypad_y);
    lv_obj_set_style_text_color(s_chk_pin, t->white, 0);
    lv_obj_set_style_text_font(s_chk_pin, &lv_font_montserrat_26, 0);
    lv_obj_set_style_pad_column(s_chk_pin, 16, 0);
    lv_obj_set_style_width(s_chk_pin, 36, LV_PART_INDICATOR);
    lv_obj_set_style_height(s_chk_pin, 36, LV_PART_INDICATOR);
    lv_obj_add_event_cb(s_chk_pin, aa_method_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);

    s_chk_maths = lv_checkbox_create(s_panel);
    lv_checkbox_set_text(s_chk_maths, "Maths");
    lv_obj_set_pos(s_chk_maths, aa_chk_x, keypad_y + (AA_KEYPAD_BTN + AA_KEYPAD_GAP));
    lv_obj_set_style_text_color(s_chk_maths, t->white, 0);
    lv_obj_set_style_text_font(s_chk_maths, &lv_font_montserrat_26, 0);
    lv_obj_set_style_pad_column(s_chk_maths, 16, 0);
    lv_obj_set_style_width(s_chk_maths, 36, LV_PART_INDICATOR);
    lv_obj_set_style_height(s_chk_maths, 36, LV_PART_INDICATOR);
    lv_obj_add_event_cb(s_chk_maths, aa_method_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *pin_bar = ui_widgets_create_purple_box(s_panel, AA_PIN_BAR_W, AA_PIN_BAR_H,
                                                     ui_layout_parent_center_x_wf(s_panel, AA_PIN_BAR_W),
                                                     ui_settings_wf_y(s_panel, AA_PIN_BAR_Y_WF), false);
    s_lbl_aa_pin = lv_label_create(pin_bar);
    lv_label_set_text(s_lbl_aa_pin, "");
    lv_obj_set_style_text_color(s_lbl_aa_pin, t->white, 0);
    lv_obj_set_style_text_font(s_lbl_aa_pin, &lv_font_montserrat_48, 0);
    lv_obj_center(s_lbl_aa_pin);

    (void)keypad_x;
    (void)keypad_y;
    s_keypad = ui_numeric_keypad_create_overlay(ui_settings_screen(), &(ui_numeric_keypad_cfg_t){
        .digit_cb = aa_pin_digit_cb,
        .backspace_cb = aa_pin_back_cb,
        .user_ctx = NULL,
    });
    if (s_keypad != NULL) {
        ui_numeric_keypad_set_visible(s_keypad, false);
        ui_settings_register_overlay_obj(ui_numeric_keypad_get_container(s_keypad));
    }

    ui_settings_attach_panel_wedges(s_panel, PANEL_AA, aa_save_cb);
    return s_panel;
}

