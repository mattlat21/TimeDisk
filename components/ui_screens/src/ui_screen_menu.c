#include "ui_screens_registry.h"
#include "ui_layout.h"
#include "ui_widgets.h"
#include "ui_format.h"
#include "ui_wedge.h"
#include "ui_lines.h"
#include "ui_theme.h"
#include "ui_nav.h"

static lv_obj_t *s_scr;

static void petal_cb(lv_event_t *e)
{
    uintptr_t id = (uintptr_t)lv_event_get_user_data(e);
    ui_nav_reset_idle_timer();
    switch (id) {
    case 0:
        ui_nav_go(UI_SCREEN_SETTINGS);
        break;
    case 1:
        ui_nav_go(UI_SCREEN_TIMER_DURATION);
        break;
    case 2:
        ui_nav_go(UI_SCREEN_SLEEP_WAKE);
        break;
    case 3:
        ui_nav_go(UI_SCREEN_REST_REST_END);
        break;
    default:
        break;
    }
}

static void back_cb(lv_event_t *e)
{
    (void)e;
    ui_nav_go(UI_SCREEN_TOD_BRIGHT);
}

void ui_screen_menu_build(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    const ui_theme_t *t = ui_theme_get();
    s_scr = ui_widgets_create_screen();
    screens[UI_SCREEN_MENU] = s_scr;

    lv_obj_t *bg = lv_obj_create(s_scr);
    lv_obj_remove_style_all(bg);
    lv_obj_set_size(bg, UI_DISP, UI_DISP);
    lv_obj_center(bg);

    const int cx = UI_DISP / 2;
    const int cy = UI_DISP / 2;

    lv_obj_t *w1 = lv_obj_create(s_scr);
    lv_obj_remove_style_all(w1);
    lv_obj_set_size(w1, 320, 320);
    lv_obj_set_pos(w1, cx, cy - 160);
    lv_obj_set_style_bg_color(w1, t->menu_petal, 0);
    lv_obj_set_style_bg_opa(w1, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(w1, 0, 0);
    lv_obj_add_event_cb(w1, petal_cb, LV_EVENT_CLICKED, (void *)0);

    lv_obj_t *w2 = lv_obj_create(s_scr);
    lv_obj_remove_style_all(w2);
    lv_obj_set_size(w2, 320, 320);
    lv_obj_set_pos(w2, cx, cy);
    lv_obj_set_style_bg_color(w2, t->menu_petal, 0);
    lv_obj_set_style_bg_opa(w2, LV_OPA_COVER, 0);
    lv_obj_add_event_cb(w2, petal_cb, LV_EVENT_CLICKED, (void *)1);

    lv_obj_t *w3 = lv_obj_create(s_scr);
    lv_obj_remove_style_all(w3);
    lv_obj_set_size(w3, 320, 320);
    lv_obj_set_pos(w3, cx - 320, cy);
    lv_obj_set_style_bg_color(w3, t->menu_petal, 0);
    lv_obj_set_style_bg_opa(w3, LV_OPA_COVER, 0);
    lv_obj_add_event_cb(w3, petal_cb, LV_EVENT_CLICKED, (void *)2);

    lv_obj_t *w4 = lv_obj_create(s_scr);
    lv_obj_remove_style_all(w4);
    lv_obj_set_size(w4, 320, 320);
    lv_obj_set_pos(w4, cx - 320, cy - 160);
    lv_obj_set_style_bg_color(w4, t->menu_petal, 0);
    lv_obj_set_style_bg_opa(w4, LV_OPA_COVER, 0);
    lv_obj_add_event_cb(w4, petal_cb, LV_EVENT_CLICKED, (void *)3);

    lv_obj_t *sep1 = lv_obj_create(s_scr);
    lv_obj_remove_style_all(sep1);
    lv_obj_set_size(sep1, 4, 280);
    lv_obj_set_pos(sep1, cx - 2, 220);
    lv_obj_set_style_bg_color(sep1, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(sep1, LV_OPA_COVER, 0);

    lv_obj_t *sep2 = lv_obj_create(s_scr);
    lv_obj_remove_style_all(sep2);
    lv_obj_set_size(sep2, 280, 4);
    lv_obj_set_pos(sep2, 220, cy - 2);
    lv_obj_set_style_bg_color(sep2, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(sep2, LV_OPA_COVER, 0);

    lv_obj_t *center_mask = lv_obj_create(s_scr);
    lv_obj_remove_style_all(center_mask);
    lv_obj_set_size(center_mask, 156, 156);
    lv_obj_set_pos(center_mask, cx - 78, cy - 78);
    lv_obj_set_style_bg_color(center_mask, t->bg, 0);
    lv_obj_set_style_bg_opa(center_mask, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(center_mask, LV_RADIUS_CIRCLE, 0);

    lv_obj_t *lbl_s = lv_label_create(s_scr);
    lv_label_set_text(lbl_s, "Settings");
    lv_obj_set_style_text_color(lbl_s, t->white, 0);
    lv_obj_set_pos(lbl_s, 430, 180);

    lv_obj_t *lbl_t = lv_label_create(s_scr);
    lv_label_set_text(lbl_t, "Start Timer");
    lv_obj_set_style_text_color(lbl_t, t->white, 0);
    lv_obj_set_pos(lbl_t, 430, 480);

    lv_obj_t *lbl_sl = lv_label_create(s_scr);
    lv_label_set_text(lbl_sl, "Sleep");
    lv_obj_set_style_text_color(lbl_sl, t->white, 0);
    lv_obj_set_pos(lbl_sl, 150, 480);

    lv_obj_t *lbl_r = lv_label_create(s_scr);
    lv_label_set_text(lbl_r, "Rest");
    lv_obj_set_style_text_color(lbl_r, t->white, 0);
    lv_obj_set_pos(lbl_r, 150, 180);

    lv_obj_t *back = lv_button_create(s_scr);
    lv_obj_set_size(back, 124, 124);
    lv_obj_set_pos(back, cx - 62, cy - 62);
    lv_obj_set_style_radius(back, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(back, lv_color_make(0xFC, 0x67, 0x00), 0);
    lv_obj_t *bl = lv_label_create(back);
    lv_label_set_text(bl, "Back");
    lv_obj_set_style_text_color(bl, t->white, 0);
    lv_obj_center(bl);
    lv_obj_add_event_cb(back, back_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_move_foreground(center_mask);
    lv_obj_move_foreground(back);
}
