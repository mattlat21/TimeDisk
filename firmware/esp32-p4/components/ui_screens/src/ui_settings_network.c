/**
 * @file ui_settings_network.c
 * @brief Settings -> Networking sub-panel (multiple Wi-Fi networks + NTP).
 */

#include "ui_screen_settings_internal.h"

#include "ui_keyboard.h"
#include "ui_layout.h"
#include "ui_theme.h"
#include "ui_widgets.h"
#include "ui_wedge.h"

#include <stdio.h>
#include <string.h>

#include "app_config.h"
#include "app_network.h"

#define HUB_BTN_W           248
#define HUB_BTN_H           44
#define HUB_BTN_GAP_Y       6
#define HUB_BTN_RADIUS      16

void ui_settings_network_sync_from_draft(void);

#define NET_FIELD_Y_WF      165
#define NET_FIELD_W         563
#define NET_FIELD_H         78
#define NET_FIELD_RADIUS    20
#define NET_EDIT_TITLE_Y_WF  72
#define NET_WEB_Y_WF         130
#define NET_MAIN_SSID_Y_WF   95
#define NET_MAIN_MANAGE_Y_WF 210
#define NET_MAIN_NTP_Y_WF    265
#define NET_LIST_Y0_WF       165
#define NET_DETAIL_TITLE_Y_WF  (48 + 200)
#define NET_DETAIL_ROW_Y0_WF   (NET_LIST_Y0_WF + 180)
#define NET_WIFI_ROW_W       ((HUB_BTN_W * 170) / 100)
#define NET_WIFI_GRIP_PAD    14
#define NET_DELETE_CONFIRM_Y_WF 280
#define NET_PLUS_BTN_SIZE    56
#define NET_PLUS_GAP_WF      14
#define NET_BACKUP_LEN        APP_NTP_SERVER_MAX

typedef enum {
    NET_FIELD_SSID = 0,
    NET_FIELD_PASSWORD,
    NET_FIELD_NTP,
} net_field_t;

typedef enum {
    NET_VIEW_MAIN = 0,
    NET_VIEW_WIFI_MANAGE,
    NET_VIEW_NET_DETAIL,
    NET_VIEW_FIELD_EDIT,
    NET_VIEW_DELETE_CONFIRM,
} net_view_t;

static lv_obj_t *s_panel;
static lv_obj_t *s_net_panel_title;
static lv_obj_t *s_net_connected_lbl;
static lv_obj_t *s_net_web_lbl;
static lv_obj_t *s_net_manage_btn;
static lv_obj_t *s_net_list_btns[APP_WIFI_NETWORK_MAX];
static lv_obj_t *s_net_add_plus_btn;
static lv_obj_t *s_net_ntp_btn;
static lv_obj_t *s_net_detail_btns[3];
static lv_obj_t *s_net_delete_confirm_lbl;
static lv_obj_t *s_net_edit_title;
static lv_obj_t *s_net_edit_field_box;
static lv_obj_t *s_net_edit_lbl;
static ui_wedge_t *s_net_cancel_wedge;
static ui_wedge_t *s_net_action_wedge;
static char s_ssid_buf[APP_WIFI_SSID_MAX];
static char s_pw_buf[APP_WIFI_PASSWORD_MAX];
static char s_ntp_buf[APP_NTP_SERVER_MAX];
static char s_net_edit_backup[NET_BACKUP_LEN];
static ui_keyboard_t *s_net_kb;
static net_field_t s_net_active = NET_FIELD_SSID;
static net_view_t s_net_view = NET_VIEW_MAIN;
static int s_net_sel = -1;
static bool s_net_adding = false;
static int s_wifi_drag_index = -1;
static int s_wifi_drag_start_y;
static int s_wifi_drag_start_obj_y;
static bool s_wifi_drag_active;

static void net_panel_cancel_cb(lv_event_t *e);
static void net_manage_cancel_cb(lv_event_t *e);
static void network_save_cb(lv_event_t *e);
static void net_edit_cancel_cb(lv_event_t *e);
static void net_edit_save_cb(lv_event_t *e);
static void network_show_main(void);
static void network_show_wifi_manage(void);
static void network_show_net_detail(int index);
static void network_show_delete_confirm(void);
static void network_show_edit(net_field_t field);
static lv_obj_t *net_wifi_row_ssid_lbl(lv_obj_t *btn);
static lv_obj_t *net_create_wifi_list_btn(lv_obj_t *parent, int index);
static void net_list_row_event_cb(lv_event_t *e);

static const char *net_field_title(net_field_t field)
{
    switch (field) {
    case NET_FIELD_SSID:
        return "Wi-Fi Name";
    case NET_FIELD_PASSWORD:
        return "Wi-Fi Password";
    case NET_FIELD_NTP:
        return "NTP";
    default:
        return "Networking";
    }
}

static int net_list_row_y(int row)
{
    return NET_LIST_Y0_WF + row * (HUB_BTN_H + HUB_BTN_GAP_Y);
}

static int net_detail_row_y(int row)
{
    return NET_DETAIL_ROW_Y0_WF + row * (HUB_BTN_H + HUB_BTN_GAP_Y);
}

static void net_layout_panel_title_default(void)
{
    if (s_net_panel_title != NULL) {
        lv_obj_align(s_net_panel_title, LV_ALIGN_TOP_MID, 0, 24);
    }
}

static void net_refresh_connected_label(void)
{
    if (s_net_connected_lbl == NULL) {
        return;
    }

    char ssid[APP_WIFI_SSID_MAX];
    char text[96];
    if (app_network_get_connected_ssid(ssid, sizeof(ssid))) {
        snprintf(text, sizeof(text), "Connected: %s", ssid);
    } else if (app_network_setup_ap_active()) {
        snprintf(text, sizeof(text), "Setup mode: " APP_NETWORK_SETUP_AP_SSID);
    } else {
        snprintf(text, sizeof(text), "Connected: (not connected)");
    }
    lv_label_set_text(s_net_connected_lbl, text);
}

static void net_refresh_web_ui_label(bool visible)
{
    if (s_net_web_lbl == NULL) {
        return;
    }
    if (!visible) {
        lv_obj_add_flag(s_net_web_lbl, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    char ip[40];
    char url[64];
    char text[160];

    if (app_network_get_device_ip(ip, sizeof(ip))) {
        if (app_network_get_web_ui_url(url, sizeof(url))) {
            if (app_network_setup_ap_active()) {
                snprintf(text, sizeof(text), "IP: %s\nWeb: %s\nJoin Wi-Fi: " APP_NETWORK_SETUP_AP_SSID,
                         ip, url);
            } else {
                snprintf(text, sizeof(text), "IP: %s\nWeb: %s", ip, url);
            }
        } else {
            snprintf(text, sizeof(text), "IP: %s", ip);
        }
        lv_label_set_text(s_net_web_lbl, text);
        lv_obj_clear_flag(s_net_web_lbl, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    if (app_network_get_web_ui_url(url, sizeof(url))) {
        if (app_network_setup_ap_active()) {
            snprintf(text, sizeof(text), "Web: %s\nJoin Wi-Fi: " APP_NETWORK_SETUP_AP_SSID, url);
        } else {
            snprintf(text, sizeof(text), "Web: %s", url);
        }
        lv_label_set_text(s_net_web_lbl, text);
        lv_obj_clear_flag(s_net_web_lbl, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_label_set_text(s_net_web_lbl, "IP: not connected");
    lv_obj_clear_flag(s_net_web_lbl, LV_OBJ_FLAG_HIDDEN);
}

static void net_edit_refresh_label(void)
{
    if (s_net_edit_lbl == NULL) {
        return;
    }
    app_config_t *draft = ui_settings_draft();
    if (s_net_active == NET_FIELD_PASSWORD && s_pw_buf[0] == '\0' && s_net_sel >= 0
        && (size_t)s_net_sel < draft->wifi_network_count) {
        const app_wifi_network_t *net = &draft->wifi_networks[s_net_sel];
        if (net->password_set && net->password[0] != '\0') {
            lv_label_set_text(s_net_edit_lbl, "********");
            return;
        }
    }

    const char *text = s_ssid_buf;
    if (s_net_active == NET_FIELD_PASSWORD) {
        text = s_pw_buf;
    } else if (s_net_active == NET_FIELD_NTP) {
        text = s_ntp_buf;
    }
    lv_label_set_text(s_net_edit_lbl, text);
}

static void net_bind_keyboard(void)
{
    char *buf = s_ssid_buf;
    size_t buf_len = sizeof(s_ssid_buf);

    if (s_net_active == NET_FIELD_PASSWORD) {
        buf = s_pw_buf;
        buf_len = sizeof(s_pw_buf);
    } else if (s_net_active == NET_FIELD_NTP) {
        buf = s_ntp_buf;
        buf_len = sizeof(s_ntp_buf);
    }

    ui_keyboard_config_t kb_cfg = {
        .buf = buf,
        .buf_len = buf_len,
        .label = s_net_edit_lbl,
        .initial_mode = UI_KEYBOARD_MODE_LOWER,
        .on_activity = ui_settings_idle_cb,
        .user_data = NULL,
    };
    if (s_net_kb == NULL) {
        s_net_kb = ui_keyboard_create_overlay(ui_settings_screen(), &kb_cfg);
        if (s_net_kb != NULL) {
            for (int m = 0; m < UI_KEYBOARD_MODE_COUNT; m++) {
                ui_settings_register_overlay_obj(ui_keyboard_get_layer(s_net_kb, (ui_keyboard_mode_t)m));
            }
            ui_settings_register_overlay_obj(ui_keyboard_get_mode_button(s_net_kb));
        }
    } else {
        ui_keyboard_bind(s_net_kb, &kb_cfg);
    }
    ui_keyboard_set_visible(s_net_kb, true);
}

static void net_hide_detail_rows(void)
{
    for (int i = 0; i < 3; i++) {
        if (s_net_detail_btns[i] != NULL) {
            lv_obj_add_flag(s_net_detail_btns[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (s_net_delete_confirm_lbl != NULL) {
        lv_obj_add_flag(s_net_delete_confirm_lbl, LV_OBJ_FLAG_HIDDEN);
    }
}

static void net_hide_main_rows(void)
{
    if (s_net_connected_lbl != NULL) {
        lv_obj_add_flag(s_net_connected_lbl, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_net_manage_btn != NULL) {
        lv_obj_add_flag(s_net_manage_btn, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_net_ntp_btn != NULL) {
        lv_obj_add_flag(s_net_ntp_btn, LV_OBJ_FLAG_HIDDEN);
    }
}

static void net_hide_manage_rows(void)
{
    for (int i = 0; i < APP_WIFI_NETWORK_MAX; i++) {
        if (s_net_list_btns[i] != NULL) {
            lv_obj_add_flag(s_net_list_btns[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (s_net_add_plus_btn != NULL) {
        lv_obj_add_flag(s_net_add_plus_btn, LV_OBJ_FLAG_HIDDEN);
    }
}

static void net_hide_edit_rows(void)
{
    if (s_net_edit_title != NULL) {
        lv_obj_add_flag(s_net_edit_title, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_net_edit_field_box != NULL) {
        lv_obj_add_flag(s_net_edit_field_box, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_net_kb != NULL) {
        ui_keyboard_set_visible(s_net_kb, false);
    }
}

static void net_wifi_move_entry(int from, int to)
{
    app_config_t *draft = ui_settings_draft();
    const int count = (int)draft->wifi_network_count;
    if (from < 0 || to < 0 || from >= count || to >= count || from == to) {
        return;
    }

    app_wifi_network_t tmp = draft->wifi_networks[from];
    if (from < to) {
        for (int i = from; i < to; i++) {
            draft->wifi_networks[i] = draft->wifi_networks[i + 1];
        }
        draft->wifi_networks[to] = tmp;
    } else {
        for (int i = from; i > to; i--) {
            draft->wifi_networks[i] = draft->wifi_networks[i - 1];
        }
        draft->wifi_networks[to] = tmp;
    }
}

static int net_wifi_row_index_from_y(int y)
{
    app_config_t *draft = ui_settings_draft();
    const int count = (int)draft->wifi_network_count;
    if (count <= 0) {
        return 0;
    }

    const int row_h = HUB_BTN_H + HUB_BTN_GAP_Y;
    const int y0 = ui_settings_wf_y(s_panel, net_list_row_y(0));
    int rel = y - y0 + row_h / 2;
    if (rel < 0) {
        return 0;
    }

    int idx = rel / row_h;
    if (idx >= count) {
        idx = count - 1;
    }
    return idx;
}

static void network_show_main(void)
{
    s_net_view = NET_VIEW_MAIN;
    s_net_sel = -1;
    s_net_adding = false;
    s_wifi_drag_active = false;
    s_wifi_drag_index = -1;

    net_hide_manage_rows();
    net_hide_detail_rows();
    net_hide_edit_rows();

    net_refresh_connected_label();
    if (s_net_connected_lbl != NULL) {
        lv_obj_clear_flag(s_net_connected_lbl, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_net_manage_btn != NULL) {
        lv_obj_align(s_net_manage_btn, LV_ALIGN_TOP_MID, 0, ui_settings_wf_y(s_panel, NET_MAIN_MANAGE_Y_WF));
        lv_obj_clear_flag(s_net_manage_btn, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_net_ntp_btn != NULL) {
        lv_obj_align(s_net_ntp_btn, LV_ALIGN_TOP_MID, 0, ui_settings_wf_y(s_panel, NET_MAIN_NTP_Y_WF));
        lv_obj_clear_flag(s_net_ntp_btn, LV_OBJ_FLAG_HIDDEN);
    }

    if (s_net_cancel_wedge != NULL) {
        ui_wedge_bind(s_net_cancel_wedge, UI_WEDGE_CANCEL, net_panel_cancel_cb, NULL);
        ui_wedge_set_visible(s_net_cancel_wedge, true);
    }
    if (s_net_action_wedge != NULL) {
        ui_wedge_bind(s_net_action_wedge, UI_WEDGE_CONFIRM, network_save_cb, NULL);
        ui_wedge_set_visible(s_net_action_wedge, true);
    }
    if (s_net_panel_title != NULL) {
        lv_label_set_text(s_net_panel_title, "Networking");
        net_layout_panel_title_default();
        lv_obj_clear_flag(s_net_panel_title, LV_OBJ_FLAG_HIDDEN);
    }
    net_refresh_web_ui_label(true);
    ui_settings_idle_cb(NULL);
}

static void network_show_wifi_manage(void)
{
    app_config_t *draft = ui_settings_draft();
    s_net_view = NET_VIEW_WIFI_MANAGE;
    s_net_sel = -1;
    s_net_adding = false;
    s_wifi_drag_active = false;
    s_wifi_drag_index = -1;

    net_hide_main_rows();
    net_hide_detail_rows();
    net_hide_edit_rows();

    int row = 0;
    for (int i = 0; i < (int)draft->wifi_network_count; i++) {
        if (s_net_list_btns[i] == NULL) {
            continue;
        }
        lv_obj_t *ssid_lbl = net_wifi_row_ssid_lbl(s_net_list_btns[i]);
        if (ssid_lbl != NULL) {
            lv_label_set_text(ssid_lbl, draft->wifi_networks[i].ssid);
        }
        lv_obj_set_pos(s_net_list_btns[i], ui_layout_parent_center_x_wf(s_panel, NET_WIFI_ROW_W),
                       ui_settings_wf_y(s_panel, net_list_row_y(row)));
        lv_obj_clear_flag(s_net_list_btns[i], LV_OBJ_FLAG_HIDDEN);
        row++;
    }
    for (int i = (int)draft->wifi_network_count; i < APP_WIFI_NETWORK_MAX; i++) {
        if (s_net_list_btns[i] != NULL) {
            lv_obj_add_flag(s_net_list_btns[i], LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (s_net_add_plus_btn != NULL) {
        if (draft->wifi_network_count < APP_WIFI_NETWORK_MAX) {
            const int plus_y = net_list_row_y(row) + NET_PLUS_GAP_WF;
            lv_obj_align(s_net_add_plus_btn, LV_ALIGN_TOP_MID, 0, ui_settings_wf_y(s_panel, plus_y));
            lv_obj_clear_flag(s_net_add_plus_btn, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_net_add_plus_btn, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (s_net_cancel_wedge != NULL) {
        ui_wedge_bind(s_net_cancel_wedge, UI_WEDGE_CANCEL, net_manage_cancel_cb, NULL);
        ui_wedge_set_visible(s_net_cancel_wedge, true);
    }
    if (s_net_action_wedge != NULL) {
        ui_wedge_set_visible(s_net_action_wedge, false);
    }
    if (s_net_panel_title != NULL) {
        lv_label_set_text(s_net_panel_title, "Manage Wi-Fi");
        net_layout_panel_title_default();
        lv_obj_clear_flag(s_net_panel_title, LV_OBJ_FLAG_HIDDEN);
    }
    net_refresh_web_ui_label(false);
    ui_settings_idle_cb(NULL);
}

static void network_load_field_buf(net_field_t field)
{
    app_config_t *draft = ui_settings_draft();
    switch (field) {
    case NET_FIELD_SSID:
        if (s_net_sel >= 0 && (size_t)s_net_sel < draft->wifi_network_count) {
            snprintf(s_ssid_buf, sizeof(s_ssid_buf), "%s", draft->wifi_networks[s_net_sel].ssid);
        } else {
            s_ssid_buf[0] = '\0';
        }
        break;
    case NET_FIELD_PASSWORD:
        s_pw_buf[0] = '\0';
        break;
    case NET_FIELD_NTP:
        snprintf(s_ntp_buf, sizeof(s_ntp_buf), "%s", draft->ntp_server);
        break;
    default:
        break;
    }
}

static void network_show_edit(net_field_t field)
{
    s_net_view = NET_VIEW_FIELD_EDIT;
    s_net_active = field;
    network_load_field_buf(field);

    if (field == NET_FIELD_SSID) {
        snprintf(s_net_edit_backup, sizeof(s_net_edit_backup), "%s", s_ssid_buf);
    } else if (field == NET_FIELD_PASSWORD) {
        s_net_edit_backup[0] = '\0';
    } else {
        snprintf(s_net_edit_backup, sizeof(s_net_edit_backup), "%s", s_ntp_buf);
    }

    if (s_net_edit_title != NULL) {
        lv_label_set_text(s_net_edit_title, net_field_title(field));
        lv_obj_clear_flag(s_net_edit_title, LV_OBJ_FLAG_HIDDEN);
    }
    net_edit_refresh_label();

    net_hide_main_rows();
    net_hide_manage_rows();
    if (s_net_panel_title != NULL) {
        lv_obj_add_flag(s_net_panel_title, LV_OBJ_FLAG_HIDDEN);
    }
    net_refresh_web_ui_label(false);
    if (s_net_edit_field_box != NULL) {
        lv_obj_clear_flag(s_net_edit_field_box, LV_OBJ_FLAG_HIDDEN);
    }
    net_bind_keyboard();

    if (s_net_cancel_wedge != NULL) {
        ui_wedge_bind(s_net_cancel_wedge, UI_WEDGE_CANCEL, net_edit_cancel_cb, NULL);
        ui_wedge_set_visible(s_net_cancel_wedge, true);
    }
    if (s_net_action_wedge != NULL) {
        ui_wedge_bind(s_net_action_wedge, UI_WEDGE_CONFIRM, net_edit_save_cb, NULL);
        ui_wedge_set_visible(s_net_action_wedge, true);
    }
    ui_settings_idle_cb(NULL);
}

static void network_show_net_detail(int index)
{
    app_config_t *draft = ui_settings_draft();
    if (index < 0 || index >= (int)draft->wifi_network_count) {
        network_show_wifi_manage();
        return;
    }

    s_net_view = NET_VIEW_NET_DETAIL;
    s_net_sel = index;
    s_net_adding = false;

    net_hide_main_rows();
    net_hide_manage_rows();
    net_hide_edit_rows();

    if (s_net_panel_title != NULL) {
        lv_label_set_text(s_net_panel_title, draft->wifi_networks[index].ssid);
        lv_obj_align(s_net_panel_title, LV_ALIGN_TOP_MID, 0,
                      ui_settings_wf_y(s_panel, NET_DETAIL_TITLE_Y_WF));
        lv_obj_clear_flag(s_net_panel_title, LV_OBJ_FLAG_HIDDEN);
    }
    net_refresh_web_ui_label(false);

    for (int i = 0; i < 3; i++) {
        if (s_net_detail_btns[i] != NULL) {
            lv_obj_align(s_net_detail_btns[i], LV_ALIGN_TOP_MID, 0,
                         ui_settings_wf_y(s_panel, net_detail_row_y(i)));
            lv_obj_clear_flag(s_net_detail_btns[i], LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (s_net_cancel_wedge != NULL) {
        ui_wedge_bind(s_net_cancel_wedge, UI_WEDGE_CANCEL, net_edit_cancel_cb, NULL);
        ui_wedge_set_visible(s_net_cancel_wedge, true);
    }
    if (s_net_action_wedge != NULL) {
        ui_wedge_set_visible(s_net_action_wedge, false);
    }
    ui_settings_idle_cb(NULL);
}

static void net_wifi_delete_selected(void)
{
    app_config_t *draft = ui_settings_draft();
    if (s_net_sel >= 0 && s_net_sel < (int)draft->wifi_network_count) {
        for (int i = s_net_sel; i < (int)draft->wifi_network_count - 1; i++) {
            draft->wifi_networks[i] = draft->wifi_networks[i + 1];
        }
        memset(&draft->wifi_networks[draft->wifi_network_count - 1], 0, sizeof(app_wifi_network_t));
        draft->wifi_network_count--;
    }
}

static void net_delete_confirm_cancel_cb(lv_event_t *e)
{
    (void)e;
    network_show_net_detail(s_net_sel);
}

static void net_delete_confirm_yes_cb(lv_event_t *e)
{
    (void)e;
    net_wifi_delete_selected();
    network_show_wifi_manage();
}

static void network_show_delete_confirm(void)
{
    s_net_view = NET_VIEW_DELETE_CONFIRM;

    net_hide_main_rows();
    net_hide_manage_rows();
    net_hide_edit_rows();
    net_hide_detail_rows();

    if (s_net_panel_title != NULL) {
        lv_obj_add_flag(s_net_panel_title, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_net_delete_confirm_lbl != NULL) {
        lv_obj_clear_flag(s_net_delete_confirm_lbl, LV_OBJ_FLAG_HIDDEN);
    }
    net_refresh_web_ui_label(false);

    if (s_net_cancel_wedge != NULL) {
        ui_wedge_bind(s_net_cancel_wedge, UI_WEDGE_CANCEL, net_delete_confirm_cancel_cb, NULL);
        ui_wedge_set_visible(s_net_cancel_wedge, true);
    }
    if (s_net_action_wedge != NULL) {
        ui_wedge_bind(s_net_action_wedge, UI_WEDGE_CONFIRM, net_delete_confirm_yes_cb, NULL);
        ui_wedge_set_visible(s_net_action_wedge, true);
    }
    ui_settings_idle_cb(NULL);
}

static void net_list_row_event_cb(lv_event_t *e)
{
    const lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);
    const int index = (int)(intptr_t)lv_event_get_user_data(e);
    lv_indev_t *indev = lv_indev_active();
    lv_point_t p;

    if (code == LV_EVENT_PRESSED) {
        s_wifi_drag_index = index;
        s_wifi_drag_active = false;
        if (indev != NULL) {
            lv_indev_get_point(indev, &p);
            s_wifi_drag_start_y = p.y;
        }
        s_wifi_drag_start_obj_y = lv_obj_get_y(btn);
        return;
    }

    if (code == LV_EVENT_PRESSING) {
        if (indev == NULL) {
            return;
        }
        lv_indev_get_point(indev, &p);
        const int dy = p.y - s_wifi_drag_start_y;
        if (dy > 8 || dy < -8) {
            s_wifi_drag_active = true;
        }
        if (s_wifi_drag_active) {
            lv_obj_set_y(btn, s_wifi_drag_start_obj_y + dy);
        }
        return;
    }

    if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        if (s_wifi_drag_active && s_wifi_drag_index >= 0) {
            if (indev != NULL) {
                lv_indev_get_point(indev, &p);
                const int target = net_wifi_row_index_from_y(p.y);
                net_wifi_move_entry(s_wifi_drag_index, target);
            }
            network_show_wifi_manage();
        } else if (code == LV_EVENT_RELEASED) {
            network_show_net_detail(index);
        }
        s_wifi_drag_active = false;
        s_wifi_drag_index = -1;
    }
}

static void net_manage_cb(lv_event_t *e)
{
    (void)e;
    network_show_wifi_manage();
}

static void net_manage_cancel_cb(lv_event_t *e)
{
    (void)e;
    network_show_main();
}

static void net_add_cb(lv_event_t *e)
{
    (void)e;
    app_config_t *draft = ui_settings_draft();
    if (draft->wifi_network_count >= APP_WIFI_NETWORK_MAX) {
        return;
    }
    s_net_sel = (int)draft->wifi_network_count;
    s_net_adding = true;
    network_show_edit(NET_FIELD_SSID);
}

static void net_ntp_cb(lv_event_t *e)
{
    (void)e;
    s_net_sel = -1;
    s_net_adding = false;
    network_show_edit(NET_FIELD_NTP);
}

static void net_detail_row_cb(lv_event_t *e)
{
    int action = (int)(intptr_t)lv_event_get_user_data(e);
    if (action == 0) {
        network_show_edit(NET_FIELD_SSID);
    } else if (action == 1) {
        network_show_edit(NET_FIELD_PASSWORD);
    } else {
        network_show_delete_confirm();
    }
}

static void net_edit_cancel_cb(lv_event_t *e)
{
    (void)e;
    if (s_net_view == NET_VIEW_FIELD_EDIT) {
        if (s_net_active == NET_FIELD_SSID) {
            snprintf(s_ssid_buf, sizeof(s_ssid_buf), "%s", s_net_edit_backup);
        } else if (s_net_active == NET_FIELD_PASSWORD) {
            s_pw_buf[0] = '\0';
        } else {
            snprintf(s_ntp_buf, sizeof(s_ntp_buf), "%s", s_net_edit_backup);
        }

        if (s_net_adding) {
            network_show_wifi_manage();
            return;
        }
        if (s_net_active == NET_FIELD_NTP) {
            network_show_main();
            return;
        }
        if (s_net_sel >= 0) {
            network_show_net_detail(s_net_sel);
            return;
        }
        network_show_main();
        return;
    }

    if (s_net_view == NET_VIEW_NET_DETAIL) {
        network_show_wifi_manage();
        return;
    }

    if (s_net_view == NET_VIEW_DELETE_CONFIRM) {
        network_show_net_detail(s_net_sel);
    }
}

static void net_edit_save_cb(lv_event_t *e)
{
    (void)e;
    app_config_t *draft = ui_settings_draft();

    if (s_net_active == NET_FIELD_SSID) {
        if (s_ssid_buf[0] == '\0') {
            return;
        }
        if (s_net_adding && s_net_sel == (int)draft->wifi_network_count
            && draft->wifi_network_count < APP_WIFI_NETWORK_MAX) {
            app_wifi_network_t *net = &draft->wifi_networks[s_net_sel];
            snprintf(net->ssid, sizeof(net->ssid), "%s", s_ssid_buf);
            net->password[0] = '\0';
            net->password_set = false;
            draft->wifi_network_count++;
            s_net_adding = false;
            network_show_edit(NET_FIELD_PASSWORD);
            return;
        }
        if (s_net_sel >= 0 && s_net_sel < (int)draft->wifi_network_count) {
            app_wifi_network_t *net = &draft->wifi_networks[s_net_sel];
            snprintf(net->ssid, sizeof(net->ssid), "%s", s_ssid_buf);
        }
        network_show_net_detail(s_net_sel);
        return;
    }

    if (s_net_active == NET_FIELD_PASSWORD) {
        if (s_net_sel >= 0 && s_net_sel < (int)draft->wifi_network_count) {
            app_wifi_network_t *net = &draft->wifi_networks[s_net_sel];
            if (s_pw_buf[0] != '\0') {
                snprintf(net->password, sizeof(net->password), "%s", s_pw_buf);
            }
            net->password_set = true;
        }
        network_show_net_detail(s_net_sel);
        return;
    }

    if (s_ntp_buf[0] == '\0') {
        return;
    }
    snprintf(draft->ntp_server, sizeof(draft->ntp_server), "%s", s_ntp_buf);
    network_show_main();
}

static void net_panel_cancel_cb(lv_event_t *e)
{
    (void)e;
    app_config_t *cfg = app_config_get();
    app_config_t *draft = ui_settings_draft();
    app_config_t *saved = ui_settings_saved();

    app_config_wifi_networks_copy(draft->wifi_networks, &draft->wifi_network_count,
                                  saved->wifi_networks, saved->wifi_network_count);
    memcpy(draft->ntp_server, saved->ntp_server, sizeof(draft->ntp_server));
    memcpy(cfg, draft, sizeof(*cfg));
    ui_settings_network_sync_from_draft();
    ui_settings_show_panel(PANEL_HUB);
}

static void network_save_cb(lv_event_t *e)
{
    (void)e;
    app_config_t *cfg = app_config_get();
    app_config_t *draft = ui_settings_draft();
    app_config_t *saved = ui_settings_saved();

    memcpy(cfg->wifi_networks, draft->wifi_networks, sizeof(cfg->wifi_networks));
    cfg->wifi_network_count = draft->wifi_network_count;
    memcpy(cfg->ntp_server, draft->ntp_server, sizeof(cfg->ntp_server));
    memcpy(saved, cfg, sizeof(*saved));
    app_config_save_network();
    ui_settings_show_panel(PANEL_HUB);
}

static lv_obj_t *net_wifi_row_ssid_lbl(lv_obj_t *btn)
{
    if (btn == NULL) {
        return NULL;
    }
    /* child 0 = grip, child 1 = SSID label */
    return lv_obj_get_child(btn, 1);
}

static lv_obj_t *net_create_wifi_list_btn(lv_obj_t *parent, int index)
{
    const ui_theme_t *t = ui_theme_get();
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, NET_WIFI_ROW_W, HUB_BTN_H);
    lv_obj_set_style_radius(btn, HUB_BTN_RADIUS, 0);
    lv_obj_set_style_bg_color(btn, t->menu_petal, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_pad_left(btn, NET_WIFI_GRIP_PAD, 0);

    lv_obj_t *grip = lv_label_create(btn);
    lv_label_set_text(grip, LV_SYMBOL_BARS);
    lv_obj_set_style_text_color(grip, t->secondary, 0);
    lv_obj_set_style_text_font(grip, &lv_font_montserrat_20, 0);
    lv_obj_align(grip, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, "");
    lv_obj_set_style_text_color(lbl, t->white, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0);
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 36, 0);

    lv_obj_add_event_cb(btn, net_list_row_event_cb, LV_EVENT_PRESSED, (void *)(intptr_t)index);
    lv_obj_add_event_cb(btn, net_list_row_event_cb, LV_EVENT_PRESSING, (void *)(intptr_t)index);
    lv_obj_add_event_cb(btn, net_list_row_event_cb, LV_EVENT_RELEASED, (void *)(intptr_t)index);
    lv_obj_add_event_cb(btn, net_list_row_event_cb, LV_EVENT_PRESS_LOST, (void *)(intptr_t)index);
    return btn;
}

static lv_obj_t *net_create_row_btn(lv_obj_t *parent, const char *text, lv_event_cb_t cb, void *user_data)
{
    const ui_theme_t *t = ui_theme_get();
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, HUB_BTN_W, HUB_BTN_H);
    lv_obj_set_style_radius(btn, HUB_BTN_RADIUS, 0);
    lv_obj_set_style_bg_color(btn, t->menu_petal, 0);
    lv_obj_set_style_border_width(btn, 0, 0);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, t->white, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0);
    lv_obj_center(lbl);
    if (cb != NULL) {
        lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, user_data);
    }
    return btn;
}

static lv_obj_t *net_create_edit_field(lv_obj_t *parent, lv_obj_t **lbl_out)
{
    const ui_theme_t *t = ui_theme_get();

    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_set_size(box, NET_FIELD_W, NET_FIELD_H);
    lv_obj_align(box, LV_ALIGN_TOP_MID, 0, ui_settings_wf_y(parent, NET_FIELD_Y_WF));
    lv_obj_set_style_bg_color(box, t->ring, 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(box, 0, 0);
    lv_obj_set_style_radius(box, NET_FIELD_RADIUS, 0);
    lv_obj_set_style_pad_left(box, 24, 0);
    lv_obj_remove_flag(box, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(box);
    lv_label_set_text(lbl, "");
    lv_obj_set_style_text_color(lbl, t->white, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_26, 0);
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 0, 0);
    *lbl_out = lbl;
    return box;
}

lv_obj_t *ui_settings_network_build(void)
{
    const ui_theme_t *t = ui_theme_get();

    s_panel = lv_obj_create(ui_settings_screen());
    lv_obj_set_style_bg_opa(s_panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_panel, 0, 0);
    lv_obj_remove_flag(s_panel, LV_OBJ_FLAG_SCROLLABLE);
    ui_settings_panel_layout(s_panel);
    lv_obj_add_flag(s_panel, LV_OBJ_FLAG_HIDDEN);

    s_net_panel_title = ui_widgets_create_title(s_panel, "Networking");

    s_net_connected_lbl = lv_label_create(s_panel);
    lv_label_set_text(s_net_connected_lbl, "Connected:");
    lv_obj_set_style_text_color(s_net_connected_lbl, t->white, 0);
    lv_obj_set_style_text_font(s_net_connected_lbl, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_align(s_net_connected_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_net_connected_lbl, 560);
    lv_obj_align(s_net_connected_lbl, LV_ALIGN_TOP_MID, 0, ui_settings_wf_y(s_panel, NET_MAIN_SSID_Y_WF));

    s_net_web_lbl = lv_label_create(s_panel);
    lv_label_set_text(s_net_web_lbl, "");
    lv_obj_set_style_text_color(s_net_web_lbl, t->secondary, 0);
    lv_obj_set_style_text_font(s_net_web_lbl, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_align(s_net_web_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_net_web_lbl, 560);
    lv_obj_align(s_net_web_lbl, LV_ALIGN_TOP_MID, 0, ui_settings_wf_y(s_panel, NET_WEB_Y_WF));
    lv_obj_add_flag(s_net_web_lbl, LV_OBJ_FLAG_HIDDEN);

    s_net_manage_btn = net_create_row_btn(s_panel, "Manage Wi-Fi", net_manage_cb, NULL);

    for (int i = 0; i < APP_WIFI_NETWORK_MAX; i++) {
        s_net_list_btns[i] = net_create_wifi_list_btn(s_panel, i);
        lv_obj_add_flag(s_net_list_btns[i], LV_OBJ_FLAG_HIDDEN);
    }

    s_net_add_plus_btn = lv_button_create(s_panel);
    lv_obj_set_size(s_net_add_plus_btn, NET_PLUS_BTN_SIZE, NET_PLUS_BTN_SIZE);
    lv_obj_set_style_radius(s_net_add_plus_btn, NET_PLUS_BTN_SIZE / 2, 0);
    lv_obj_set_style_bg_color(s_net_add_plus_btn, t->menu_petal, 0);
    lv_obj_set_style_border_width(s_net_add_plus_btn, 0, 0);
    lv_obj_add_event_cb(s_net_add_plus_btn, net_add_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(s_net_add_plus_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_t *plus_lbl = lv_label_create(s_net_add_plus_btn);
    lv_label_set_text(plus_lbl, "+");
    lv_obj_set_style_text_color(plus_lbl, t->white, 0);
    lv_obj_set_style_text_font(plus_lbl, &lv_font_montserrat_48, 0);
    lv_obj_center(plus_lbl);

    s_net_ntp_btn = net_create_row_btn(s_panel, "NTP", net_ntp_cb, NULL);
    lv_obj_add_flag(s_net_ntp_btn, LV_OBJ_FLAG_HIDDEN);

    static const char *detail_labels[] = {"Wi-Fi Name", "Wi-Fi Password", "Delete Network"};
    for (int i = 0; i < 3; i++) {
        s_net_detail_btns[i] = net_create_row_btn(s_panel, detail_labels[i], net_detail_row_cb,
                                                  (void *)(intptr_t)i);
        lv_obj_add_flag(s_net_detail_btns[i], LV_OBJ_FLAG_HIDDEN);
        if (i == 2) {
            lv_obj_set_style_bg_color(s_net_detail_btns[i], t->orange, 0);
        }
    }

    s_net_delete_confirm_lbl = lv_label_create(s_panel);
    lv_label_set_text(s_net_delete_confirm_lbl, "Are you sure?");
    lv_obj_set_style_text_color(s_net_delete_confirm_lbl, t->white, 0);
    lv_obj_set_style_text_font(s_net_delete_confirm_lbl, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_align(s_net_delete_confirm_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_net_delete_confirm_lbl, 560);
    lv_obj_align(s_net_delete_confirm_lbl, LV_ALIGN_TOP_MID, 0,
                 ui_settings_wf_y(s_panel, NET_DELETE_CONFIRM_Y_WF));
    lv_obj_add_flag(s_net_delete_confirm_lbl, LV_OBJ_FLAG_HIDDEN);

    s_net_cancel_wedge = ui_wedge_create_overlay(ui_settings_screen(), UI_WEDGE_CANCEL);
    s_net_action_wedge = ui_wedge_create_overlay(ui_settings_screen(), UI_WEDGE_CONFIRM);
    if (s_net_cancel_wedge != NULL) {
        ui_wedge_set_visible(s_net_cancel_wedge, false);
        ui_settings_register_overlay_obj(ui_wedge_get_obj(s_net_cancel_wedge));
    }
    if (s_net_action_wedge != NULL) {
        ui_wedge_set_visible(s_net_action_wedge, false);
        ui_settings_register_overlay_obj(ui_wedge_get_obj(s_net_action_wedge));
    }

    s_net_edit_title = ui_widgets_create_title(s_panel, "Wi-Fi Name");
    lv_obj_align(s_net_edit_title, LV_ALIGN_TOP_MID, 0,
                 ui_settings_wf_y(s_panel, NET_EDIT_TITLE_Y_WF));
    lv_obj_add_flag(s_net_edit_title, LV_OBJ_FLAG_HIDDEN);

    s_net_edit_field_box = net_create_edit_field(s_panel, &s_net_edit_lbl);
    lv_obj_add_flag(s_net_edit_field_box, LV_OBJ_FLAG_HIDDEN);

    return s_panel;
}

void ui_settings_network_sync_from_draft(void)
{
    app_config_t *draft = ui_settings_draft();
    s_ssid_buf[0] = '\0';
    s_pw_buf[0] = '\0';
    snprintf(s_ntp_buf, sizeof(s_ntp_buf), "%s", draft->ntp_server);
    network_show_main();
}

void ui_settings_network_apply_theme(void)
{
    const ui_theme_t *t = ui_theme_get();
    if (s_net_edit_field_box != NULL) {
        lv_obj_set_style_bg_color(s_net_edit_field_box, t->ring, 0);
    }
    if (s_net_add_plus_btn != NULL) {
        lv_obj_set_style_bg_color(s_net_add_plus_btn, t->menu_petal, 0);
    }
    if (s_net_cancel_wedge != NULL) {
        ui_wedge_refresh_theme(s_net_cancel_wedge);
    }
    if (s_net_action_wedge != NULL) {
        ui_wedge_refresh_theme(s_net_action_wedge);
    }
}
