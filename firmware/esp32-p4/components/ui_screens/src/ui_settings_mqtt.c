/**
 * @file ui_settings_mqtt.c
 * @brief Settings -> MQTT broker sub-panel.
 */

#include "ui_screen_settings_internal.h"

#include "ui_keyboard.h"
#include "ui_layout.h"
#include "ui_theme.h"
#include "ui_widgets.h"
#include "ui_wedge.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MQTT_ROW_W           400
#define MQTT_ROW_H           52
#define MQTT_ROW_GAP         8
#define MQTT_ROW_Y0_WF       130
#define MQTT_DEVICE_Y_WF     95
#define MQTT_CHK_Y_WF        72

typedef enum {
    MQTT_FIELD_HOST = 0,
    MQTT_FIELD_PORT,
    MQTT_FIELD_USER,
    MQTT_FIELD_PASS,
} mqtt_field_t;

static lv_obj_t *s_panel;
static lv_obj_t *s_chk_enable;
static lv_obj_t *s_lbl_device;
static lv_obj_t *s_row_btns[4];
static lv_obj_t *s_row_lbls[4];
static lv_obj_t *s_edit_title;
static lv_obj_t *s_edit_box;
static lv_obj_t *s_edit_lbl;
static ui_wedge_t *s_edit_cancel_wedge;
static ui_wedge_t *s_edit_save_wedge;
static ui_keyboard_t *s_kb;
static mqtt_field_t s_active_field = MQTT_FIELD_HOST;
static char s_host_buf[APP_MQTT_HOST_MAX];
static char s_user_buf[APP_MQTT_USER_MAX];
static char s_pass_buf[APP_MQTT_PASS_MAX];
static char s_port_buf[8];
static char s_edit_backup[APP_MQTT_HOST_MAX];

static void mqtt_refresh_rows(void);
static void mqtt_show_list(void);
static void mqtt_show_edit(mqtt_field_t field);

static const char *mqtt_field_title(mqtt_field_t field)
{
    switch (field) {
    case MQTT_FIELD_HOST:
        return "Broker Host";
    case MQTT_FIELD_PORT:
        return "Broker Port";
    case MQTT_FIELD_USER:
        return "Username";
    case MQTT_FIELD_PASS:
        return "Password";
    default:
        return "MQTT";
    }
}

static void mqtt_sync_bufs_from_draft(void)
{
    app_config_t *draft = ui_settings_draft();
    snprintf(s_host_buf, sizeof(s_host_buf), "%s", draft->mqtt_host);
    snprintf(s_user_buf, sizeof(s_user_buf), "%s", draft->mqtt_username);
    snprintf(s_pass_buf, sizeof(s_pass_buf), "%s", draft->mqtt_password);
    snprintf(s_port_buf, sizeof(s_port_buf), "%u", (unsigned)draft->mqtt_port);
}

static void mqtt_apply_bufs_to_draft(void)
{
    app_config_t *draft = ui_settings_draft();
    snprintf(draft->mqtt_host, sizeof(draft->mqtt_host), "%s", s_host_buf);
    snprintf(draft->mqtt_username, sizeof(draft->mqtt_username), "%s", s_user_buf);
    snprintf(draft->mqtt_password, sizeof(draft->mqtt_password), "%s", s_pass_buf);
    draft->mqtt_port = (uint16_t)strtoul(s_port_buf, NULL, 10);
    if (draft->mqtt_port == 0) {
        draft->mqtt_port = 1883;
    }
}

static void mqtt_enable_changed_cb(lv_event_t *e)
{
    (void)e;
    ui_settings_draft()->mqtt_enabled = lv_obj_has_state(s_chk_enable, LV_STATE_CHECKED);
    ui_settings_idle_cb(NULL);
}

static void mqtt_row_cb(lv_event_t *e)
{
    mqtt_field_t field = (mqtt_field_t)(uintptr_t)lv_event_get_user_data(e);
    mqtt_show_edit(field);
    ui_settings_idle_cb(NULL);
}

static void mqtt_edit_cancel_cb(lv_event_t *e)
{
    (void)e;
    switch (s_active_field) {
    case MQTT_FIELD_HOST:
        snprintf(s_host_buf, sizeof(s_host_buf), "%s", s_edit_backup);
        break;
    case MQTT_FIELD_PORT:
        snprintf(s_port_buf, sizeof(s_port_buf), "%s", s_edit_backup);
        break;
    case MQTT_FIELD_USER:
        snprintf(s_user_buf, sizeof(s_user_buf), "%s", s_edit_backup);
        break;
    case MQTT_FIELD_PASS:
        snprintf(s_pass_buf, sizeof(s_pass_buf), "%s", s_edit_backup);
        break;
    default:
        break;
    }
    mqtt_show_list();
}

static char *mqtt_active_buf(void)
{
    switch (s_active_field) {
    case MQTT_FIELD_HOST:
        return s_host_buf;
    case MQTT_FIELD_PORT:
        return s_port_buf;
    case MQTT_FIELD_USER:
        return s_user_buf;
    case MQTT_FIELD_PASS:
        return s_pass_buf;
    default:
        return s_host_buf;
    }
}

static size_t mqtt_active_buf_len(void)
{
    switch (s_active_field) {
    case MQTT_FIELD_HOST:
        return sizeof(s_host_buf);
    case MQTT_FIELD_PORT:
        return sizeof(s_port_buf);
    case MQTT_FIELD_USER:
        return sizeof(s_user_buf);
    case MQTT_FIELD_PASS:
        return sizeof(s_pass_buf);
    default:
        return sizeof(s_host_buf);
    }
}

static void mqtt_edit_refresh_label(void)
{
    char *buf = mqtt_active_buf();
    if (s_active_field == MQTT_FIELD_PASS && buf[0] != '\0') {
        lv_label_set_text(s_edit_lbl, "********");
    } else {
        lv_label_set_text(s_edit_lbl, buf);
    }
}

static void mqtt_edit_save_cb(lv_event_t *e)
{
    (void)e;
    mqtt_apply_bufs_to_draft();
    mqtt_show_list();
}

static void mqtt_bind_keyboard(void)
{
    ui_keyboard_config_t kb_cfg = {
        .buf = mqtt_active_buf(),
        .buf_len = mqtt_active_buf_len(),
        .label = s_edit_lbl,
        .initial_mode = (s_active_field == MQTT_FIELD_PORT) ? UI_KEYBOARD_MODE_NUMBER : UI_KEYBOARD_MODE_LOWER,
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

static void mqtt_show_list(void)
{
    for (int i = 0; i < 4; i++) {
        if (s_row_btns[i] != NULL) {
            lv_obj_clear_flag(s_row_btns[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (s_chk_enable != NULL) {
        lv_obj_clear_flag(s_chk_enable, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_lbl_device != NULL) {
        lv_obj_clear_flag(s_lbl_device, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_edit_title != NULL) {
        lv_obj_add_flag(s_edit_title, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_edit_box != NULL) {
        lv_obj_add_flag(s_edit_box, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_edit_cancel_wedge != NULL) {
        ui_wedge_set_visible(s_edit_cancel_wedge, false);
    }
    if (s_edit_save_wedge != NULL) {
        ui_wedge_set_visible(s_edit_save_wedge, false);
    }
    if (s_kb != NULL) {
        ui_keyboard_set_visible(s_kb, false);
    }
    ui_settings_set_panel_wedges_visible(PANEL_MQTT, true);
    mqtt_refresh_rows();
}

static void mqtt_show_edit(mqtt_field_t field)
{
    s_active_field = field;
    char *buf = mqtt_active_buf();
    snprintf(s_edit_backup, sizeof(s_edit_backup), "%s", buf);

    for (int i = 0; i < 4; i++) {
        if (s_row_btns[i] != NULL) {
            lv_obj_add_flag(s_row_btns[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (s_chk_enable != NULL) {
        lv_obj_add_flag(s_chk_enable, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_lbl_device != NULL) {
        lv_obj_add_flag(s_lbl_device, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_edit_title != NULL) {
        lv_label_set_text(s_edit_title, mqtt_field_title(field));
        lv_obj_clear_flag(s_edit_title, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_edit_box != NULL) {
        lv_obj_clear_flag(s_edit_box, LV_OBJ_FLAG_HIDDEN);
    }
    mqtt_edit_refresh_label();
    ui_settings_set_panel_wedges_visible(PANEL_MQTT, false);
    mqtt_bind_keyboard();
    if (s_edit_cancel_wedge != NULL) {
        ui_wedge_set_visible(s_edit_cancel_wedge, true);
    }
    if (s_edit_save_wedge != NULL) {
        ui_wedge_set_visible(s_edit_save_wedge, true);
    }
}

static void mqtt_refresh_rows(void)
{
    static const char *prefix[] = {"Host", "Port", "User", "Password"};
    char line[96];
    for (int i = 0; i < 4; i++) {
        if (s_row_lbls[i] == NULL) {
            continue;
        }
        switch ((mqtt_field_t)i) {
        case MQTT_FIELD_HOST:
            snprintf(line, sizeof(line), "%s: %s", prefix[i], s_host_buf[0] ? s_host_buf : "(not set)");
            break;
        case MQTT_FIELD_PORT:
            snprintf(line, sizeof(line), "%s: %s", prefix[i], s_port_buf);
            break;
        case MQTT_FIELD_USER:
            snprintf(line, sizeof(line), "%s: %s", prefix[i], s_user_buf[0] ? s_user_buf : "(none)");
            break;
        case MQTT_FIELD_PASS:
            snprintf(line, sizeof(line), "%s: %s", prefix[i], s_pass_buf[0] ? "********" : "(none)");
            break;
        default:
            line[0] = '\0';
            break;
        }
        lv_label_set_text(s_row_lbls[i], line);
    }
}

void ui_settings_mqtt_sync_from_draft(void)
{
    app_config_t *draft = ui_settings_draft();
    mqtt_sync_bufs_from_draft();
    if (draft->mqtt_enabled) {
        lv_obj_add_state(s_chk_enable, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(s_chk_enable, LV_STATE_CHECKED);
    }
    char dev_id[40];
    if (app_config_get_device_id(dev_id, sizeof(dev_id))) {
        char text[64];
        snprintf(text, sizeof(text), "Device ID: %s", dev_id);
        lv_label_set_text(s_lbl_device, text);
    }
    mqtt_show_list();
}

static void mqtt_save_cb(lv_event_t *e)
{
    (void)e;
    app_config_t *cfg = app_config_get();
    app_config_t *draft = ui_settings_draft();
    app_config_t *saved = ui_settings_saved();

    mqtt_apply_bufs_to_draft();
    cfg->mqtt_enabled = draft->mqtt_enabled;
    snprintf(cfg->mqtt_host, sizeof(cfg->mqtt_host), "%s", draft->mqtt_host);
    cfg->mqtt_port = draft->mqtt_port;
    snprintf(cfg->mqtt_username, sizeof(cfg->mqtt_username), "%s", draft->mqtt_username);
    snprintf(cfg->mqtt_password, sizeof(cfg->mqtt_password), "%s", draft->mqtt_password);

    saved->mqtt_enabled = cfg->mqtt_enabled;
    snprintf(saved->mqtt_host, sizeof(saved->mqtt_host), "%s", cfg->mqtt_host);
    saved->mqtt_port = cfg->mqtt_port;
    snprintf(saved->mqtt_username, sizeof(saved->mqtt_username), "%s", cfg->mqtt_username);
    snprintf(saved->mqtt_password, sizeof(saved->mqtt_password), "%s", cfg->mqtt_password);

    app_config_save_mqtt();
    ui_settings_show_panel(PANEL_HUB);
}

static lv_obj_t *mqtt_create_row(lv_obj_t *parent, int row, mqtt_field_t field)
{
    const ui_theme_t *t = ui_theme_get();
    const int x = ui_layout_parent_center_x_wf(parent, MQTT_ROW_W);
    const int y = ui_settings_wf_y(parent, MQTT_ROW_Y0_WF + row * (MQTT_ROW_H + MQTT_ROW_GAP));

    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, MQTT_ROW_W, MQTT_ROW_H);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_radius(btn, 16, 0);
    lv_obj_set_style_bg_color(btn, t->menu_petal, 0);
    lv_obj_set_style_border_width(btn, 0, 0);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_obj_set_style_text_color(lbl, t->white, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0);
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 16, 0);
    s_row_lbls[field] = lbl;

    lv_obj_add_event_cb(btn, mqtt_row_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)field);
    return btn;
}

lv_obj_t *ui_settings_mqtt_build(void)
{
    const ui_theme_t *t = ui_theme_get();

    s_panel = ui_settings_create_sub_panel("MQTT");
    lv_obj_align(lv_obj_get_child(s_panel, 0), LV_ALIGN_TOP_MID, 0, ui_settings_wf_y(s_panel, 48));

    s_chk_enable = lv_checkbox_create(s_panel);
    lv_checkbox_set_text(s_chk_enable, "Enable MQTT");
    lv_obj_set_pos(s_chk_enable, ui_layout_parent_center_x_wf(s_panel, 220), ui_settings_wf_y(s_panel, MQTT_CHK_Y_WF));
    lv_obj_set_style_text_color(s_chk_enable, t->white, 0);
    lv_obj_set_style_text_font(s_chk_enable, &lv_font_montserrat_20, 0);
    lv_obj_add_event_cb(s_chk_enable, mqtt_enable_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);

    s_lbl_device = lv_label_create(s_panel);
    lv_label_set_text(s_lbl_device, "Device ID:");
    lv_obj_set_style_text_color(s_lbl_device, t->secondary, 0);
    lv_obj_set_style_text_font(s_lbl_device, &lv_font_montserrat_16, 0);
    lv_obj_align(s_lbl_device, LV_ALIGN_TOP_MID, 0, ui_settings_wf_y(s_panel, MQTT_DEVICE_Y_WF));

    for (int i = 0; i < 4; i++) {
        s_row_btns[i] = mqtt_create_row(s_panel, i, (mqtt_field_t)i);
    }

    s_edit_title = lv_label_create(s_panel);
    lv_label_set_text(s_edit_title, "");
    lv_obj_set_style_text_color(s_edit_title, t->white, 0);
    lv_obj_set_style_text_font(s_edit_title, &lv_font_montserrat_26, 0);
    lv_obj_align(s_edit_title, LV_ALIGN_TOP_MID, 0, ui_settings_wf_y(s_panel, 72));
    lv_obj_add_flag(s_edit_title, LV_OBJ_FLAG_HIDDEN);

    s_edit_box = lv_obj_create(s_panel);
    lv_obj_set_size(s_edit_box, 400, 78);
    lv_obj_align(s_edit_box, LV_ALIGN_TOP_MID, 0, ui_settings_wf_y(s_panel, 165));
    lv_obj_set_style_bg_color(s_edit_box, t->ring, 0);
    lv_obj_set_style_border_width(s_edit_box, 0, 0);
    lv_obj_set_style_radius(s_edit_box, 20, 0);
    lv_obj_add_flag(s_edit_box, LV_OBJ_FLAG_HIDDEN);

    s_edit_lbl = lv_label_create(s_edit_box);
    lv_obj_set_style_text_color(s_edit_lbl, t->white, 0);
    lv_obj_set_style_text_font(s_edit_lbl, &lv_font_montserrat_26, 0);
    lv_obj_align(s_edit_lbl, LV_ALIGN_LEFT_MID, 20, 0);
    lv_obj_add_event_cb(s_edit_box, mqtt_row_cb, LV_EVENT_CLICKED, NULL);

    s_edit_cancel_wedge = ui_wedge_create_overlay(ui_settings_screen(), UI_WEDGE_CANCEL);
    s_edit_save_wedge = ui_wedge_create_overlay(ui_settings_screen(), UI_WEDGE_CONFIRM);
    if (s_edit_cancel_wedge != NULL) {
        ui_wedge_bind(s_edit_cancel_wedge, UI_WEDGE_CANCEL, mqtt_edit_cancel_cb, NULL);
        ui_wedge_set_visible(s_edit_cancel_wedge, false);
    }
    if (s_edit_save_wedge != NULL) {
        ui_wedge_bind(s_edit_save_wedge, UI_WEDGE_CONFIRM, mqtt_edit_save_cb, NULL);
        ui_wedge_set_visible(s_edit_save_wedge, false);
    }

    ui_settings_attach_panel_wedges(s_panel, PANEL_MQTT, mqtt_save_cb);
    if (s_edit_cancel_wedge != NULL) {
        ui_settings_register_overlay_obj(ui_wedge_get_obj(s_edit_cancel_wedge));
    }
    if (s_edit_save_wedge != NULL) {
        ui_settings_register_overlay_obj(ui_wedge_get_obj(s_edit_save_wedge));
    }

    return s_panel;
}

void ui_settings_mqtt_apply_theme(void)
{
    if (s_edit_cancel_wedge != NULL) {
        ui_wedge_refresh_theme(s_edit_cancel_wedge);
    }
    if (s_edit_save_wedge != NULL) {
        ui_wedge_refresh_theme(s_edit_save_wedge);
    }
}
