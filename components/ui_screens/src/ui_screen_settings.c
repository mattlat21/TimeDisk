/**
 * @file ui_screen_settings.c
 * @brief Settings hub and editable sub-panels (draft / saved model).
 */

#include "ui_screens_registry.h"
#include "ui_layout.h"
#include "ui_widgets.h"
#include "ui_format.h"
#include "ui_duration_editor.h"
#include "ui_wedge.h"
#include "ui_theme.h"
#include "ui_keyboard.h"
#include "ui_nav.h"
#include "app_config.h"
#include "app_time.h"
#include "timezone_catalog.h"
#include <stdio.h>
#include <string.h>

#define HUB_BTN_W           560
#define HUB_BTN_H           80
#define HUB_BTN_GAP         12
#define HUB_BTN_RADIUS      24

typedef enum {
    PANEL_HUB = 0,
    PANEL_COLOURS,
    PANEL_NETWORK,
    PANEL_TIMEZONE,
    PANEL_SCHEDULE,
    PANEL_TIMEOUTS,
    PANEL_AA,
    PANEL_COUNT,
} settings_panel_t;

typedef enum {
    TIMEOUT_VIEW_LIST = 0,
    TIMEOUT_VIEW_EDIT,
} timeout_view_t;

typedef enum {
    NET_FIELD_SSID = 0,
    NET_FIELD_PASSWORD,
    NET_FIELD_NTP,
} net_field_t;

/* --- Colours (theme swatches) --- */
#define THEME_SWATCH_SIZE     56
#define THEME_SWATCH_GAP      12
#define THEME_SWATCH_COUNT    8
#define THEME_SWATCH_BLOCK_CY 360
#define THEME_ROW_SPACING     80
#define THEME_ROW_PRIMARY_Y   (THEME_SWATCH_BLOCK_CY - THEME_ROW_SPACING / 2 - THEME_SWATCH_SIZE / 2)
#define THEME_ROW_SECONDARY_Y (THEME_SWATCH_BLOCK_CY + THEME_ROW_SPACING / 2 - THEME_SWATCH_SIZE / 2)

static const uint32_t s_palette[THEME_SWATCH_COUNT] = {
    0x7A24BC, 0x6BCA24, 0x5A189A, 0x1D4ED8,
    0xE87A2E, 0x3CB85C, 0x0F5BF9, 0xFFFFFF,
};

typedef enum {
    THEME_SLOT_PRIMARY = 0,
    THEME_SLOT_SECONDARY,
} theme_slot_t;

/* --- Timezone --- */
#define TZ_DROPDOWN_W       480
#define TZ_DROPDOWN_H       52
#define TZ_COUNTRY_Y        140
#define TZ_LOCATION_Y       210
#define TZ_OPTIONS_BUF_LEN  512

/* --- Network (list + per-field edit, wizard-style keyboard) --- */
#define NET_FIELD_X_WF      78
#define NET_FIELD_Y_WF      210
#define NET_FIELD_W         563
#define NET_FIELD_H         78
#define NET_FIELD_RADIUS    20
#define NET_EDIT_TITLE_Y    28
#define NET_BACKUP_LEN        APP_NTP_SERVER_MAX

static lv_obj_t *s_scr;
static lv_obj_t *s_panels[PANEL_COUNT];
static lv_obj_t *s_hub_cancel_wedge;
static lv_obj_t *s_panel_cancel_wedge[PANEL_COUNT];
static lv_obj_t *s_panel_save_wedge[PANEL_COUNT];
static app_config_t s_saved;
static app_config_t s_draft;

/* Colours */
static lv_obj_t *s_swatch_btns[2][THEME_SWATCH_COUNT];
static theme_slot_t s_active_slot = THEME_SLOT_PRIMARY;
static uint32_t s_primary_rgb;
static uint32_t s_secondary_rgb;

/* Network */
static lv_obj_t *s_net_panel_title;
static lv_obj_t *s_net_list;
static lv_obj_t *s_net_edit;
static lv_obj_t *s_net_edit_title;
static lv_obj_t *s_net_edit_field_box;
static lv_obj_t *s_net_edit_lbl;
static lv_obj_t *s_net_panel_cancel;
static lv_obj_t *s_net_panel_save;
static lv_obj_t *s_net_edit_cancel;
static lv_obj_t *s_net_edit_save;
static char s_ssid_buf[APP_WIFI_SSID_MAX];
static char s_pw_buf[APP_WIFI_PASSWORD_MAX];
static char s_ntp_buf[APP_NTP_SERVER_MAX];
static char s_net_edit_backup[NET_BACKUP_LEN];
static ui_keyboard_t *s_net_kb;
static net_field_t s_net_active = NET_FIELD_SSID;

/* Timezone */
static lv_obj_t *s_dd_country;
static lv_obj_t *s_dd_location;
static lv_obj_t *s_lbl_tz_preview;
static char s_country_opts[TZ_OPTIONS_BUF_LEN];
static char s_location_opts[TZ_OPTIONS_BUF_LEN];
static size_t s_country_idx;
static size_t s_location_idx;

/* Schedule */
static ui_duration_editor_bundle_t s_sched_bundles[3];
static uint32_t s_sched_vals[3];
static const char *s_sched_labels[3] = {"Wind down", "Sleep", "Rest"};

/* Timeouts */
static timeout_view_t s_timeout_view;
static int s_timeout_edit_idx;
static lv_obj_t *s_timeout_list;
static lv_obj_t *s_timeout_edit;
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

/* Adult auth */
static lv_obj_t *s_chk_pin;
static lv_obj_t *s_chk_maths;
static lv_obj_t *s_lbl_aa_pin;
static char s_aa_pin_buf[APP_AA_PIN_LEN];

static void show_panel(settings_panel_t panel);
static void settings_panel_layout(lv_obj_t *panel);
static void settings_wedges_hide_all(void);
static void settings_wedges_show_for_panel(settings_panel_t panel);
static void settings_idle_cb(void *user_data);
static void colours_sync_from_draft(void);
static void colours_apply_preview(void);
static void network_sync_from_draft(void);
static void network_show_list(void);
static void network_show_edit(net_field_t field);
static void tz_select_from_draft(void);
static void schedule_sync_from_draft(void);
static void timeout_show_list(void);
static void aa_sync_from_draft(void);

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
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_22, 0);
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
        s_primary_rgb = s_draft.ui_primary_color;
        s_secondary_rgb = s_draft.ui_secondary_color;
        break;
    case PANEL_NETWORK:
        memcpy(s_draft.wifi_ssid, s_saved.wifi_ssid, sizeof(s_draft.wifi_ssid));
        memcpy(s_draft.wifi_password, s_saved.wifi_password, sizeof(s_draft.wifi_password));
        s_draft.wifi_password_set = s_saved.wifi_password_set;
        memcpy(s_draft.ntp_server, s_saved.ntp_server, sizeof(s_draft.ntp_server));
        network_show_list();
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
        s_draft.timeout_splash_sec = s_saved.timeout_splash_sec;
        s_draft.timeout_tod_dim_sec = s_saved.timeout_tod_dim_sec;
        s_draft.timeout_aa_sec = s_saved.timeout_aa_sec;
        s_draft.timeout_main_menu_sec = s_saved.timeout_main_menu_sec;
        s_draft.timeout_timer_dim_sec = s_saved.timeout_timer_dim_sec;
        timeout_show_list();
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
        colours_sync_from_draft();
        ui_theme_apply();
    } else if (panel == PANEL_TIMEZONE && s_saved.timezone_set) {
        app_time_apply_timezone_id(s_saved.timezone_id);
    }

    show_panel(PANEL_HUB);
}

static void settings_panel_layout(lv_obj_t *panel)
{
    int32_t cw = 0;
    int32_t ch = 0;

    ui_layout_get_content_size(s_scr, &cw, &ch);
    lv_obj_set_size(panel, cw, ch);
    lv_obj_align(panel, LV_ALIGN_TOP_LEFT, 0, 0);
}

static void settings_wedges_hide_all(void)
{
    if (s_hub_cancel_wedge != NULL) {
        lv_obj_add_flag(s_hub_cancel_wedge, LV_OBJ_FLAG_HIDDEN);
    }
    for (int i = 0; i < PANEL_COUNT; i++) {
        if (s_panel_cancel_wedge[i] != NULL) {
            lv_obj_add_flag(s_panel_cancel_wedge[i], LV_OBJ_FLAG_HIDDEN);
        }
        if (s_panel_save_wedge[i] != NULL) {
            lv_obj_add_flag(s_panel_save_wedge[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (s_net_panel_cancel != NULL) {
        lv_obj_add_flag(s_net_panel_cancel, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_net_panel_save != NULL) {
        lv_obj_add_flag(s_net_panel_save, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_net_edit_cancel != NULL) {
        lv_obj_add_flag(s_net_edit_cancel, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_net_edit_save != NULL) {
        lv_obj_add_flag(s_net_edit_save, LV_OBJ_FLAG_HIDDEN);
    }
}

static void settings_wedges_show_for_panel(settings_panel_t panel)
{
    settings_wedges_hide_all();

    if (panel == PANEL_HUB) {
        if (s_hub_cancel_wedge != NULL) {
            lv_obj_clear_flag(s_hub_cancel_wedge, LV_OBJ_FLAG_HIDDEN);
            lv_obj_move_foreground(s_hub_cancel_wedge);
        }
        return;
    }

    if (panel == PANEL_NETWORK) {
        return;
    }

    if (s_panel_cancel_wedge[panel] != NULL) {
        lv_obj_clear_flag(s_panel_cancel_wedge[panel], LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_panel_cancel_wedge[panel]);
    }
    if (s_panel_save_wedge[panel] != NULL) {
        lv_obj_clear_flag(s_panel_save_wedge[panel], LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_panel_save_wedge[panel]);
    }
}

static void attach_panel_wedges(lv_obj_t *panel, settings_panel_t panel_id,
                                lv_event_cb_t save_cb)
{
    (void)panel;
    lv_obj_t *cancel = ui_wedge_create(s_scr, UI_WEDGE_CANCEL);
    lv_obj_add_event_cb(cancel, panel_cancel_cb, LV_EVENT_CLICKED,
                        (void *)(uintptr_t)panel_id);

    lv_obj_t *save = ui_wedge_create(s_scr, UI_WEDGE_CONFIRM);
    lv_obj_add_event_cb(save, save_cb, LV_EVENT_CLICKED, NULL);

    s_panel_cancel_wedge[panel_id] = cancel;
    s_panel_save_wedge[panel_id] = save;
    lv_obj_add_flag(cancel, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(save, LV_OBJ_FLAG_HIDDEN);
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
    case PANEL_COLOURS:
        colours_sync_from_draft();
        break;
    case PANEL_NETWORK:
        network_sync_from_draft();
        network_show_list();
        break;
    case PANEL_TIMEZONE:
        tz_select_from_draft();
        break;
    case PANEL_SCHEDULE:
        schedule_sync_from_draft();
        break;
    case PANEL_TIMEOUTS:
        timeout_show_list();
        break;
    case PANEL_AA:
        aa_sync_from_draft();
        break;
    default:
        break;
    }
}

static lv_obj_t *create_sub_panel(const char *title)
{
    lv_obj_t *panel = lv_obj_create(s_scr);
    lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(panel, 0, 0);
    lv_obj_remove_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    settings_panel_layout(panel);
    lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN);

    ui_widgets_create_title(panel, title);
    return panel;
}

/* ---------- Colours ---------- */

static int palette_index_for_rgb(uint32_t rgb)
{
    for (int i = 0; i < THEME_SWATCH_COUNT; i++) {
        if (s_palette[i] == rgb) {
            return i;
        }
    }
    return -1;
}

static void colours_refresh_swatch_highlights(void)
{
    int pri_idx = palette_index_for_rgb(s_primary_rgb);
    int sec_idx = palette_index_for_rgb(s_secondary_rgb);

    for (int i = 0; i < THEME_SWATCH_COUNT; i++) {
        if (s_swatch_btns[THEME_SLOT_PRIMARY][i] != NULL) {
            lv_obj_set_style_border_width(s_swatch_btns[THEME_SLOT_PRIMARY][i],
                                          (i == pri_idx) ? 3 : 0, 0);
            lv_obj_set_style_border_color(s_swatch_btns[THEME_SLOT_PRIMARY][i],
                                          lv_color_white(), 0);
        }
        if (s_swatch_btns[THEME_SLOT_SECONDARY][i] != NULL) {
            lv_obj_set_style_border_width(s_swatch_btns[THEME_SLOT_SECONDARY][i],
                                          (i == sec_idx) ? 3 : 0, 0);
            lv_obj_set_style_border_color(s_swatch_btns[THEME_SLOT_SECONDARY][i],
                                          lv_color_white(), 0);
        }
    }
}

static void colours_swatch_cb(lv_event_t *e)
{
    int slot = (int)(uintptr_t)lv_event_get_user_data(e);
    lv_obj_t *btn = lv_event_get_target(e);
    uint32_t rgb = (uint32_t)(uintptr_t)lv_obj_get_user_data(btn);

    s_active_slot = (theme_slot_t)slot;
    if (slot == THEME_SLOT_PRIMARY) {
        s_primary_rgb = rgb;
    } else {
        s_secondary_rgb = rgb;
    }
    colours_refresh_swatch_highlights();
    colours_apply_preview();
    ui_nav_reset_idle_timer();
}

static void colours_build_swatch_row(lv_obj_t *parent, theme_slot_t slot, int row_y)
{
    const int row_w = THEME_SWATCH_COUNT * THEME_SWATCH_SIZE
                      + (THEME_SWATCH_COUNT - 1) * THEME_SWATCH_GAP;
    int x = (UI_DISP - row_w) / 2;

    for (int i = 0; i < THEME_SWATCH_COUNT; i++) {
        lv_obj_t *btn = lv_button_create(parent);
        lv_obj_set_size(btn, THEME_SWATCH_SIZE, THEME_SWATCH_SIZE);
        lv_obj_set_pos(btn, x, row_y);
        lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(s_palette[i]), 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_user_data(btn, (void *)(uintptr_t)s_palette[i]);
        lv_obj_add_event_cb(btn, colours_swatch_cb, LV_EVENT_CLICKED,
                            (void *)(uintptr_t)slot);
        s_swatch_btns[slot][i] = btn;
        x += THEME_SWATCH_SIZE + THEME_SWATCH_GAP;
    }
}

static void colours_sync_from_draft(void)
{
    s_primary_rgb = s_draft.ui_primary_color;
    s_secondary_rgb = s_draft.ui_secondary_color;
    colours_refresh_swatch_highlights();
}

static void colours_apply_preview(void)
{
    app_config_t *cfg = app_config_get();
    cfg->ui_primary_color = s_primary_rgb;
    cfg->ui_secondary_color = s_secondary_rgb;
    ui_theme_apply();
}

static void colours_save_cb(lv_event_t *e)
{
    (void)e;
    app_config_t *cfg = app_config_get();

    s_draft.ui_primary_color = s_primary_rgb;
    s_draft.ui_secondary_color = s_secondary_rgb;
    s_draft.theme_set = true;
    cfg->ui_primary_color = s_draft.ui_primary_color;
    cfg->ui_secondary_color = s_draft.ui_secondary_color;
    cfg->theme_set = true;

    s_saved.ui_primary_color = s_draft.ui_primary_color;
    s_saved.ui_secondary_color = s_draft.ui_secondary_color;
    s_saved.theme_set = true;

    app_config_save_theme();
    ui_theme_apply();
    show_panel(PANEL_HUB);
}

static void build_colours_panel(void)
{
    s_panels[PANEL_COLOURS] = create_sub_panel("Colours");
    colours_build_swatch_row(s_panels[PANEL_COLOURS], THEME_SLOT_PRIMARY, THEME_ROW_PRIMARY_Y);
    colours_build_swatch_row(s_panels[PANEL_COLOURS], THEME_SLOT_SECONDARY, THEME_ROW_SECONDARY_Y);
    attach_panel_wedges(s_panels[PANEL_COLOURS], PANEL_COLOURS, colours_save_cb);
}

/* ---------- Network ---------- */

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

static void net_edit_refresh_label(void)
{
    if (s_net_edit_lbl == NULL) {
        return;
    }
    if (s_net_active == NET_FIELD_PASSWORD && s_pw_buf[0] == '\0'
        && s_draft.wifi_password_set && s_draft.wifi_password[0] != '\0') {
        lv_label_set_text(s_net_edit_lbl, "********");
    } else {
        const char *text = s_ssid_buf;
        if (s_net_active == NET_FIELD_PASSWORD) {
            text = s_pw_buf;
        } else if (s_net_active == NET_FIELD_NTP) {
            text = s_ntp_buf;
        }
        lv_label_set_text(s_net_edit_lbl, text);
    }
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

    if (s_net_kb != NULL) {
        ui_keyboard_destroy(s_net_kb);
    }

    ui_keyboard_config_t kb_cfg = {
        .buf = buf,
        .buf_len = buf_len,
        .label = s_net_edit_lbl,
        .initial_mode = UI_KEYBOARD_MODE_LOWER,
        .on_activity = settings_idle_cb,
        .user_data = NULL,
    };
    s_net_kb = ui_keyboard_create(s_scr, &kb_cfg);
}

static void network_show_list(void)
{
    if (s_net_list != NULL) {
        lv_obj_clear_flag(s_net_list, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_net_edit != NULL) {
        lv_obj_add_flag(s_net_edit, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_net_panel_cancel != NULL) {
        lv_obj_clear_flag(s_net_panel_cancel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_net_panel_cancel);
    }
    if (s_net_panel_save != NULL) {
        lv_obj_clear_flag(s_net_panel_save, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(s_net_panel_save);
    }
    if (s_net_edit_cancel != NULL) {
        lv_obj_add_flag(s_net_edit_cancel, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_net_edit_save != NULL) {
        lv_obj_add_flag(s_net_edit_save, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_net_panel_title != NULL) {
        lv_obj_clear_flag(s_net_panel_title, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_net_kb != NULL) {
        ui_keyboard_destroy(s_net_kb);
        s_net_kb = NULL;
    }
}

static void network_load_field_buf(net_field_t field)
{
    switch (field) {
    case NET_FIELD_SSID:
        snprintf(s_ssid_buf, sizeof(s_ssid_buf), "%s", s_draft.wifi_ssid);
        break;
    case NET_FIELD_PASSWORD:
        s_pw_buf[0] = '\0';
        break;
    case NET_FIELD_NTP:
        snprintf(s_ntp_buf, sizeof(s_ntp_buf), "%s", s_draft.ntp_server);
        break;
    default:
        break;
    }
}

static void network_show_edit(net_field_t field)
{
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
    }
    net_edit_refresh_label();

    if (s_net_list != NULL) {
        lv_obj_add_flag(s_net_list, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_net_edit != NULL) {
        lv_obj_clear_flag(s_net_edit, LV_OBJ_FLAG_HIDDEN);
    }
    net_bind_keyboard();
    if (s_net_panel_cancel != NULL) {
        lv_obj_add_flag(s_net_panel_cancel, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_net_panel_save != NULL) {
        lv_obj_add_flag(s_net_panel_save, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_net_edit_cancel != NULL) {
        lv_obj_clear_flag(s_net_edit_cancel, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_net_edit_save != NULL) {
        lv_obj_clear_flag(s_net_edit_save, LV_OBJ_FLAG_HIDDEN);
    }
    if (s_net_edit_field_box != NULL) {
        lv_obj_move_foreground(s_net_edit_field_box);
    }
    if (s_net_edit_title != NULL) {
        lv_obj_move_foreground(s_net_edit_title);
    }
    if (s_net_edit_cancel != NULL) {
        lv_obj_move_foreground(s_net_edit_cancel);
    }
    if (s_net_edit_save != NULL) {
        lv_obj_move_foreground(s_net_edit_save);
    }
    if (s_net_panel_title != NULL) {
        lv_obj_add_flag(s_net_panel_title, LV_OBJ_FLAG_HIDDEN);
    }
    ui_nav_reset_idle_timer();
}

static void net_row_cb(lv_event_t *e)
{
    net_field_t field = (net_field_t)(uintptr_t)lv_event_get_user_data(e);
    network_show_edit(field);
}

static void net_edit_cancel_cb(lv_event_t *e)
{
    (void)e;
    if (s_net_active == NET_FIELD_SSID) {
        snprintf(s_ssid_buf, sizeof(s_ssid_buf), "%s", s_net_edit_backup);
    } else if (s_net_active == NET_FIELD_PASSWORD) {
        s_pw_buf[0] = '\0';
    } else {
        snprintf(s_ntp_buf, sizeof(s_ntp_buf), "%s", s_net_edit_backup);
    }
    network_show_list();
}

static void net_edit_save_cb(lv_event_t *e)
{
    (void)e;
    app_config_t *cfg = app_config_get();

    if (s_net_active == NET_FIELD_SSID) {
        if (s_ssid_buf[0] == '\0') {
            return;
        }
        snprintf(s_draft.wifi_ssid, sizeof(s_draft.wifi_ssid), "%s", s_ssid_buf);
    } else if (s_net_active == NET_FIELD_PASSWORD) {
        if (s_pw_buf[0] != '\0') {
            snprintf(s_draft.wifi_password, sizeof(s_draft.wifi_password), "%s", s_pw_buf);
            s_draft.wifi_password_set = true;
        }
    } else {
        if (s_ntp_buf[0] == '\0') {
            return;
        }
        snprintf(s_draft.ntp_server, sizeof(s_draft.ntp_server), "%s", s_ntp_buf);
    }

    memcpy(cfg, &s_draft, sizeof(*cfg));
    memcpy(&s_saved, &s_draft, sizeof(s_saved));
    app_config_save_network();
    network_show_list();
}

static void network_sync_from_draft(void)
{
    snprintf(s_ssid_buf, sizeof(s_ssid_buf), "%s", s_draft.wifi_ssid);
    s_pw_buf[0] = '\0';
    snprintf(s_ntp_buf, sizeof(s_ntp_buf), "%s", s_draft.ntp_server);
}

static void network_save_cb(lv_event_t *e)
{
    (void)e;
    app_config_t *cfg = app_config_get();

    memcpy(cfg, &s_draft, sizeof(*cfg));
    memcpy(&s_saved, &s_draft, sizeof(s_saved));
    app_config_save_network();
    show_panel(PANEL_HUB);
}

static lv_obj_t *net_create_edit_field(lv_obj_t *parent, lv_obj_t **lbl_out)
{
    const ui_theme_t *t = ui_theme_get();
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_set_size(box, NET_FIELD_W, NET_FIELD_H);
    lv_obj_set_pos(box, UI_WF_X(NET_FIELD_X_WF, UI_RING_BORDER),
                   UI_WF_Y(NET_FIELD_Y_WF, UI_RING_BORDER));
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

static void build_network_panel(void)
{
    const ui_theme_t *t = ui_theme_get();
    static const char *row_labels[] = {"Wi-Fi Name", "Wi-Fi Password", "NTP"};

    s_panels[PANEL_NETWORK] = lv_obj_create(s_scr);
    lv_obj_set_style_bg_opa(s_panels[PANEL_NETWORK], LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_panels[PANEL_NETWORK], 0, 0);
    lv_obj_remove_flag(s_panels[PANEL_NETWORK], LV_OBJ_FLAG_SCROLLABLE);
    settings_panel_layout(s_panels[PANEL_NETWORK]);
    lv_obj_add_flag(s_panels[PANEL_NETWORK], LV_OBJ_FLAG_HIDDEN);
    s_net_panel_title = ui_widgets_create_title(s_panels[PANEL_NETWORK], "Networking");

    s_net_list = lv_obj_create(s_panels[PANEL_NETWORK]);
    lv_obj_set_size(s_net_list, UI_DISP, UI_DISP);
    lv_obj_set_style_bg_opa(s_net_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_net_list, 0, 0);
    lv_obj_remove_flag(s_net_list, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(s_net_list);

    for (int i = 0; i < 3; i++) {
        lv_obj_t *btn = lv_button_create(s_net_list);
        lv_obj_set_size(btn, HUB_BTN_W, HUB_BTN_H);
        lv_obj_set_pos(btn, (UI_DISP - HUB_BTN_W) / 2, 180 + i * (HUB_BTN_H + HUB_BTN_GAP));
        lv_obj_set_style_radius(btn, HUB_BTN_RADIUS, 0);
        lv_obj_set_style_bg_color(btn, t->menu_petal, 0);
        lv_obj_set_style_border_width(btn, 0, 0);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, row_labels[i]);
        lv_obj_set_style_text_color(lbl, t->white, 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_22, 0);
        lv_obj_center(lbl);
        lv_obj_add_event_cb(btn, net_row_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
    }

    s_net_panel_cancel = ui_wedge_create(s_scr, UI_WEDGE_CANCEL);
    lv_obj_add_event_cb(s_net_panel_cancel, panel_cancel_cb, LV_EVENT_CLICKED,
                        (void *)(uintptr_t)PANEL_NETWORK);
    lv_obj_add_flag(s_net_panel_cancel, LV_OBJ_FLAG_HIDDEN);

    s_net_panel_save = ui_wedge_create(s_scr, UI_WEDGE_CONFIRM);
    lv_obj_add_event_cb(s_net_panel_save, network_save_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(s_net_panel_save, LV_OBJ_FLAG_HIDDEN);

    s_net_edit = lv_obj_create(s_panels[PANEL_NETWORK]);
    lv_obj_set_size(s_net_edit, UI_DISP, UI_DISP);
    lv_obj_set_style_bg_opa(s_net_edit, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_net_edit, 0, 0);
    lv_obj_remove_flag(s_net_edit, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(s_net_edit);
    lv_obj_add_flag(s_net_edit, LV_OBJ_FLAG_HIDDEN);

    s_net_edit_title = ui_widgets_create_title(s_net_edit, "Wi-Fi Name");
    lv_obj_align(s_net_edit_title, LV_ALIGN_TOP_MID, 0, NET_EDIT_TITLE_Y);

    s_net_edit_field_box = net_create_edit_field(s_net_edit, &s_net_edit_lbl);

    s_net_edit_cancel = ui_wedge_create(s_scr, UI_WEDGE_CANCEL);
    lv_obj_add_event_cb(s_net_edit_cancel, net_edit_cancel_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(s_net_edit_cancel, LV_OBJ_FLAG_HIDDEN);

    s_net_edit_save = ui_wedge_create(s_scr, UI_WEDGE_CONFIRM);
    lv_obj_add_event_cb(s_net_edit_save, net_edit_save_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(s_net_edit_save, LV_OBJ_FLAG_HIDDEN);
}

/* ---------- Timezone ---------- */

static void tz_refresh_preview(void)
{
    if (s_lbl_tz_preview == NULL) {
        return;
    }
    app_runtime_t *rt = app_runtime_get();
    char tbuf[20];
    if (!rt->time_valid) {
        snprintf(tbuf, sizeof(tbuf), "--:--");
    } else {
        ui_format_hh_mm_ampm_now(tbuf, sizeof(tbuf));
    }
    lv_label_set_text(s_lbl_tz_preview, tbuf);
}

static void tz_apply_preview(void)
{
    const char *tz_id = timezone_catalog_timezone_id(s_country_idx, s_location_idx);
    if (tz_id[0] != '\0') {
        app_time_apply_timezone_id(tz_id);
    }
    tz_refresh_preview();
}

static void tz_sync_location_dropdown(size_t country_idx, size_t location_idx)
{
    if (timezone_catalog_format_location_options(country_idx, s_location_opts,
                                                 sizeof(s_location_opts)) == 0) {
        s_location_opts[0] = '\0';
    }
    lv_dropdown_set_options(s_dd_location, s_location_opts);
    if (location_idx >= timezone_catalog_location_count(country_idx)) {
        location_idx = 0;
    }
    lv_dropdown_set_selected(s_dd_location, location_idx);
    s_location_idx = location_idx;
}

static void tz_select_from_draft(void)
{
    size_t ci = 0;
    size_t li = 0;
    if (s_draft.timezone_id[0] != '\0'
        && timezone_catalog_find_by_id(s_draft.timezone_id, &ci, &li)) {
        s_country_idx = ci;
        s_location_idx = li;
    } else if (!timezone_catalog_default_selection(&ci, &li)) {
        ci = 0;
        li = 0;
    } else {
        s_country_idx = ci;
        s_location_idx = li;
    }
    lv_dropdown_set_selected(s_dd_country, s_country_idx);
    tz_sync_location_dropdown(s_country_idx, s_location_idx);
    tz_apply_preview();
}

static void tz_country_changed_cb(lv_event_t *e)
{
    (void)e;
    s_country_idx = lv_dropdown_get_selected(s_dd_country);
    tz_sync_location_dropdown(s_country_idx, 0);
    tz_apply_preview();
    ui_nav_reset_idle_timer();
}

static void tz_location_changed_cb(lv_event_t *e)
{
    (void)e;
    s_location_idx = lv_dropdown_get_selected(s_dd_location);
    tz_apply_preview();
    ui_nav_reset_idle_timer();
}

static lv_obj_t *tz_create_dropdown(lv_obj_t *parent, int y)
{
    const ui_theme_t *th = ui_theme_get();
    lv_obj_t *dd = lv_dropdown_create(parent);
    lv_obj_set_size(dd, TZ_DROPDOWN_W, TZ_DROPDOWN_H);
    lv_obj_set_pos(dd, (UI_DISP - TZ_DROPDOWN_W) / 2, y);
    lv_obj_set_style_bg_color(dd, th->ring, 0);
    lv_obj_set_style_text_color(dd, th->white, 0);
    lv_obj_set_style_border_width(dd, 0, 0);
    lv_obj_set_style_pad_all(dd, 10, 0);
    lv_obj_set_style_text_font(dd, &lv_font_montserrat_20, 0);
    return dd;
}

static void timezone_save_cb(lv_event_t *e)
{
    (void)e;
    const char *tz_id = timezone_catalog_timezone_id(s_country_idx, s_location_idx);
    if (tz_id[0] == '\0') {
        return;
    }

    app_config_t *cfg = app_config_get();
    snprintf(s_draft.timezone_id, sizeof(s_draft.timezone_id), "%s", tz_id);
    s_draft.timezone_set = true;
    cfg->timezone_set = true;
    snprintf(cfg->timezone_id, sizeof(cfg->timezone_id), "%s", tz_id);
    memcpy(s_saved.timezone_id, s_draft.timezone_id, sizeof(s_saved.timezone_id));
    s_saved.timezone_set = true;

    app_time_apply_timezone_id(tz_id);
    app_config_save_timezone();
    show_panel(PANEL_HUB);
}

static void build_timezone_panel(void)
{
    const ui_theme_t *th = ui_theme_get();

    s_panels[PANEL_TIMEZONE] = create_sub_panel("Timezone");

    s_dd_country = tz_create_dropdown(s_panels[PANEL_TIMEZONE], TZ_COUNTRY_Y);
    s_dd_location = tz_create_dropdown(s_panels[PANEL_TIMEZONE], TZ_LOCATION_Y);

    if (timezone_catalog_format_country_options(s_country_opts, sizeof(s_country_opts)) == 0) {
        s_country_opts[0] = '\0';
    }
    lv_dropdown_set_options(s_dd_country, s_country_opts);
    lv_obj_add_event_cb(s_dd_country, tz_country_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(s_dd_location, tz_location_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);

    s_lbl_tz_preview = lv_label_create(s_panels[PANEL_TIMEZONE]);
    lv_label_set_text(s_lbl_tz_preview, "--:--");
    lv_obj_set_style_text_color(s_lbl_tz_preview, th->white, 0);
    lv_obj_set_style_text_font(s_lbl_tz_preview, &lv_font_montserrat_48, 0);
    lv_obj_align(s_lbl_tz_preview, LV_ALIGN_CENTER, 0, 60);

    attach_panel_wedges(s_panels[PANEL_TIMEZONE], PANEL_TIMEZONE, timezone_save_cb);
}

/* ---------- Schedule ---------- */

static void schedule_sync_from_draft(void)
{
    s_sched_vals[0] = s_draft.wind_down_sec;
    s_sched_vals[1] = s_draft.sleep_sec;
    s_sched_vals[2] = s_draft.rest_sec;
    for (int i = 0; i < 3; i++) {
        ui_duration_editor_refresh(&s_sched_bundles[i].editor, &s_sched_bundles[i].cfg);
    }
}

static void schedule_save_cb(lv_event_t *e)
{
    (void)e;
    app_config_t *cfg = app_config_get();

    s_draft.wind_down_sec = s_sched_vals[0];
    s_draft.sleep_sec = s_sched_vals[1];
    s_draft.rest_sec = s_sched_vals[2];
    cfg->wind_down_sec = s_draft.wind_down_sec;
    cfg->sleep_sec = s_draft.sleep_sec;
    cfg->rest_sec = s_draft.rest_sec;
    s_saved.wind_down_sec = s_draft.wind_down_sec;
    s_saved.sleep_sec = s_draft.sleep_sec;
    s_saved.rest_sec = s_draft.rest_sec;

    app_config_save_schedule();
    show_panel(PANEL_HUB);
}

static void build_schedule_panel(void)
{
    const ui_theme_t *t = ui_theme_get();
    static const int row_y[3] = {120, 260, 400};
    static const int box_h = 72;

    s_panels[PANEL_SCHEDULE] = create_sub_panel("Schedule");

    for (int i = 0; i < 3; i++) {
        lv_obj_t *lbl = lv_label_create(s_panels[PANEL_SCHEDULE]);
        lv_label_set_text(lbl, s_sched_labels[i]);
        lv_obj_set_style_text_color(lbl, t->white, 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0);
        lv_obj_set_pos(lbl, 160, row_y[i] - 24);

        s_sched_bundles[i].cfg = (ui_duration_editor_cfg_t){
            .value_sec = &s_sched_vals[i],
            .box_x = 160,
            .box_y = row_y[i],
            .box_w = 400,
            .box_h = box_h,
            .show_end_time = false,
            .on_change = settings_idle_cb,
        };
        ui_duration_editor_create(s_panels[PANEL_SCHEDULE], &s_sched_bundles[i]);
    }

    attach_panel_wedges(s_panels[PANEL_SCHEDULE], PANEL_SCHEDULE, schedule_save_cb);
}

/* ---------- Timeouts ---------- */

static uint32_t *timeout_field_ptr(int idx)
{
    switch (idx) {
    case 0:
        return &s_draft.timeout_splash_sec;
    case 1:
        return &s_draft.timeout_tod_dim_sec;
    case 2:
        return &s_draft.timeout_aa_sec;
    case 3:
        return &s_draft.timeout_main_menu_sec;
    case 4:
        return &s_draft.timeout_timer_dim_sec;
    default:
        return &s_draft.timeout_splash_sec;
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

static void timeout_show_list(void)
{
    s_timeout_view = TIMEOUT_VIEW_LIST;
    lv_obj_clear_flag(s_timeout_list, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s_timeout_edit, LV_OBJ_FLAG_HIDDEN);
    timeout_refresh_list_labels();
}

static void timeout_row_cb(lv_event_t *e)
{
    int idx = (int)(uintptr_t)lv_event_get_user_data(e);
    s_timeout_edit_idx = idx;
    s_timeout_edit_val = *timeout_field_ptr(idx);
    s_timeout_view = TIMEOUT_VIEW_EDIT;

    lv_obj_add_flag(s_timeout_list, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s_timeout_edit, LV_OBJ_FLAG_HIDDEN);
    ui_duration_editor_refresh(&s_timeout_bundle.editor, &s_timeout_bundle.cfg);
    ui_nav_reset_idle_timer();
}

static void timeout_edit_back_cb(lv_event_t *e)
{
    (void)e;
    *timeout_field_ptr(s_timeout_edit_idx) = s_timeout_edit_val;
    timeout_show_list();
}

static void timeouts_save_cb(lv_event_t *e)
{
    (void)e;
    app_config_t *cfg = app_config_get();

    memcpy(cfg, &s_draft, sizeof(*cfg));
    s_saved.timeout_splash_sec = s_draft.timeout_splash_sec;
    s_saved.timeout_tod_dim_sec = s_draft.timeout_tod_dim_sec;
    s_saved.timeout_aa_sec = s_draft.timeout_aa_sec;
    s_saved.timeout_main_menu_sec = s_draft.timeout_main_menu_sec;
    s_saved.timeout_timer_dim_sec = s_draft.timeout_timer_dim_sec;

    app_config_save_timeouts();
    timeout_show_list();
    show_panel(PANEL_HUB);
}

static void build_timeouts_panel(void)
{
    const ui_theme_t *t = ui_theme_get();

    s_panels[PANEL_TIMEOUTS] = create_sub_panel("Timeouts");

    s_timeout_list = lv_obj_create(s_panels[PANEL_TIMEOUTS]);
    lv_obj_set_size(s_timeout_list, UI_DISP, UI_DISP);
    lv_obj_set_style_bg_opa(s_timeout_list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_timeout_list, 0, 0);
    lv_obj_remove_flag(s_timeout_list, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(s_timeout_list);

    for (int i = 0; i < 5; i++) {
        lv_obj_t *btn = lv_button_create(s_timeout_list);
        lv_obj_set_size(btn, HUB_BTN_W, 64);
        lv_obj_set_pos(btn, (UI_DISP - HUB_BTN_W) / 2, 100 + i * 76);
        lv_obj_set_style_radius(btn, 16, 0);
        lv_obj_set_style_bg_color(btn, t->menu_petal, 0);
        lv_obj_set_style_border_width(btn, 0, 0);

        s_timeout_row_lbls[i] = lv_label_create(btn);
        lv_label_set_text(s_timeout_row_lbls[i], s_timeout_names[i]);
        lv_obj_set_style_text_color(s_timeout_row_lbls[i], t->white, 0);
        lv_obj_set_style_text_font(s_timeout_row_lbls[i], &lv_font_montserrat_20, 0);
        lv_obj_center(s_timeout_row_lbls[i]);
        lv_obj_add_event_cb(btn, timeout_row_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
    }

    s_timeout_edit = lv_obj_create(s_panels[PANEL_TIMEOUTS]);
    lv_obj_set_size(s_timeout_edit, UI_DISP, UI_DISP);
    lv_obj_set_style_bg_opa(s_timeout_edit, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_timeout_edit, 0, 0);
    lv_obj_remove_flag(s_timeout_edit, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(s_timeout_edit);
    lv_obj_add_flag(s_timeout_edit, LV_OBJ_FLAG_HIDDEN);

    s_timeout_bundle.cfg = (ui_duration_editor_cfg_t){
        .value_sec = &s_timeout_edit_val,
        .show_end_time = false,
        .on_change = settings_idle_cb,
    };
    ui_duration_editor_create(s_timeout_edit, &s_timeout_bundle);

    lv_obj_t *edit_back = ui_widgets_create_side_btn(s_timeout_edit, true, 36, 330, NULL);
    lv_obj_add_event_cb(edit_back, timeout_edit_back_cb, LV_EVENT_CLICKED, NULL);

    attach_panel_wedges(s_panels[PANEL_TIMEOUTS], PANEL_TIMEOUTS, timeouts_save_cb);
    s_timeout_view = TIMEOUT_VIEW_LIST;
}

/* ---------- Adult auth ---------- */

static void aa_refresh_pin_label(void)
{
    size_t len = strlen(s_aa_pin_buf);
    char dots[12] = "";
    for (size_t i = 0; i < len; i++) {
        dots[i * 2] = '*';
        if (i < len - 1) {
            dots[i * 2 + 1] = ' ';
        }
    }
    lv_label_set_text(s_lbl_aa_pin, dots);
}

static void aa_pin_digit_cb(lv_event_t *e)
{
    const char *digit = (const char *)lv_event_get_user_data(e);
    size_t len = strlen(s_aa_pin_buf);
    if (len >= 4) {
        return;
    }
    s_aa_pin_buf[len] = digit[0];
    s_aa_pin_buf[len + 1] = '\0';
    aa_refresh_pin_label();
    ui_nav_reset_idle_timer();
}

static void aa_pin_back_cb(lv_event_t *e)
{
    (void)e;
    size_t len = strlen(s_aa_pin_buf);
    if (len > 0) {
        s_aa_pin_buf[len - 1] = '\0';
        aa_refresh_pin_label();
    }
    ui_nav_reset_idle_timer();
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
    s_draft.aa_methods = methods;
    ui_nav_reset_idle_timer();
}

static void aa_sync_from_draft(void)
{
    snprintf(s_aa_pin_buf, sizeof(s_aa_pin_buf), "%s", s_draft.aa_pin);
    aa_refresh_pin_label();

    if (s_draft.aa_methods & AA_METHOD_PIN) {
        lv_obj_add_state(s_chk_pin, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(s_chk_pin, LV_STATE_CHECKED);
    }
    if (s_draft.aa_methods & AA_METHOD_MATHS) {
        lv_obj_add_state(s_chk_maths, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(s_chk_maths, LV_STATE_CHECKED);
    }
}

static void aa_save_cb(lv_event_t *e)
{
    (void)e;
    app_config_t *cfg = app_config_get();

    if (strlen(s_aa_pin_buf) == 4) {
        snprintf(s_draft.aa_pin, sizeof(s_draft.aa_pin), "%s", s_aa_pin_buf);
    }
    cfg->aa_methods = s_draft.aa_methods;
    snprintf(cfg->aa_pin, sizeof(cfg->aa_pin), "%s", s_draft.aa_pin);
    s_saved.aa_methods = s_draft.aa_methods;
    memcpy(s_saved.aa_pin, s_draft.aa_pin, sizeof(s_saved.aa_pin));

    app_config_save_aa();
    show_panel(PANEL_HUB);
}

static void build_aa_panel(void)
{
    const ui_theme_t *t = ui_theme_get();

    s_panels[PANEL_AA] = create_sub_panel("Adult Authentication");

    s_chk_pin = lv_checkbox_create(s_panels[PANEL_AA]);
    lv_checkbox_set_text(s_chk_pin, "PIN");
    lv_obj_set_pos(s_chk_pin, 120, 90);
    lv_obj_set_style_text_color(s_chk_pin, t->white, 0);
    lv_obj_add_event_cb(s_chk_pin, aa_method_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);

    s_chk_maths = lv_checkbox_create(s_panels[PANEL_AA]);
    lv_checkbox_set_text(s_chk_maths, "Maths");
    lv_obj_set_pos(s_chk_maths, 120, 130);
    lv_obj_set_style_text_color(s_chk_maths, t->white, 0);
    lv_obj_add_event_cb(s_chk_maths, aa_method_changed_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *pin_bar = ui_widgets_create_purple_box(s_panels[PANEL_AA], 280, 48, 220, 180, false);
    s_lbl_aa_pin = lv_label_create(pin_bar);
    lv_label_set_text(s_lbl_aa_pin, "");
    lv_obj_set_style_text_color(s_lbl_aa_pin, t->white, 0);
    lv_obj_set_style_text_font(s_lbl_aa_pin, &lv_font_montserrat_22, 0);
    lv_obj_center(s_lbl_aa_pin);

    ui_widgets_add_numeric_keypad(s_panels[PANEL_AA], 300, aa_pin_digit_cb, NULL);
    lv_obj_t *back = ui_widgets_create_side_btn(s_panels[PANEL_AA], true, 36, 400, NULL);
    lv_obj_add_event_cb(back, aa_pin_back_cb, LV_EVENT_CLICKED, NULL);

    attach_panel_wedges(s_panels[PANEL_AA], PANEL_AA, aa_save_cb);
}

/* ---------- Hub ---------- */

static void settings_idle_cb(void *user_data)
{
    (void)user_data;
    ui_nav_reset_idle_timer();
}

static void build_hub_panel(void)
{
    int32_t cw = 0;
    int32_t ch = 0;

    s_panels[PANEL_HUB] = lv_obj_create(s_scr);
    lv_obj_set_style_bg_opa(s_panels[PANEL_HUB], LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_panels[PANEL_HUB], 0, 0);
    lv_obj_remove_flag(s_panels[PANEL_HUB], LV_OBJ_FLAG_SCROLLABLE);
    settings_panel_layout(s_panels[PANEL_HUB]);

    ui_widgets_create_title(s_panels[PANEL_HUB], "Settings");

    ui_layout_get_content_size(s_scr, &cw, &ch);
    const int total_h = 6 * HUB_BTN_H + 5 * HUB_BTN_GAP;
    const int y0 = (int)((ch - total_h) / 2) - 20;
    const int x0 = (int)((cw - HUB_BTN_W) / 2);

    static const char *labels[] = {
        "Colours",
        "Networking",
        "Timezone",
        "Schedule",
        "Timeouts",
        "Adult Authentication",
    };

    for (int i = 0; i < 6; i++) {
        hub_create_btn(s_panels[PANEL_HUB], labels[i], x0,
                       y0 + i * (HUB_BTN_H + HUB_BTN_GAP),
                       (settings_panel_t)(PANEL_COLOURS + i));
    }

    s_hub_cancel_wedge = ui_wedge_create(s_scr, UI_WEDGE_CANCEL);
    lv_obj_add_event_cb(s_hub_cancel_wedge, hub_cancel_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_flag(s_hub_cancel_wedge, LV_OBJ_FLAG_HIDDEN);
}

void ui_screen_settings_build(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    s_scr = ui_widgets_create_screen();
    screens[UI_SCREEN_SETTINGS] = s_scr;

    ui_keyboard_module_init();
    build_hub_panel();
    build_colours_panel();
    build_network_panel();
    build_timezone_panel();
    build_schedule_panel();
    build_timeouts_panel();
    build_aa_panel();

    show_panel(PANEL_HUB);
}

void ui_screen_settings_on_show(void)
{
    app_config_t *cfg = app_config_get();

    memcpy(&s_saved, cfg, sizeof(s_saved));
    memcpy(&s_draft, cfg, sizeof(s_draft));

    colours_sync_from_draft();
    network_sync_from_draft();
    tz_select_from_draft();
    schedule_sync_from_draft();
    timeout_show_list();
    aa_sync_from_draft();

    show_panel(PANEL_HUB);
}

void ui_screen_settings_apply_theme(void)
{
    const ui_theme_t *t = ui_theme_get();

    if (s_scr != NULL) {
        ui_widgets_style_circle_panel(s_scr);
    }
    if (s_net_edit_field_box != NULL) {
        lv_obj_set_style_bg_color(s_net_edit_field_box, t->ring, 0);
    }
    if (s_dd_country != NULL) {
        lv_obj_set_style_bg_color(s_dd_country, t->ring, 0);
    }
    if (s_dd_location != NULL) {
        lv_obj_set_style_bg_color(s_dd_location, t->ring, 0);
    }
}
