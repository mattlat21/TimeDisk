#include "ui_screens_registry.h"
#include "ui_common.h"
#include "ui_theme.h"
#include "ui_nav.h"
#include "app_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static lv_obj_t *s_scr;
static lv_obj_t *scr_pin;
static lv_obj_t *scr_maths;
static lv_obj_t *lbl_pin;
static lv_obj_t *lbl_a;
static lv_obj_t *lbl_b;
static lv_obj_t *lbl_answer;

static void pin_digit_cb(lv_event_t *e)
{
    const char *digit = (const char *)lv_event_get_user_data(e);
    aa_session_t *s = ui_nav_aa_session();
    size_t len = strlen(s->pin_buf);
    if (len >= 4) {
        return;
    }
    s->pin_buf[len] = digit[0];
    s->pin_buf[len + 1] = '\0';
    char dots[12] = "";
    for (size_t i = 0; i < len + 1; i++) {
        dots[i * 2] = '*';
        if (i < len) {
            dots[i * 2 + 1] = ' ';
        }
    }
    lv_label_set_text(lbl_pin, dots);
    ui_nav_aa_touch();
}

static void pin_back_cb(lv_event_t *e)
{
    (void)e;
    aa_session_t *s = ui_nav_aa_session();
    size_t len = strlen(s->pin_buf);
    if (len > 0) {
        s->pin_buf[len - 1] = '\0';
        char dots[12] = "";
        for (size_t i = 0; i < len - 1; i++) {
            dots[i * 2] = '*';
            dots[i * 2 + 1] = ' ';
        }
        lv_label_set_text(lbl_pin, dots);
    }
    ui_nav_aa_touch();
}

static void pin_ok_cb(lv_event_t *e)
{
    (void)e;
    aa_session_t *s = ui_nav_aa_session();
    const app_config_t *cfg = app_config_get();
    if (strlen(s->pin_buf) != 4) {
        return;
    }
    if (strcmp(s->pin_buf, cfg->aa_pin) != 0) {
        ui_screen_aa_reset_pin();
        return;
    }
    if ((cfg->aa_methods & AA_METHOD_MATHS) != 0) {
        s->step = AA_STEP_MATHS;
        ui_nav_aa_new_maths();
        ui_screen_aa_reset_maths();
        ui_screen_aa_update_maths_labels(s->maths_a, s->maths_b);
        ui_screen_aa_show_maths();
    } else {
        ui_nav_aa_pass();
    }
    ui_nav_aa_touch();
}

static void pin_cancel_cb(lv_event_t *e)
{
    (void)e;
    ui_nav_aa_cancel();
}

static void maths_digit_cb(lv_event_t *e)
{
    const char *digit = (const char *)lv_event_get_user_data(e);
    aa_session_t *s = ui_nav_aa_session();
    size_t len = strlen(s->answer_buf);
    if (len >= 2) {
        return;
    }
    s->answer_buf[len] = digit[0];
    s->answer_buf[len + 1] = '\0';
    lv_label_set_text(lbl_answer, s->answer_buf);
    ui_nav_aa_touch();
}

static void maths_back_cb(lv_event_t *e)
{
    (void)e;
    aa_session_t *s = ui_nav_aa_session();
    size_t len = strlen(s->answer_buf);
    if (len > 0) {
        s->answer_buf[len - 1] = '\0';
        lv_label_set_text(lbl_answer, s->answer_buf);
    } else if ((app_config_get()->aa_methods & AA_METHOD_PIN) != 0) {
        s->step = AA_STEP_PIN;
        ui_screen_aa_show_pin();
    }
    ui_nav_aa_touch();
}

static void maths_ok_cb(lv_event_t *e)
{
    (void)e;
    aa_session_t *s = ui_nav_aa_session();
    if (strlen(s->answer_buf) != 2) {
        return;
    }
    int ans = atoi(s->answer_buf);
    if (ans == s->maths_sum) {
        ui_nav_aa_pass();
    } else {
        ui_nav_aa_new_maths();
        ui_screen_aa_reset_maths();
        ui_screen_aa_update_maths_labels(s->maths_a, s->maths_b);
    }
    ui_nav_aa_touch();
}

static void build_pin_panel(lv_obj_t *parent)
{
    const ui_theme_t *t = ui_theme_get();
    scr_pin = lv_obj_create(parent);
    lv_obj_set_size(scr_pin, UI_DISP, UI_DISP);
    lv_obj_set_style_bg_opa(scr_pin, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(scr_pin, 0, 0);
    lv_obj_remove_flag(scr_pin, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(scr_pin);

    lv_obj_t *pin_bar = ui_common_create_purple_box(scr_pin, 340, 56, (UI_DISP - 340) / 2, 72, false);
    lbl_pin = lv_label_create(pin_bar);
    lv_label_set_text(lbl_pin, "");
    lv_obj_set_style_text_color(lbl_pin, t->white, 0);
    lv_obj_set_style_text_font(lbl_pin, &lv_font_montserrat_26, 0);
    lv_obj_center(lbl_pin);

    ui_common_add_numeric_keypad(scr_pin, 268, pin_digit_cb, NULL);

    lv_obj_t *back = ui_common_create_side_btn(scr_pin, true, 36, 330, NULL);
    lv_obj_add_event_cb(back, pin_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ok = ui_common_create_side_btn(scr_pin, false, UI_DISP - 36 - 64, 330, NULL);
    lv_obj_add_event_cb(ok, pin_ok_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *cancel = lv_button_create(scr_pin);
    lv_obj_set_size(cancel, 107, 107);
    lv_obj_align(cancel, LV_ALIGN_BOTTOM_MID, 0, -24);
    lv_obj_set_style_radius(cancel, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(cancel, t->keypad, 0);
    lv_obj_t *cl = lv_label_create(cancel);
    lv_label_set_text(cl, "Cancel");
    lv_obj_set_style_text_color(cl, t->white, 0);
    lv_obj_set_style_text_font(cl, &lv_font_montserrat_20, 0);
    lv_obj_center(cl);
    lv_obj_add_event_cb(cancel, pin_cancel_cb, LV_EVENT_CLICKED, NULL);
}

static lv_obj_t *make_eq_box(lv_obj_t *parent, int x, int y, lv_obj_t **lbl_out, bool outline)
{
    const ui_theme_t *t = ui_theme_get();
    lv_obj_t *box = ui_common_create_purple_box(parent, 64, 64, x, y, outline);
    *lbl_out = lv_label_create(box);
    lv_label_set_text(*lbl_out, "");
    lv_obj_set_style_text_color(*lbl_out, t->white, 0);
    lv_obj_set_style_text_font(*lbl_out, &lv_font_montserrat_26, 0);
    lv_obj_center(*lbl_out);
    return box;
}

static void build_maths_panel(lv_obj_t *parent)
{
    const ui_theme_t *t = ui_theme_get();
    scr_maths = lv_obj_create(parent);
    lv_obj_set_size(scr_maths, UI_DISP, UI_DISP);
    lv_obj_set_style_bg_opa(scr_maths, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(scr_maths, 0, 0);
    lv_obj_remove_flag(scr_maths, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_center(scr_maths);
    lv_obj_add_flag(scr_maths, LV_OBJ_FLAG_HIDDEN);

    const int box = 64;
    const int gap = 12;
    const int eq_w = box * 3 + gap * 2 + 24 + 28 + 24;
    int x = (UI_DISP - eq_w) / 2;
    const int y = 88;

    make_eq_box(scr_maths, x, y, &lbl_a, false);
    x += box + gap;
    lv_obj_t *plus = lv_label_create(scr_maths);
    lv_label_set_text(plus, "+");
    lv_obj_set_style_text_color(plus, t->white, 0);
    lv_obj_set_style_text_font(plus, &lv_font_montserrat_26, 0);
    lv_obj_set_pos(plus, x, y + 16);
    x += 28 + gap;
    make_eq_box(scr_maths, x, y, &lbl_b, false);
    x += box + gap;
    lv_obj_t *eq = lv_label_create(scr_maths);
    lv_label_set_text(eq, "=");
    lv_obj_set_style_text_color(eq, t->white, 0);
    lv_obj_set_style_text_font(eq, &lv_font_montserrat_26, 0);
    lv_obj_set_pos(eq, x, y + 16);
    x += 28 + gap;
    make_eq_box(scr_maths, x, y, &lbl_answer, true);

    ui_common_add_numeric_keypad(scr_maths, 300, maths_digit_cb, NULL);

    lv_obj_t *back = ui_common_create_side_btn(scr_maths, true, 36, 330, NULL);
    lv_obj_add_event_cb(back, maths_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ok = ui_common_create_side_btn(scr_maths, false, UI_DISP - 36 - 64, 330, NULL);
    lv_obj_add_event_cb(ok, maths_ok_cb, LV_EVENT_CLICKED, NULL);
}

extern "C" void ui_screen_aa_build(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    s_scr = ui_common_create_screen();
    screens[UI_SCREEN_ADULT_AUTH] = s_scr;
    build_pin_panel(s_scr);
    build_maths_panel(s_scr);
}

extern "C" void ui_screen_aa_on_show(void)
{
    aa_session_t *s = ui_nav_aa_session();
    if (s->step == AA_STEP_MATHS) {
        ui_screen_aa_show_maths();
    } else {
        ui_screen_aa_show_pin();
    }
}

extern "C" void ui_screen_aa_show_pin(void)
{
    lv_obj_remove_flag(scr_pin, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(scr_maths, LV_OBJ_FLAG_HIDDEN);
}

extern "C" void ui_screen_aa_show_maths(void)
{
    lv_obj_add_flag(scr_pin, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(scr_maths, LV_OBJ_FLAG_HIDDEN);
}

extern "C" void ui_screen_aa_reset_pin(void)
{
    aa_session_t *s = ui_nav_aa_session();
    s->pin_buf[0] = '\0';
    lv_label_set_text(lbl_pin, "");
}

extern "C" void ui_screen_aa_reset_maths(void)
{
    aa_session_t *s = ui_nav_aa_session();
    s->answer_buf[0] = '\0';
    lv_label_set_text(lbl_answer, "");
}

extern "C" void ui_screen_aa_update_maths_labels(int a, int b)
{
    char buf[4];
    snprintf(buf, sizeof(buf), "%d", a);
    lv_label_set_text(lbl_a, buf);
    snprintf(buf, sizeof(buf), "%d", b);
    lv_label_set_text(lbl_b, buf);
}

extern "C" bool ui_screen_aa_pin_ok_pressed(void)
{
    return strlen(ui_nav_aa_session()->pin_buf) == 4;
}

extern "C" bool ui_screen_aa_maths_ok_pressed(void)
{
    return strlen(ui_nav_aa_session()->answer_buf) == 2;
}
