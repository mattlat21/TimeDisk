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
#define NET_WEB_Y_WF         115
#define NET_LIST_Y0_WF       200
#define NET_BACKUP_LEN        APP_NTP_SERVER_MAX

typedef enum {
    NET_FIELD_SSID = 0,
    NET_FIELD_PASSWORD,
    NET_FIELD_NTP,
} net_field_t;

typedef enum {
    NET_VIEW_LIST = 0,
    NET_VIEW_NET_DETAIL,
    NET_VIEW_FIELD_EDIT,
} net_view_t;

static lv_obj_t *s_panel;
static lv_obj_t *s_net_panel_title;
static lv_obj_t *s_net_web_lbl;
static lv_obj_t *s_net_list_btns[APP_WIFI_NETWORK_MAX];
static lv_obj_t *s_net_add_btn;
static lv_obj_t *s_net_ntp_btn;
static lv_obj_t *s_net_detail_btns[3];
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
static net_view_t s_net_view = NET_VIEW_LIST;
static int s_net_sel = -1;
static bool s_net_adding = false;

static void net_panel_cancel_cb(lv_event_t *e);
static void network_save_cb(lv_event_t *e);
static void net_edit_cancel_cb(lv_event_t *e);
static void net_edit_save_cb(lv_event_t *e);
static void network_show_list(void);
static void network_show_net_detail(int index);
static void network_show_edit(net_field_t field);

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
}

static void net_hide_list_rows(void)
{
    for (int i = 0; i < APP_WIFI_NETWORK_MAX; i++) {
        if (s_net_list_btns[i] != NULL) {
            lv_obj_add_flag(s_net_list_btns[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (s_net_add_btn != NULL) {
        lv_obj_add_flag(s_net_add_btn, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_net_ntp_btn != NULL) {
        lv_obj_add_flag(s_net_ntp_btn, LV_OBJ_FLAG_HIDDEN);
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

static void network_show_list(void)
{
    app_config_t *draft = ui_settings_draft();
    s_net_view = NET_VIEW_LIST;
    s_net_sel = -1;
    s_net_adding = false;

    net_hide_detail_rows();
    net_hide_edit_rows();

    int row = 0;
    for (int i = 0; i < (int)draft->wifi_network_count; i++) {
        if (s_net_list_btns[i] == NULL) {
            continue;
        }
        lv_label_set_text(lv_obj_get_child(s_net_list_btns[i], 0), draft->wifi_networks[i].ssid);
        lv_obj_align(s_net_list_btns[i], LV_ALIGN_TOP_MID, 0, ui_settings_wf_y(s_panel, net_list_row_y(row)));
        lv_obj_clear_flag(s_net_list_btns[i], LV_OBJ_FLAG_HIDDEN);
        row++;
    }
    for (int i = (int)draft->wifi_network_count; i < APP_WIFI_NETWORK_MAX; i++) {
        if (s_net_list_btns[i] != NULL) {
            lv_obj_add_flag(s_net_list_btns[i], LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (s_net_add_btn != NULL) {
        if (draft->wifi_network_count < APP_WIFI_NETWORK_MAX) {
            lv_obj_align(s_net_add_btn, LV_ALIGN_TOP_MID, 0, ui_settings_wf_y(s_panel, net_list_row_y(row)));
            lv_obj_clear_flag(s_net_add_btn, LV_OBJ_FLAG_HIDDEN);
            row++;
        } else {
            lv_obj_add_flag(s_net_add_btn, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (s_net_ntp_btn != NULL) {
        lv_obj_align(s_net_ntp_btn, LV_ALIGN_TOP_MID, 0, ui_settings_wf_y(s_panel, net_list_row_y(row)));
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
        lv_obj_clear_flag(s_net_panel_title, LV_OBJ_FLAG_HIDDEN);
    }
    net_refresh_web_ui_label(true);
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

    net_hide_list_rows();
    net_hide_detail_rows();
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
        network_show_list();
        return;
    }

    s_net_view = NET_VIEW_NET_DETAIL;
    s_net_sel = index;
    s_net_adding = false;

    net_hide_list_rows();
    net_hide_edit_rows();

    if (s_net_panel_title != NULL) {
        lv_label_set_text(s_net_panel_title, draft->wifi_networks[index].ssid);
        lv_obj_clear_flag(s_net_panel_title, LV_OBJ_FLAG_HIDDEN);
    }
    net_refresh_web_ui_label(false);

    for (int i = 0; i < 3; i++) {
        if (s_net_detail_btns[i] != NULL) {
            lv_obj_align(s_net_detail_btns[i], LV_ALIGN_TOP_MID, 0,
                         ui_settings_wf_y(s_panel, net_list_row_y(i)));
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

static void net_list_row_cb(lv_event_t *e)
{
    int index = (int)(intptr_t)lv_event_get_user_data(e);
    network_show_net_detail(index);
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
        app_config_t *draft = ui_settings_draft();
        if (s_net_sel >= 0 && s_net_sel < (int)draft->wifi_network_count) {
            for (int i = s_net_sel; i < (int)draft->wifi_network_count - 1; i++) {
                draft->wifi_networks[i] = draft->wifi_networks[i + 1];
            }
            memset(&draft->wifi_networks[draft->wifi_network_count - 1], 0, sizeof(app_wifi_network_t));
            draft->wifi_network_count--;
        }
        network_show_list();
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
            network_show_list();
            return;
        }
        if (s_net_active == NET_FIELD_NTP) {
            network_show_list();
            return;
        }
        if (s_net_sel >= 0) {
            network_show_net_detail(s_net_sel);
            return;
        }
        network_show_list();
        return;
    }

    if (s_net_view == NET_VIEW_NET_DETAIL) {
        network_show_list();
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
    network_show_list();
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
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, user_data);
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

    s_net_web_lbl = lv_label_create(s_panel);
    lv_label_set_text(s_net_web_lbl, "");
    lv_obj_set_style_text_color(s_net_web_lbl, t->secondary, 0);
    lv_obj_set_style_text_font(s_net_web_lbl, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_align(s_net_web_lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(s_net_web_lbl, 560);
    lv_obj_align(s_net_web_lbl, LV_ALIGN_TOP_MID, 0, ui_settings_wf_y(s_panel, NET_WEB_Y_WF));
    lv_obj_add_flag(s_net_web_lbl, LV_OBJ_FLAG_HIDDEN);

    for (int i = 0; i < APP_WIFI_NETWORK_MAX; i++) {
        s_net_list_btns[i] = net_create_row_btn(s_panel, "", net_list_row_cb, (void *)(intptr_t)i);
        lv_obj_add_flag(s_net_list_btns[i], LV_OBJ_FLAG_HIDDEN);
    }

    s_net_add_btn = net_create_row_btn(s_panel, "Add Wi-Fi", net_add_cb, NULL);
    lv_obj_add_flag(s_net_add_btn, LV_OBJ_FLAG_HIDDEN);

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
    network_show_list();
}

void ui_settings_network_apply_theme(void)
{
    const ui_theme_t *t = ui_theme_get();
    if (s_net_edit_field_box != NULL) {
        lv_obj_set_style_bg_color(s_net_edit_field_box, t->ring, 0);
    }
    if (s_net_cancel_wedge != NULL) {
        ui_wedge_refresh_theme(s_net_cancel_wedge);
    }
    if (s_net_action_wedge != NULL) {
        ui_wedge_refresh_theme(s_net_action_wedge);
    }
}
