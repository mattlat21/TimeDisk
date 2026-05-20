#include "ui_screens_registry.h"
#include "ui_layout.h"
#include "ui_widgets.h"
#include "ui_wedge.h"
#include "ui_theme.h"
#include "ui_nav.h"

static lv_obj_t *s_scr;

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

void ui_screen_menu_build(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    const int border = UI_RING_BORDER_DEFAULT;

    s_scr = ui_widgets_create_screen();
    screens[UI_SCREEN_MENU] = s_scr;

    int32_t cw = 0;
    int32_t ch = 0;
    ui_layout_get_content_size(s_scr, &cw, &ch);

    const int total_h = 3 * MENU_BTN_H + 2 * MENU_BTN_GAP;
    const int y0 = (int)((ch - total_h) / 2) - 36;
    const int x0 = (int)((cw - MENU_BTN_W) / 2);

    menu_create_action_btn(s_scr, "Start Rest", x0, y0, rest_cb);
    menu_create_action_btn(s_scr, "Start Sleep", x0, y0 + MENU_BTN_H + MENU_BTN_GAP, sleep_cb);
    menu_create_action_btn(s_scr, "Start Timer", x0, y0 + 2 * (MENU_BTN_H + MENU_BTN_GAP), timer_cb);

    lv_obj_t *back = ui_wedge_create(
        s_scr, UI_WEDGE_CANCEL,
        UI_WF_X(UI_WEDGE_CANCEL_X_WF, border),
        UI_WF_Y(UI_WEDGE_CANCEL_Y_WF, border));
    lv_obj_add_event_cb(back, back_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *settings = ui_wedge_create(
        s_scr, UI_WEDGE_SETTINGS,
        UI_WF_X(UI_WEDGE_CONFIRM_X_WF, border),
        UI_WF_Y(UI_WEDGE_CONFIRM_Y_WF, border));
    lv_obj_add_event_cb(settings, settings_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_move_foreground(back);
    lv_obj_move_foreground(settings);
}

void ui_screen_menu_apply_theme(void)
{
    if (s_scr != NULL) {
        ui_widgets_style_circle_panel(s_scr);
    }
}
