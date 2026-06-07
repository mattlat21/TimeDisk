/**
 * @file ui_screen_settings.c
 * @brief Settings hub and editable sub-panels (draft / saved model).
 */

#include "ui_screens_registry.h"
#include "ui_screen_settings_internal.h"
#include "ui_layout.h"
#include "ui_widgets.h"
#include "ui_wedge.h"
#include "ui_theme.h"
#include "ui_keyboard.h"
#include "ui_nav.h"
#include "app_config.h"
#include "app_network.h"
#include "app_time.h"
#include <stdio.h>
#include <string.h>

#define HUB_BTN_W           248
#define HUB_BTN_H           52
#define HUB_BTN_GAP_X       14
#define HUB_BTN_GAP_Y       10
#define HUB_BTN_COLS        2
#define HUB_BTN_RADIUS      16

static lv_obj_t *s_scr;
static lv_obj_t *s_panels[PANEL_COUNT];
static lv_obj_t *s_overlay_objs[24];
static size_t s_overlay_obj_count;

static app_config_t s_saved;
static app_config_t s_draft;

/* Panels (now split into modules). */
lv_obj_t *ui_settings_colours_build(void);
void ui_settings_colours_sync_from_draft(void);
void ui_settings_colours_apply_theme(void);

lv_obj_t *ui_settings_network_build(void);
void ui_settings_network_sync_from_draft(void);
void ui_settings_network_apply_theme(void);

lv_obj_t *ui_settings_timezone_build(void);
void ui_settings_timezone_select_from_draft(void);
void ui_settings_timezone_apply_theme(void);

lv_obj_t *ui_settings_schedule_build(void);
void ui_settings_schedule_sync_from_draft(void);

lv_obj_t *ui_settings_timeouts_build(void);
void ui_settings_timeouts_show_list(void);

lv_obj_t *ui_settings_adult_auth_build(void);
void ui_settings_adult_auth_sync_from_draft(void);

lv_obj_t *ui_settings_screen(void)
{
    return s_scr;
}

app_config_t *ui_settings_draft(void) { return &s_draft; }
app_config_t *ui_settings_saved(void) { return &s_saved; }

void ui_settings_register_overlay_obj(lv_obj_t *obj)
{
    if (obj == NULL) {
        return;
    }
    if (s_overlay_obj_count >= (sizeof(s_overlay_objs) / sizeof(s_overlay_objs[0]))) {
        return;
    }
    s_overlay_objs[s_overlay_obj_count++] = obj;
}

int ui_settings_wf_y(lv_obj_t *parent, int y_wf)
{
    int x = 0;
    int y = 0;
    ui_layout_parent_pos_from_wf(parent, 0, y_wf, &x, &y);
    return y;
}

int ui_settings_wf_x(lv_obj_t *parent, int x_wf)
{
    int x = 0;
    int y = 0;
    ui_layout_parent_pos_from_wf(parent, x_wf, 0, &x, &y);
    return x;
}

static ui_wedge_t *s_hub_cancel_wedge;
static lv_obj_t *s_hub_web_lbl;
static ui_wedge_t *s_panel_cancel_wedge[PANEL_COUNT];
static ui_wedge_t *s_panel_save_wedge[PANEL_COUNT];

static void show_panel(settings_panel_t panel);
void ui_settings_show_panel(settings_panel_t panel);
void ui_settings_panel_layout(lv_obj_t *panel);
static void settings_wedges_hide_all(void);
static void settings_wedges_show_for_panel(settings_panel_t panel);
void ui_settings_idle_cb(void *user_data);

static void hub_cancel_cb(lv_event_t *e)
{
    (void)e;
    app_runtime_t *rt = app_runtime_get();
    if (rt->time_valid) {
        ui_nav_go(UI_SCREEN_MENU);
    } else {
        ui_nav_go(UI_SCREEN_LOADING);
    }
}

static void hub_btn_cb(lv_event_t *e)
{
    settings_panel_t target = (settings_panel_t)(uintptr_t)lv_event_get_user_data(e);
    show_panel(target);
    ui_nav_reset_idle_timer();
}

static lv_obj_t *hub_create_btn(lv_obj_t *parent, const char *text, int x, int y,
                                settings_panel_t target)
{
    const ui_theme_t *t = ui_theme_get();
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, HUB_BTN_W, HUB_BTN_H);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_radius(btn, HUB_BTN_RADIUS, 0);
    lv_obj_set_style_bg_color(btn, t->menu_petal, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, t->white, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0);
    lv_obj_center(lbl);

    lv_obj_add_event_cb(btn, hub_btn_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)target);
    return btn;
}

static void panel_cancel_cb(lv_event_t *e)
{
    settings_panel_t panel = (settings_panel_t)(uintptr_t)lv_event_get_user_data(e);
    app_config_t *cfg = app_config_get();

    switch (panel) {
    case PANEL_COLOURS:
        s_draft.ui_primary_color = s_saved.ui_primary_color;
        s_draft.ui_secondary_color = s_saved.ui_secondary_color;
        break;
    case PANEL_NETWORK:
        app_config_wifi_networks_copy(s_draft.wifi_networks, &s_draft.wifi_network_count,
                                      s_saved.wifi_networks, s_saved.wifi_network_count);
        memcpy(s_draft.ntp_server, s_saved.ntp_server, sizeof(s_draft.ntp_server));
        ui_settings_network_sync_from_draft();
        break;
    case PANEL_TIMEZONE:
        memcpy(s_draft.timezone_id, s_saved.timezone_id, sizeof(s_draft.timezone_id));
        s_draft.timezone_set = s_saved.timezone_set;
        break;
    case PANEL_SCHEDULE:
        s_draft.wind_down_sec = s_saved.wind_down_sec;
        s_draft.sleep_sec = s_saved.sleep_sec;
        s_draft.rest_sec = s_saved.rest_sec;
        break;
    case PANEL_TIMEOUTS:
        if (ui_settings_timeouts_on_cancel()) {
            return;
        }
        s_draft.timeout_splash_sec = s_saved.timeout_splash_sec;
        s_draft.timeout_tod_dim_sec = s_saved.timeout_tod_dim_sec;
        s_draft.timeout_tod_menu_sec = s_saved.timeout_tod_menu_sec;
        s_draft.timeout_aa_sec = s_saved.timeout_aa_sec;
        s_draft.timeout_main_menu_sec = s_saved.timeout_main_menu_sec;
        s_draft.timeout_timer_dim_sec = s_saved.timeout_timer_dim_sec;
        ui_settings_timeouts_show_list();
        break;
    case PANEL_AA:
        s_draft.aa_methods = s_saved.aa_methods;
        memcpy(s_draft.aa_pin, s_saved.aa_pin, sizeof(s_draft.aa_pin));
        break;
    default:
        break;
    }

    memcpy(cfg, &s_draft, sizeof(*cfg));

    if (panel == PANEL_COLOURS) {
        ui_settings_colours_sync_from_draft();
        ui_theme_apply();
    } else if (panel == PANEL_TIMEZONE && s_saved.timezone_set) {
        app_time_apply_timezone_id(s_saved.timezone_id);
    }

    show_panel(PANEL_HUB);
}

void ui_settings_panel_layout(lv_obj_t *panel)
{
    int32_t cw = 0;
    int32_t ch = 0;

    ui_layout_get_content_size(s_scr, &cw, &ch);
    lv_obj_set_size(panel, cw, ch);
    lv_obj_set_style_pad_all(panel, 0, 0);
    lv_obj_align(panel, LV_ALIGN_TOP_LEFT, 0, 0);
}

static void settings_wedges_hide_all(void)
{
    if (s_hub_cancel_wedge != NULL) {
        ui_wedge_set_visible(s_hub_cancel_wedge, false);
    }
    for (int i = 0; i < PANEL_COUNT; i++) {
        if (s_panel_cancel_wedge[i] != NULL) {
            ui_wedge_set_visible(s_panel_cancel_wedge[i], false);
        }
        if (s_panel_save_wedge[i] != NULL) {
            ui_wedge_set_visible(s_panel_save_wedge[i], false);
        }
    }
    for (size_t i = 0; i < s_overlay_obj_count; i++) {
        if (s_overlay_objs[i] != NULL) {
            lv_obj_add_flag(s_overlay_objs[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void settings_wedges_show_for_panel(settings_panel_t panel)
{
    settings_wedges_hide_all();

    if (panel == PANEL_HUB) {
        if (s_hub_cancel_wedge != NULL) {
            ui_wedge_set_visible(s_hub_cancel_wedge, true);
        }
        return;
    }

    if (panel == PANEL_NETWORK) {
        return;
    }

    if (s_panel_cancel_wedge[panel] != NULL) {
        ui_wedge_set_visible(s_panel_cancel_wedge[panel], true);
    }
    if (s_panel_save_wedge[panel] != NULL) {
        ui_wedge_set_visible(s_panel_save_wedge[panel], true);
    }
}

void ui_settings_attach_panel_wedges(lv_obj_t *panel, settings_panel_t panel_id,
                                     lv_event_cb_t save_cb)
{
    (void)panel;
    s_panel_cancel_wedge[panel_id] = ui_wedge_create_overlay(s_scr, UI_WEDGE_CANCEL);
    s_panel_save_wedge[panel_id] = ui_wedge_create_overlay(s_scr, UI_WEDGE_CONFIRM);

    if (s_panel_cancel_wedge[panel_id] != NULL) {
        ui_wedge_bind(s_panel_cancel_wedge[panel_id], UI_WEDGE_CANCEL, panel_cancel_cb,
                      (void *)(uintptr_t)panel_id);
        ui_wedge_set_visible(s_panel_cancel_wedge[panel_id], false);
    }
    if (s_panel_save_wedge[panel_id] != NULL) {
        ui_wedge_bind(s_panel_save_wedge[panel_id], UI_WEDGE_CONFIRM, save_cb, NULL);
        ui_wedge_set_visible(s_panel_save_wedge[panel_id], false);
    }
}

static void show_panel(settings_panel_t panel)
{
    for (int i = 0; i < PANEL_COUNT; i++) {
        if (s_panels[i] != NULL) {
            lv_obj_add_flag(s_panels[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (s_panels[panel] != NULL) {
        lv_obj_clear_flag(s_panels[panel], LV_OBJ_FLAG_HIDDEN);
    }

    settings_wedges_show_for_panel(panel);

    switch (panel) {
    case PANEL_HUB:
        ui_settings_refresh_web_ui_label();
        break;
    case PANEL_COLOURS:
        ui_settings_colours_sync_from_draft();
        break;
    case PANEL_NETWORK:
        ui_settings_network_sync_from_draft();
        break;
    case PANEL_TIMEZONE:
        ui_settings_timezone_select_from_draft();
        break;
    case PANEL_SCHEDULE:
        ui_settings_schedule_sync_from_draft();
        break;
    case PANEL_TIMEOUTS:
        ui_settings_timeouts_show_list();
        break;
    case PANEL_AA:
        ui_settings_adult_auth_sync_from_draft();
        break;
    default:
        break;
    }
}

void ui_settings_show_panel(settings_panel_t panel)
{
    show_panel(panel);
}

lv_obj_t *ui_settings_create_sub_panel(const char *title)
{
    lv_obj_t *panel = lv_obj_create(s_scr);
    lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    ui_settings_panel_layout(panel);
    lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN);

    ui_widgets_create_title(panel, title);
    return panel;
}

/* Panels have been split out into dedicated compilation units:
 * - ui_settings_colours.c
 * - ui_settings_network.c
 * - ui_settings_timezone.c
 * - ui_settings_schedule.c
 * - ui_settings_timeouts.c
 * - ui_settings_adult_auth.c
 */

/* ---------- Hub ---------- */

void ui_settings_idle_cb(void *user_data)
{
    (void)user_data;
    ui_nav_reset_idle_timer();
}

void ui_settings_refresh_web_ui_label(void)
{
    char ip[40];
    char url[64];
    char text[160];

    if (s_hub_web_lbl == NULL) {
        return;
    }

    if (app_network_get_device_ip(ip, sizeof(ip)) && app_network_get_web_ui_url(url, sizeof(url))) {
        if (app_network_setup_ap_active()) {
            snprintf(text, sizeof(text), "IP: %s\nWeb: %s\nJoin Wi-Fi: " APP_NETWORK_SETUP_AP_SSID, ip,
                     url);
        } else {
            snprintf(text, sizeof(text), "IP: %s\nWeb: %s", ip, url);
        }
        lv_label_set_text(s_hub_web_lbl, text);
        lv_obj_clear_flag(s_hub_web_lbl, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    if (app_network_get_web_ui_url(url, sizeof(url))) {
        if (app_network_setup_ap_active()) {
            snprintf(text, sizeof(text), "Web: %s\nJoin Wi-Fi: " APP_NETWORK_SETUP_AP_SSID, url);
        } else {
            snprintf(text, sizeof(text), "Web: %s", url);
        }
        lv_label_set_text(s_hub_web_lbl, text);
        lv_obj_clear_flag(s_hub_web_lbl, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    if (app_network_get_device_ip(ip, sizeof(ip))) {
        snprintf(text, sizeof(text), "IP: %s", ip);
        lv_label_set_text(s_hub_web_lbl, text);
        lv_obj_clear_flag(s_hub_web_lbl, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_obj_add_flag(s_hub_web_lbl, LV_OBJ_FLAG_HIDDEN);
}

static lv_obj_t *settings_create_web_ui_label(lv_obj_t *parent)
{
    const ui_theme_t *t = ui_theme_get();
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, "");
    lv_obj_set_style_text_color(lbl, t->secondary, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(lbl, 560);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 58);
    lv_obj_add_flag(lbl, LV_OBJ_FLAG_HIDDEN);
    return lbl;
}

static void build_hub_panel(void)
{
    int32_t cw = 0;
    int32_t ch = 0;

    s_panels[PANEL_HUB] = lv_obj_create(s_scr);
    lv_obj_set_style_bg_opa(s_panels[PANEL_HUB], LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_panels[PANEL_HUB], 0, 0);
    lv_obj_remove_flag(s_panels[PANEL_HUB], LV_OBJ_FLAG_SCROLLABLE);
    ui_settings_panel_layout(s_panels[PANEL_HUB]);

    ui_widgets_create_title(s_panels[PANEL_HUB], "Settings");
    s_hub_web_lbl = settings_create_web_ui_label(s_panels[PANEL_HUB]);

    ui_layout_get_content_size(s_scr, &cw, &ch);
    (void)cw;
    const int grid_w = HUB_BTN_COLS * HUB_BTN_W + (HUB_BTN_COLS - 1) * HUB_BTN_GAP_X;
    const int rows = 6 / HUB_BTN_COLS;
    const int total_h = rows * HUB_BTN_H + (rows - 1) * HUB_BTN_GAP_Y;
    const int y0 = (int)((ch - total_h) / 2) - 16;
    const int x0 = ui_layout_parent_center_x_wf(s_panels[PANEL_HUB], grid_w);

    static const char *labels[] = {
        "Colours",
        "Networking",
        "Timezone",
        "Schedule",
        "Timeouts",
        "Adult Auth",
    };

    for (int i = 0; i < 6; i++) {
        const int col = i % HUB_BTN_COLS;
        const int row = i / HUB_BTN_COLS;
        hub_create_btn(s_panels[PANEL_HUB], labels[i],
                       x0 + col * (HUB_BTN_W + HUB_BTN_GAP_X),
                       y0 + row * (HUB_BTN_H + HUB_BTN_GAP_Y),
                       (settings_panel_t)(PANEL_COLOURS + i));
    }

    s_hub_cancel_wedge = ui_wedge_create_overlay(s_scr, UI_WEDGE_CANCEL);
    if (s_hub_cancel_wedge != NULL) {
        ui_wedge_bind(s_hub_cancel_wedge, UI_WEDGE_CANCEL, hub_cancel_cb, NULL);
        ui_wedge_set_visible(s_hub_cancel_wedge, false);
    }
}

void ui_screen_settings_build(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    s_scr = ui_widgets_create_screen();
    screens[UI_SCREEN_SETTINGS] = s_scr;

    ui_keyboard_module_init();
    build_hub_panel();
    s_panels[PANEL_COLOURS] = ui_settings_colours_build();
    s_panels[PANEL_NETWORK] = ui_settings_network_build();
    s_panels[PANEL_TIMEZONE] = ui_settings_timezone_build();
    s_panels[PANEL_SCHEDULE] = ui_settings_schedule_build();
    s_panels[PANEL_TIMEOUTS] = ui_settings_timeouts_build();
    s_panels[PANEL_AA] = ui_settings_adult_auth_build();

    show_panel(PANEL_HUB);
}

void ui_screen_settings_on_show(void)
{
    app_config_t *cfg = app_config_get();

    memcpy(&s_saved, cfg, sizeof(s_saved));
    memcpy(&s_draft, cfg, sizeof(s_draft));

    ui_settings_colours_sync_from_draft();
    ui_settings_network_sync_from_draft();
    ui_settings_timezone_select_from_draft();
    ui_settings_schedule_sync_from_draft();
    ui_settings_timeouts_show_list();
    ui_settings_adult_auth_sync_from_draft();

    ui_settings_refresh_web_ui_label();
    show_panel(PANEL_HUB);
}

void ui_screen_settings_apply_theme(void)
{
    const ui_theme_t *t = ui_theme_get();
    (void)t;

    if (s_scr != NULL) {
        ui_widgets_style_circle_panel(s_scr);
    }
    if (s_hub_cancel_wedge != NULL) {
        ui_wedge_refresh_theme(s_hub_cancel_wedge);
    }
    for (int i = 0; i < PANEL_COUNT; i++) {
        if (s_panel_cancel_wedge[i] != NULL) {
            ui_wedge_refresh_theme(s_panel_cancel_wedge[i]);
        }
        if (s_panel_save_wedge[i] != NULL) {
            ui_wedge_refresh_theme(s_panel_save_wedge[i]);
        }
    }
    ui_settings_colours_apply_theme();
    ui_settings_network_apply_theme();
    ui_settings_timezone_apply_theme();
}
