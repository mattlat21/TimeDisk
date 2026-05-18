#include "ui_screens_registry.h"
#include "ui_common.h"
#include "ui_theme.h"
#include "ui_nav.h"
#include "app_config.h"
#include <stdio.h>
#include <string.h>

static lv_obj_t *s_scr_ssid;
static lv_obj_t *s_scr_pw;
static lv_obj_t *lbl_ssid;
static lv_obj_t *lbl_pw;
static char s_ssid_buf[APP_WIFI_SSID_MAX];
static char s_pw_buf[APP_WIFI_PASSWORD_MAX];

static void ssid_digit_cb(lv_event_t *e)
{
    const char *digit = (const char *)lv_event_get_user_data(e);
    size_t len = strlen(s_ssid_buf);
    if (len + 1 >= sizeof(s_ssid_buf)) {
        return;
    }
    s_ssid_buf[len] = digit[0];
    s_ssid_buf[len + 1] = '\0';
    lv_label_set_text(lbl_ssid, s_ssid_buf);
    ui_nav_reset_idle_timer();
}

static void ssid_back_cb(lv_event_t *e)
{
    (void)e;
    size_t len = strlen(s_ssid_buf);
    if (len > 0) {
        s_ssid_buf[len - 1] = '\0';
        lv_label_set_text(lbl_ssid, s_ssid_buf);
    }
}

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

static void pw_next_cb(lv_event_t *e)
{
    (void)e;
    app_config_t *cfg = app_config_get();
    snprintf(cfg->wifi_password, sizeof(cfg->wifi_password), "%s", s_pw_buf);
    cfg->wifi_password_set = true;
    ui_nav_go(UI_SCREEN_LOADING);
}

static void pw_back_cb(lv_event_t *e)
{
    (void)e;
    ui_nav_go(UI_SCREEN_STARTUP_SSID);
}

static void build_ssid(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    const ui_theme_t *t = ui_theme_get();
    s_scr_ssid = ui_common_create_screen();
    screens[UI_SCREEN_STARTUP_SSID] = s_scr_ssid;
    s_ssid_buf[0] = '\0';

    ui_common_create_title(s_scr_ssid, "WiFi SSID");

    lv_obj_t *bar = ui_common_create_purple_box(s_scr_ssid, 340, 56, (UI_DISP - 340) / 2, 72, false);
    lbl_ssid = lv_label_create(bar);
    lv_label_set_text(lbl_ssid, "");
    lv_obj_set_style_text_color(lbl_ssid, t->white, 0);
    lv_obj_set_style_text_font(lbl_ssid, &lv_font_montserrat_26, 0);
    lv_obj_center(lbl_ssid);

    ui_common_add_numeric_keypad(s_scr_ssid, 268, ssid_digit_cb, NULL);

    lv_obj_t *back = ui_common_create_side_btn(s_scr_ssid, true, 36, 330, NULL);
    lv_obj_add_event_cb(back, ssid_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *next = ui_common_create_side_next(s_scr_ssid, UI_DISP - 36 - 64, 330);
    lv_obj_add_event_cb(next, ssid_next_cb, LV_EVENT_CLICKED, NULL);
}

static void build_password(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    const ui_theme_t *t = ui_theme_get();
    s_scr_pw = ui_common_create_screen();
    screens[UI_SCREEN_STARTUP_PASSWORD] = s_scr_pw;
    s_pw_buf[0] = '\0';

    ui_common_create_title(s_scr_pw, "WiFi Password");

    lv_obj_t *bar = ui_common_create_purple_box(s_scr_pw, 340, 56, (UI_DISP - 340) / 2, 120, false);
    lbl_pw = lv_label_create(bar);
    lv_label_set_text(lbl_pw, "(blank = open)");
    lv_obj_set_style_text_color(lbl_pw, t->white, 0);
    lv_obj_set_style_text_font(lbl_pw, &lv_font_montserrat_20, 0);
    lv_obj_center(lbl_pw);

    lv_obj_t *next = ui_common_create_side_next(s_scr_pw, UI_DISP - 36 - 64, 330);
    lv_obj_add_event_cb(next, pw_next_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back = ui_common_create_side_btn(s_scr_pw, true, 36, 330, NULL);
    lv_obj_add_event_cb(back, pw_back_cb, LV_EVENT_CLICKED, NULL);
}

extern "C" void ui_screen_wizard_build(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    build_ssid(screens);
    build_password(screens);
}

extern "C" void ui_screen_wizard_ssid_get_text(char *out, size_t len)
{
    strncpy(out, s_ssid_buf, len - 1);
    out[len - 1] = '\0';
}

extern "C" void ui_screen_wizard_password_get_text(char *out, size_t len)
{
    strncpy(out, s_pw_buf, len - 1);
    out[len - 1] = '\0';
}
