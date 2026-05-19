/*
 * ui_screen_startup_wifi_wizard.cpp — Boot-time WiFi setup (SSID + password).
 *
 * Wireframe: docs/wireframes/startup_wizard_ssid.svg
 *   - 720×720 circular panel, 14 px purple ring (#7D23BE)
 *   - Title, purple rounded SSID field (563×78 @ x=78 y=215, radius 30)
 *   - On-screen keyboard via ui_keyboard (three rows, four layouts)
 *   - Bottom-left orange (backspace / back), bottom-right green (confirm)
 *
 * Screens built here (registered in ui_screens_registry.cpp):
 *   UI_SCREEN_STARTUP_SSID      — "Wi-Fi Setup", enter network name
 *   UI_SCREEN_STARTUP_PASSWORD  — "Wi-Fi Password", optional passphrase
 *
 * Boot flow (ui_nav.cpp, stubbed until NVS/WiFi stack):
 *   splash → [SSID if missing] → [password if unset] → loading → TOD
 *   See docs/screen_flow.md and docs/data_model.md.
 *
 * Exported getters (tests / future esp_wifi_set_config):
 *   ui_screen_startup_wifi_wizard_ssid_get_text()
 *   ui_screen_startup_wifi_wizard_password_get_text()
 */

#include "ui_screens_registry.h"
#include "ui_common.h"
#include "ui_theme.h"
#include "ui_keyboard.h"
#include "ui_nav.h"
#include "app_config.h"
#include <stdio.h>
#include <string.h>

/* Purple text field — wireframe rect x=78 y=215 w=563 h=78 rx=30 */
#define WIFI_FIELD_X        78
#define WIFI_FIELD_Y        215
#define WIFI_FIELD_W        563
#define WIFI_FIELD_H        78
#define WIFI_FIELD_RADIUS   30

#define WIFI_TITLE_Y_OFFSET 28

/* Corner wedge buttons — lower than y≈330 used on other screens (wireframe ~y 589) */
#define WIFI_BTN_BACK_X     108
#define WIFI_BTN_NEXT_X     (UI_DISP - 108 - 64)
#define WIFI_BTN_Y          600

static lv_obj_t *s_scr_ssid;
static lv_obj_t *s_scr_pw;
static lv_obj_t *lbl_ssid;
static lv_obj_t *lbl_pw;
static char s_ssid_buf[APP_WIFI_SSID_MAX];
static char s_pw_buf[APP_WIFI_PASSWORD_MAX];
static ui_keyboard_t *s_ssid_kb;
static ui_keyboard_t *s_pw_kb;

static void wifi_keyboard_activity(void *user_data)
{
    (void)user_data;
    ui_nav_reset_idle_timer();
}

/* Purple rounded input bar; typed text shown in lbl (left-aligned). */
static lv_obj_t *wifi_create_ssid_field(lv_obj_t *parent, lv_obj_t **label_out)
{
    const ui_theme_t *t = ui_theme_get();
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_set_size(box, WIFI_FIELD_W, WIFI_FIELD_H);
    lv_obj_set_pos(box, WIFI_FIELD_X, WIFI_FIELD_Y);
    lv_obj_set_style_bg_color(box, t->ring, 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(box, 0, 0);
    lv_obj_set_style_radius(box, WIFI_FIELD_RADIUS, 0);
    lv_obj_set_style_pad_left(box, 24, 0);
    lv_obj_remove_flag(box, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(box);
    lv_label_set_text(lbl, "");
    lv_obj_set_style_text_color(lbl, t->white, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_26, 0);
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 0, 0);
    *label_out = lbl;
    return box;
}

/* Layout guides — coordinates relative to screen content area (inside 14 px ring). */
static void wifi_add_layout_guides(lv_obj_t *scr)
{
    int32_t cw;
    int32_t ch;
    ui_common_get_content_size(scr, &cw, &ch);

    ui_common_add_vertical_line(scr, cw / 2);
    ui_common_add_vertical_line(scr, cw - 20);
    ui_common_add_vertical_line(scr, 20);

    ui_common_add_horizontal_line(scr, ch / 2);
    ui_common_add_horizontal_line(scr, ch - 20);
    ui_common_add_horizontal_line(scr, 20);
}

/* Black circular screen + 14 px ring (thicker than ui_common_create_screen's 6 px). */
static lv_obj_t *wifi_create_screen(void)
{
    const ui_theme_t *t = ui_theme_get();
    lv_obj_t *scr = ui_common_create_screen();
    lv_obj_set_style_border_width(scr, 14, 0);
    lv_obj_set_style_border_color(scr, t->ring, 0);
    ui_common_line_points_reset();
    wifi_add_layout_guides(scr);
    return scr;
}

/* Orange corner on SSID screen: delete last character (not navigate back). */
static void ssid_back_cb(lv_event_t *e)
{
    (void)e;
    size_t len = strlen(s_ssid_buf);
    if (len > 0) {
        s_ssid_buf[len - 1] = '\0';
        lv_label_set_text(lbl_ssid, s_ssid_buf);
    }
    ui_nav_reset_idle_timer();
}

/* Green corner: require non-empty SSID, save to app_config, advance boot flow. */
static void ssid_next_cb(lv_event_t *e)
{
    (void)e;
    if (s_ssid_buf[0] == '\0') {
        return;
    }
    app_config_t *cfg = app_config_get();
    snprintf(cfg->wifi_ssid, sizeof(cfg->wifi_ssid), "%s", s_ssid_buf);
    if (app_config_wifi_password_unset()) {
        ui_nav_go(UI_SCREEN_STARTUP_PASSWORD);
    } else {
        ui_nav_go(UI_SCREEN_LOADING);
    }
}

/* Green corner: save password (may be empty for open network), go to loading. */
static void pw_next_cb(lv_event_t *e)
{
    (void)e;
    app_config_t *cfg = app_config_get();
    snprintf(cfg->wifi_password, sizeof(cfg->wifi_password), "%s", s_pw_buf);
    cfg->wifi_password_set = true;
    ui_nav_go(UI_SCREEN_LOADING);
}

/* Orange corner on password screen: return to SSID entry (does not delete chars). */
static void pw_back_cb(lv_event_t *e)
{
    (void)e;
    ui_nav_go(UI_SCREEN_STARTUP_SSID);
}

static void build_ssid(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    s_scr_ssid = wifi_create_screen();
    screens[UI_SCREEN_STARTUP_SSID] = s_scr_ssid;
    s_ssid_buf[0] = '\0';

    lv_obj_t *title = ui_common_create_title(s_scr_ssid, "Wi-Fi Setup");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, WIFI_TITLE_Y_OFFSET);

    wifi_create_ssid_field(s_scr_ssid, &lbl_ssid);

    ui_keyboard_config_t kb_cfg = {
        .buf = s_ssid_buf,
        .buf_len = sizeof(s_ssid_buf),
        .label = lbl_ssid,
        .initial_mode = UI_KEYBOARD_MODE_LOWER,
        .on_activity = wifi_keyboard_activity,
        .user_data = NULL,
    };
    s_ssid_kb = ui_keyboard_create(s_scr_ssid, &kb_cfg);

    lv_obj_t *back = ui_common_create_side_btn(s_scr_ssid, true, WIFI_BTN_BACK_X, WIFI_BTN_Y, NULL);
    lv_obj_add_event_cb(back, ssid_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *next = ui_common_create_side_btn(s_scr_ssid, false, WIFI_BTN_NEXT_X, WIFI_BTN_Y, NULL);
    lv_obj_add_event_cb(next, ssid_next_cb, LV_EVENT_CLICKED, NULL);
}

static void build_password(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    s_scr_pw = wifi_create_screen();
    screens[UI_SCREEN_STARTUP_PASSWORD] = s_scr_pw;
    s_pw_buf[0] = '\0';

    lv_obj_t *title = ui_common_create_title(s_scr_pw, "Wi-Fi Password");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, WIFI_TITLE_Y_OFFSET);

    wifi_create_ssid_field(s_scr_pw, &lbl_pw);
    lv_obj_set_style_text_font(lbl_pw, &lv_font_montserrat_20, 0);

    ui_keyboard_config_t kb_cfg = {
        .buf = s_pw_buf,
        .buf_len = sizeof(s_pw_buf),
        .label = lbl_pw,
        .initial_mode = UI_KEYBOARD_MODE_LOWER,
        .on_activity = wifi_keyboard_activity,
        .user_data = NULL,
    };
    s_pw_kb = ui_keyboard_create(s_scr_pw, &kb_cfg);

    lv_obj_t *back = ui_common_create_side_btn(s_scr_pw, true, WIFI_BTN_BACK_X, WIFI_BTN_Y, NULL);
    lv_obj_add_event_cb(back, pw_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *next = ui_common_create_side_btn(s_scr_pw, false, WIFI_BTN_NEXT_X, WIFI_BTN_Y, NULL);
    lv_obj_add_event_cb(next, pw_next_cb, LV_EVENT_CLICKED, NULL);
}

extern "C" void ui_screen_startup_wifi_wizard_build(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    ui_keyboard_module_init();
    build_ssid(screens);
    build_password(screens);
}

extern "C" void ui_screen_startup_wifi_wizard_ssid_get_text(char *out, size_t len)
{
    snprintf(out, len, "%s", s_ssid_buf);
}

extern "C" void ui_screen_startup_wifi_wizard_password_get_text(char *out, size_t len)
{
    snprintf(out, len, "%s", s_pw_buf);
}
