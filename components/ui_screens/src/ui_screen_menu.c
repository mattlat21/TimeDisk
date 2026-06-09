#include "ui_screens_registry.h"
#include "ui_layout.h"
#include "ui_widgets.h"
#include "ui_wedge.h"
#include "ui_theme.h"
#include "ui_nav.h"
#include "app_config.h"

static lv_obj_t *s_scr;
static lv_obj_t *s_scr_confirm;
static lv_obj_t *s_btn_rest;
static lv_obj_t *s_btn_sleep;
static lv_obj_t *s_btn_switch_wake;
static lv_obj_t *s_btn_timer;

#define MENU_BTN_W       560
#define MENU_BTN_H       112
#define MENU_BTN_GAP     20
#define MENU_BTN_RADIUS  24

static lv_obj_t *menu_create_action_btn(lv_obj_t *parent, const char *text, int x, int y,
                                        lv_event_cb_t cb)
{
    const ui_theme_t *t = ui_theme_get();
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, MENU_BTN_W, MENU_BTN_H);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_radius(btn, MENU_BTN_RADIUS, 0);
    lv_obj_set_style_bg_color(btn, t->menu_petal, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, t->white, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_26, 0);
    lv_obj_center(lbl);

    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    return btn;
}

static void menu_layout_buttons(bool in_wake_mode)
{
    int32_t cw = 0;
    int32_t ch = 0;

    ui_layout_get_content_size(s_scr, &cw, &ch);
    const int x0 = (int)((cw - MENU_BTN_W) / 2);

    if (in_wake_mode) {
        lv_obj_remove_flag(s_btn_rest, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(s_btn_sleep, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_btn_switch_wake, LV_OBJ_FLAG_HIDDEN);

        const int total_h = 3 * MENU_BTN_H + 2 * MENU_BTN_GAP;
        const int y0 = (int)((ch - total_h) / 2) - 36;
        lv_obj_set_pos(s_btn_rest, x0, y0);
        lv_obj_set_pos(s_btn_sleep, x0, y0 + MENU_BTN_H + MENU_BTN_GAP);
        lv_obj_set_pos(s_btn_timer, x0, y0 + 2 * (MENU_BTN_H + MENU_BTN_GAP));
    } else {
        lv_obj_add_flag(s_btn_rest, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_btn_sleep, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(s_btn_switch_wake, LV_OBJ_FLAG_HIDDEN);

        const int total_h = 2 * MENU_BTN_H + MENU_BTN_GAP;
        const int y0 = (int)((ch - total_h) / 2) - 36;
        lv_obj_set_pos(s_btn_switch_wake, x0, y0);
        lv_obj_set_pos(s_btn_timer, x0, y0 + MENU_BTN_H + MENU_BTN_GAP);
    }
}

static void rest_cb(lv_event_t *e)
{
    (void)e;
    ui_nav_reset_idle_timer();
    ui_nav_go(UI_SCREEN_REST_REST_END);
}

static void sleep_cb(lv_event_t *e)
{
    (void)e;
    ui_nav_reset_idle_timer();
    ui_nav_go(UI_SCREEN_SLEEP_WAKE);
}

static void switch_wake_cb(lv_event_t *e)
{
    (void)e;
    ui_nav_reset_idle_timer();
    ui_nav_go(UI_SCREEN_CONFIRM_SWITCH_WAKE);
}

static void timer_cb(lv_event_t *e)
{
    (void)e;
    ui_nav_reset_idle_timer();
    ui_nav_go(UI_SCREEN_TIMER_DURATION);
}

static void back_cb(lv_event_t *e)
{
    (void)e;
    ui_nav_reset_idle_timer();
    ui_nav_go(UI_SCREEN_TOD_BRIGHT);
}

static void settings_cb(lv_event_t *e)
{
    (void)e;
    ui_nav_reset_idle_timer();
    ui_nav_go(UI_SCREEN_SETTINGS);
}

static void switch_wake_confirm_no_cb(lv_event_t *e)
{
    (void)e;
    ui_nav_reset_idle_timer();
    ui_nav_go(UI_SCREEN_MENU);
}

static void switch_wake_confirm_yes_cb(lv_event_t *e)
{
    (void)e;
    ui_nav_reset_idle_timer();
    mode_engine_switch_to_wake();
    ui_nav_go(UI_SCREEN_MENU);
}

static void build_confirm_switch_wake(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    const ui_theme_t *t = ui_theme_get();

    s_scr_confirm = ui_widgets_create_screen();
    screens[UI_SCREEN_CONFIRM_SWITCH_WAKE] = s_scr_confirm;

    lv_obj_t *prompt = lv_label_create(s_scr_confirm);
    lv_label_set_text(prompt, "Are you sure you want to\nswitch to Wake mode?");
    lv_obj_set_width(prompt, UI_SCREEN_W);
    lv_obj_set_style_text_color(prompt, t->white, 0);
    lv_obj_set_style_text_font(prompt, &lv_font_montserrat_26, 0);
    lv_obj_set_style_text_align(prompt, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(prompt);

    lv_obj_t *cancel = ui_wedge_create(s_scr_confirm, UI_WEDGE_CANCEL);
    lv_obj_add_event_cb(cancel, switch_wake_confirm_no_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *confirm = ui_wedge_create(s_scr_confirm, UI_WEDGE_CONFIRM);
    lv_obj_add_event_cb(confirm, switch_wake_confirm_yes_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_move_foreground(cancel);
    lv_obj_move_foreground(confirm);
}

void ui_screen_menu_build(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    s_scr = ui_widgets_create_screen();
    screens[UI_SCREEN_MENU] = s_scr;

    int32_t cw = 0;
    int32_t ch = 0;
    ui_layout_get_content_size(s_scr, &cw, &ch);

    const int total_h = 3 * MENU_BTN_H + 2 * MENU_BTN_GAP;
    const int y0 = (int)((ch - total_h) / 2) - 36;
    const int x0 = (int)((cw - MENU_BTN_W) / 2);

    s_btn_rest = menu_create_action_btn(s_scr, "Start Rest", x0, y0, rest_cb);
    s_btn_sleep = menu_create_action_btn(s_scr, "Start Sleep", x0, y0 + MENU_BTN_H + MENU_BTN_GAP, sleep_cb);
    s_btn_switch_wake = menu_create_action_btn(s_scr, "Switch to Wake Mode", x0, y0, switch_wake_cb);
    s_btn_timer = menu_create_action_btn(s_scr, "Start Timer", x0, y0 + 2 * (MENU_BTN_H + MENU_BTN_GAP), timer_cb);

    lv_obj_add_flag(s_btn_switch_wake, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *back = ui_wedge_create(s_scr, UI_WEDGE_CANCEL);
    lv_obj_add_event_cb(back, back_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *settings = ui_wedge_create(s_scr, UI_WEDGE_SETTINGS);
    lv_obj_add_event_cb(settings, settings_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_move_foreground(back);
    lv_obj_move_foreground(settings);

    build_confirm_switch_wake(screens);
}

void ui_screen_menu_on_show(void)
{
    app_runtime_t *rt = app_runtime_get();
    menu_layout_buttons(rt->current_mode == APP_MODE_WAKE);
}

void ui_screen_menu_apply_theme(void)
{
    if (s_scr != NULL) {
        ui_widgets_style_circle_panel(s_scr);
    }
    if (s_scr_confirm != NULL) {
        ui_widgets_style_circle_panel(s_scr_confirm);
    }
}
