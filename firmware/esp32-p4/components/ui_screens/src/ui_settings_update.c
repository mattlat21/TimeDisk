/**
 * @file ui_settings_update.c
 * @brief Settings -> Update sub-panel (OTA firmware URL + progress).
 */

#include "ui_screen_settings_internal.h"

#include "app_ota.h"
#include "app_network.h"
#include <esp_err.h>
#include "ui_keyboard.h"
#include "ui_theme.h"
#include "ui_widgets.h"
#include "ui_wedge.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UPDATE_VERSION_Y_WF   100
#define UPDATE_URL_LABEL_Y_WF 155
#define UPDATE_URL_FIELD_Y_WF 195
#define UPDATE_URL_FIELD_W    563
#define UPDATE_URL_FIELD_H    78
#define UPDATE_BTN_Y_WF       300
#define UPDATE_BTN_W          248
#define UPDATE_BTN_H          52
#define UPDATE_BAR_Y_WF       380
#define UPDATE_BAR_W          480
#define UPDATE_BAR_H          24
#define UPDATE_STATUS_Y_WF    430

typedef struct {
    int percent;
    char status[64];
} update_progress_msg_t;

typedef struct {
    esp_err_t err;
    char message[96];
} update_done_msg_t;

static lv_obj_t *s_panel;
static lv_obj_t *s_version_lbl;
static lv_obj_t *s_url_box;
static lv_obj_t *s_url_lbl;
static lv_obj_t *s_update_btn;
static lv_obj_t *s_progress_bar;
static lv_obj_t *s_status_lbl;
static ui_wedge_t *s_cancel_wedge;
static ui_keyboard_t *s_kb;
static char s_url_buf[APP_UPDATE_URL_MAX];

static void update_set_controls_enabled(bool enabled);
static void update_show_panel(void);

static void update_progress_async_cb(void *arg)
{
    update_progress_msg_t *msg = (update_progress_msg_t *)arg;
    if (msg == NULL) {
        return;
    }

    if (s_progress_bar != NULL) {
        lv_bar_set_value(s_progress_bar, msg->percent, LV_ANIM_OFF);
        lv_obj_clear_flag(s_progress_bar, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_status_lbl != NULL) {
        lv_label_set_text(s_status_lbl, msg->status);
    }

    free(msg);
}

static void update_done_async_cb(void *arg)
{
    update_done_msg_t *msg = (update_done_msg_t *)arg;
    if (msg == NULL) {
        return;
    }

    update_set_controls_enabled(true);

    if (s_status_lbl != NULL) {
        lv_label_set_text(s_status_lbl, msg->message);
    }
    if (msg->err != ESP_OK && s_progress_bar != NULL) {
        lv_bar_set_value(s_progress_bar, 0, LV_ANIM_OFF);
    }

    free(msg);
}

static void update_progress_cb(int percent, const char *status, void *user_data)
{
    (void)user_data;
    update_progress_msg_t *msg = calloc(1, sizeof(*msg));
    if (msg == NULL) {
        return;
    }
    msg->percent = percent;
    snprintf(msg->status, sizeof(msg->status), "%s", status != NULL ? status : "");
    lv_async_call(update_progress_async_cb, msg);
}

static void update_done_cb(esp_err_t err, const char *message, void *user_data)
{
    (void)user_data;
    update_done_msg_t *msg = calloc(1, sizeof(*msg));
    if (msg == NULL) {
        return;
    }
    msg->err = err;
    snprintf(msg->message, sizeof(msg->message), "%s", message != NULL ? message : "");
    lv_async_call(update_done_async_cb, msg);
}

static void update_set_controls_enabled(bool enabled)
{
    if (s_url_box != NULL) {
        if (enabled) {
            lv_obj_clear_state(s_url_box, LV_STATE_DISABLED);
        } else {
            lv_obj_add_state(s_url_box, LV_STATE_DISABLED);
        }
    }
    if (s_update_btn != NULL) {
        if (enabled) {
            lv_obj_clear_state(s_update_btn, LV_STATE_DISABLED);
        } else {
            lv_obj_add_state(s_update_btn, LV_STATE_DISABLED);
        }
    }
}

static void update_bind_keyboard(void)
{
    ui_keyboard_config_t kb_cfg = {
        .buf = s_url_buf,
        .buf_len = sizeof(s_url_buf),
        .label = s_url_lbl,
        .initial_mode = UI_KEYBOARD_MODE_LOWER,
        .on_activity = ui_settings_idle_cb,
        .user_data = NULL,
    };

    if (s_kb == NULL) {
        s_kb = ui_keyboard_create_overlay(ui_settings_screen(), &kb_cfg);
        if (s_kb != NULL) {
            for (int m = 0; m < UI_KEYBOARD_MODE_COUNT; m++) {
                ui_settings_register_overlay_obj(ui_keyboard_get_layer(s_kb, (ui_keyboard_mode_t)m));
            }
            ui_settings_register_overlay_obj(ui_keyboard_get_mode_button(s_kb));
        }
    } else {
        ui_keyboard_bind(s_kb, &kb_cfg);
    }
    ui_keyboard_set_visible(s_kb, true);
}

static void update_url_box_cb(lv_event_t *e)
{
    (void)e;
    if (app_update_active()) {
        return;
    }
    update_bind_keyboard();
    ui_settings_idle_cb(NULL);
}

static void update_cancel_cb(lv_event_t *e)
{
    (void)e;
    if (app_update_active()) {
        return;
    }
    if (s_kb != NULL) {
        ui_keyboard_set_visible(s_kb, false);
    }
    ui_settings_show_panel(PANEL_HUB);
}

static void update_btn_cb(lv_event_t *e)
{
    (void)e;
    if (app_update_active()) {
        return;
    }

    if (s_kb != NULL) {
        ui_keyboard_set_visible(s_kb, false);
    }

    if (app_network_get_state() != APP_NETWORK_STATE_READY
        && app_network_get_state() != APP_NETWORK_STATE_GOT_IP) {
        if (s_status_lbl != NULL) {
            lv_label_set_text(s_status_lbl, "Connect to Wi-Fi first");
        }
        return;
    }

    if (s_url_buf[0] == '\0') {
        if (s_status_lbl != NULL) {
            lv_label_set_text(s_status_lbl, "Enter a firmware URL");
        }
        return;
    }

    update_set_controls_enabled(false);
    if (s_progress_bar != NULL) {
        lv_bar_set_value(s_progress_bar, 0, LV_ANIM_OFF);
        lv_obj_clear_flag(s_progress_bar, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_status_lbl != NULL) {
        lv_label_set_text(s_status_lbl, "Starting update...");
    }

    esp_err_t err = app_update_start(s_url_buf, update_progress_cb, update_done_cb, NULL);
    if (err != ESP_OK) {
        update_set_controls_enabled(true);
        if (s_status_lbl != NULL) {
            lv_label_set_text(s_status_lbl, "Could not start update");
        }
    }

    ui_settings_idle_cb(NULL);
}

static void update_show_panel(void)
{
    char version_text[64];
    snprintf(version_text, sizeof(version_text), "Version %s", app_update_get_version());
    if (s_version_lbl != NULL) {
        lv_label_set_text(s_version_lbl, version_text);
    }
    if (s_url_lbl != NULL) {
        lv_label_set_text(s_url_lbl, s_url_buf);
    }
    if (s_progress_bar != NULL && !app_update_active()) {
        lv_bar_set_value(s_progress_bar, 0, LV_ANIM_OFF);
        lv_obj_add_flag(s_progress_bar, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_status_lbl != NULL && !app_update_active()) {
        lv_label_set_text(s_status_lbl, "Tap URL to edit, then Update");
    }

    update_set_controls_enabled(!app_update_active());

    if (s_cancel_wedge != NULL) {
        ui_wedge_bind(s_cancel_wedge, UI_WEDGE_CANCEL, update_cancel_cb, NULL);
        ui_wedge_set_visible(s_cancel_wedge, true);
    }
    if (s_kb != NULL) {
        ui_keyboard_set_visible(s_kb, false);
    }
}

void ui_settings_update_sync_from_draft(void)
{
    (void)0;
    update_show_panel();
}

void ui_settings_update_apply_theme(void)
{
    const ui_theme_t *t = ui_theme_get();
    if (s_url_box != NULL) {
        lv_obj_set_style_bg_color(s_url_box, t->ring, 0);
    }
    if (s_update_btn != NULL) {
        lv_obj_set_style_bg_color(s_update_btn, t->menu_petal, 0);
    }
    if (s_progress_bar != NULL) {
        lv_obj_set_style_bg_color(s_progress_bar, t->panel, 0);
        lv_obj_set_style_bg_color(s_progress_bar, t->ring, LV_PART_INDICATOR);
    }
    if (s_cancel_wedge != NULL) {
        ui_wedge_refresh_theme(s_cancel_wedge);
    }
}

lv_obj_t *ui_settings_update_build(void)
{
    const ui_theme_t *t = ui_theme_get();

    s_panel = ui_settings_create_sub_panel("Update");

    snprintf(s_url_buf, sizeof(s_url_buf), "%s", APP_UPDATE_DEFAULT_FIRMWARE_URL);

    s_version_lbl = lv_label_create(s_panel);
    lv_label_set_text(s_version_lbl, "");
    lv_obj_set_style_text_color(s_version_lbl, t->secondary, 0);
    lv_obj_set_style_text_font(s_version_lbl, &lv_font_montserrat_20, 0);
    lv_obj_align(s_version_lbl, LV_ALIGN_TOP_MID, 0, ui_settings_wf_y(s_panel, UPDATE_VERSION_Y_WF));

    lv_obj_t *url_title = lv_label_create(s_panel);
    lv_label_set_text(url_title, "Firmware URL");
    lv_obj_set_style_text_color(url_title, t->white, 0);
    lv_obj_set_style_text_font(url_title, &lv_font_montserrat_20, 0);
    lv_obj_align(url_title, LV_ALIGN_TOP_MID, 0, ui_settings_wf_y(s_panel, UPDATE_URL_LABEL_Y_WF));

    int field_x = ui_settings_wf_x(s_panel, (720 - UPDATE_URL_FIELD_W) / 2);
    s_url_box = ui_widgets_create_purple_box(s_panel, UPDATE_URL_FIELD_W, UPDATE_URL_FIELD_H,
                                             field_x, ui_settings_wf_y(s_panel, UPDATE_URL_FIELD_Y_WF),
                                             false);
    lv_obj_add_flag(s_url_box, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_url_box, update_url_box_cb, LV_EVENT_CLICKED, NULL);

    s_url_lbl = lv_label_create(s_url_box);
    lv_label_set_text(s_url_lbl, s_url_buf);
    lv_obj_set_style_text_color(s_url_lbl, t->white, 0);
    lv_obj_set_style_text_font(s_url_lbl, &lv_font_montserrat_18, 0);
    lv_label_set_long_mode(s_url_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_width(s_url_lbl, UPDATE_URL_FIELD_W - 32);
    lv_obj_center(s_url_lbl);

    s_update_btn = lv_button_create(s_panel);
    lv_obj_set_size(s_update_btn, UPDATE_BTN_W, UPDATE_BTN_H);
    lv_obj_align(s_update_btn, LV_ALIGN_TOP_MID, 0, ui_settings_wf_y(s_panel, UPDATE_BTN_Y_WF));
    lv_obj_set_style_radius(s_update_btn, 16, 0);
    lv_obj_set_style_bg_color(s_update_btn, t->menu_petal, 0);
    lv_obj_set_style_border_width(s_update_btn, 0, 0);
    lv_obj_set_style_shadow_width(s_update_btn, 0, 0);
    lv_obj_add_event_cb(s_update_btn, update_btn_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *update_lbl = lv_label_create(s_update_btn);
    lv_label_set_text(update_lbl, "Update");
    lv_obj_set_style_text_color(update_lbl, t->white, 0);
    lv_obj_set_style_text_font(update_lbl, &lv_font_montserrat_20, 0);
    lv_obj_center(update_lbl);

    s_progress_bar = lv_bar_create(s_panel);
    lv_obj_set_size(s_progress_bar, UPDATE_BAR_W, UPDATE_BAR_H);
    lv_obj_align(s_progress_bar, LV_ALIGN_TOP_MID, 0, ui_settings_wf_y(s_panel, UPDATE_BAR_Y_WF));
    lv_bar_set_range(s_progress_bar, 0, 100);
    lv_bar_set_value(s_progress_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_radius(s_progress_bar, UPDATE_BAR_H / 2, 0);
    lv_obj_set_style_radius(s_progress_bar, UPDATE_BAR_H / 2, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(s_progress_bar, t->panel, 0);
    lv_obj_set_style_bg_color(s_progress_bar, t->ring, LV_PART_INDICATOR);
    lv_obj_add_flag(s_progress_bar, LV_OBJ_FLAG_HIDDEN);

    s_status_lbl = lv_label_create(s_panel);
    lv_label_set_text(s_status_lbl, "");
    lv_obj_set_style_text_color(s_status_lbl, t->secondary, 0);
    lv_obj_set_style_text_font(s_status_lbl, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_align(s_status_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_status_lbl, UPDATE_BAR_W);
    lv_obj_align(s_status_lbl, LV_ALIGN_TOP_MID, 0, ui_settings_wf_y(s_panel, UPDATE_STATUS_Y_WF));

    s_cancel_wedge = ui_wedge_create_overlay(ui_settings_screen(), UI_WEDGE_CANCEL);
    if (s_cancel_wedge != NULL) {
        ui_wedge_set_visible(s_cancel_wedge, false);
        ui_settings_register_overlay_obj(ui_wedge_get_obj(s_cancel_wedge));
    }

    update_show_panel();
    return s_panel;
}
