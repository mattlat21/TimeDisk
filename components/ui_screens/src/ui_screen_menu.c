#include "ui_screens_registry.h"
#include "ui_layout.h"
#include "ui_widgets.h"
#include "ui_wedge.h"
#include "ui_screen_ring.h"
#include "ui_theme.h"
#include "ui_nav.h"
#include "ui_assets.h"
#include "app_config.h"

static lv_obj_t *s_scr;
static lv_obj_t *s_scr_confirm;
static lv_obj_t *s_btn_rest;
static lv_obj_t *s_btn_sleep;
static lv_obj_t *s_btn_switch_wake;
static lv_obj_t *s_btn_timer;
static lv_obj_t *s_lbl_switch_wake;
static lv_obj_t *s_wedge_back;
static lv_obj_t *s_wedge_settings;

#define MENU_BTN_REST_W       320
#define MENU_BTN_REST_H       320
#define MENU_BTN_SLEEP_W      320
#define MENU_BTN_SLEEP_H      320
#define MENU_BTN_WAKE_W       660
#define MENU_BTN_WAKE_H       320
#define MENU_BTN_TIMER_W      660
#define MENU_BTN_TIMER_H      200

#define MENU_BTN_REST_X_WF    30
#define MENU_BTN_REST_Y_WF    30
#define MENU_BTN_SLEEP_X_WF   370
#define MENU_BTN_SLEEP_Y_WF   30
#define MENU_BTN_WAKE_X_WF    30
#define MENU_BTN_WAKE_Y_WF    30
#define MENU_BTN_TIMER_X_WF   30
#define MENU_BTN_TIMER_Y_WF   370

static lv_obj_t *menu_create_image_btn(lv_obj_t *parent, const lv_image_dsc_t *src, int w, int h,
                                       int x_wf, int y_wf, const char *label_text,
                                       const lv_font_t *label_font, int label_ofs_x, int label_ofs_y,
                                       lv_obj_t **label_out, lv_event_cb_t cb)
{
    int x = 0;
    int y = 0;

    ui_layout_screen_pos_from_wf(parent, x_wf, y_wf, &x, &y);

    lv_obj_t *btn = lv_obj_create(parent);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_pos(btn, x, y);
    lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);

    lv_obj_t *img = lv_image_create(btn);
    lv_image_set_src(img, src);
    lv_obj_set_size(img, w, h);
    lv_obj_set_pos(img, 0, 0);
    lv_obj_remove_flag(img, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    if (label_text != NULL && label_text[0] != '\0') {
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, label_text);
        lv_obj_set_width(lbl, w);
        lv_obj_set_style_text_color(lbl, ui_theme_get()->white, 0);
        lv_obj_set_style_text_font(lbl, label_font != NULL ? label_font : &lv_font_montserrat_26, 0);
        lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_remove_flag(lbl, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_align(lbl, LV_ALIGN_CENTER, label_ofs_x, label_ofs_y);
        lv_obj_move_foreground(lbl);
        if (label_out != NULL) {
            *label_out = lbl;
        }
    } else if (label_out != NULL) {
        *label_out = NULL;
    }

    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    return btn;
}

static const char *switch_wake_label_for_mode(app_mode_t mode)
{
    switch (mode) {
    case APP_MODE_REST:
        return "End Rest";
    case APP_MODE_SLEEP:
        return "End Sleep";
    case APP_MODE_WIND_DOWN:
        return "End Wind Down";
    default:
        return "End Rest";
    }
}

static void menu_layout_buttons(bool in_wake_mode)
{
    if (in_wake_mode) {
        lv_obj_remove_flag(s_btn_rest, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(s_btn_sleep, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_btn_switch_wake, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s_btn_rest, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_btn_sleep, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(s_btn_switch_wake, LV_OBJ_FLAG_HIDDEN);
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

    /* Raise ring overlay before layout so wf→content uses a consistent border (0). */
    ui_screen_ring_raise_overlay(s_scr);

    s_btn_rest = menu_create_image_btn(s_scr, &btn_start_rest, MENU_BTN_REST_W, MENU_BTN_REST_H,
                                       MENU_BTN_REST_X_WF, MENU_BTN_REST_Y_WF, "Start\nRest",
                                       &lv_font_montserrat_48, 30, 60, NULL, rest_cb);
    s_btn_sleep = menu_create_image_btn(s_scr, &btn_start_sleep, MENU_BTN_SLEEP_W, MENU_BTN_SLEEP_H,
                                        MENU_BTN_SLEEP_X_WF, MENU_BTN_SLEEP_Y_WF, "Start\nSleep",
                                        &lv_font_montserrat_48, -30, 60, NULL, sleep_cb);
    s_btn_switch_wake = menu_create_image_btn(s_scr, &btn_start_wake, MENU_BTN_WAKE_W, MENU_BTN_WAKE_H,
                                              MENU_BTN_WAKE_X_WF, MENU_BTN_WAKE_Y_WF, "End Rest",
                                              &lv_font_montserrat_48, 0, 60, &s_lbl_switch_wake, switch_wake_cb);
    s_btn_timer = menu_create_image_btn(s_scr, &btn_start_timer, MENU_BTN_TIMER_W, MENU_BTN_TIMER_H,
                                        MENU_BTN_TIMER_X_WF, MENU_BTN_TIMER_Y_WF, "Start Timer",
                                        &lv_font_montserrat_48, 0, 60, NULL, timer_cb);

    lv_obj_add_flag(s_btn_switch_wake, LV_OBJ_FLAG_HIDDEN);

    /* Ring above buttons; wedge positions unchanged (created after, raised last). */
    ui_screen_ring_raise_overlay(s_scr);

    s_wedge_back = ui_wedge_create(s_scr, UI_WEDGE_CANCEL);
    lv_obj_add_event_cb(s_wedge_back, back_cb, LV_EVENT_CLICKED, NULL);

    s_wedge_settings = ui_wedge_create(s_scr, UI_WEDGE_SETTINGS);
    lv_obj_add_event_cb(s_wedge_settings, settings_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_move_foreground(s_wedge_back);
    lv_obj_move_foreground(s_wedge_settings);

    build_confirm_switch_wake(screens);
}

void ui_screen_menu_on_show(void)
{
    app_runtime_t *rt = app_runtime_get();
    menu_layout_buttons(rt->current_mode == APP_MODE_WAKE);
    if (s_lbl_switch_wake != NULL) {
        lv_label_set_text(s_lbl_switch_wake, switch_wake_label_for_mode(rt->current_mode));
    }
}

void ui_screen_menu_apply_theme(void)
{
    if (s_scr != NULL) {
        ui_widgets_style_circle_panel(s_scr);
        ui_screen_ring_raise_overlay(s_scr);
        ui_screen_ring_refresh(s_scr);
        if (s_wedge_back != NULL) {
            lv_obj_move_foreground(s_wedge_back);
        }
        if (s_wedge_settings != NULL) {
            lv_obj_move_foreground(s_wedge_settings);
        }
    }
    if (s_scr_confirm != NULL) {
        ui_widgets_style_circle_panel(s_scr_confirm);
    }
}
