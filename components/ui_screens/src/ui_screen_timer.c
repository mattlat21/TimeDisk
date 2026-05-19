#include "ui_screens_registry.h"
#include "ui_layout.h"
#include "ui_widgets.h"
#include "ui_format.h"
#include "ui_wedge.h"
#include "ui_lines.h"
#include "ui_theme.h"
#include "ui_nav.h"
#include "app_config.h"
#include <stdio.h>

static lv_obj_t *s_scr_duration;
static lv_obj_t *s_scr_style;
static lv_obj_t *s_scr_bright;
static lv_obj_t *s_scr_dim;
static lv_obj_t *s_scr_triggered;
static lv_obj_t *s_scr_confirm;
static lv_obj_t *lbl_duration;
static lv_obj_t *lbl_countdown_bright;
static lv_obj_t *lbl_countdown_dim;
static uint32_t s_duration_sec;
static uint8_t s_style_id;

static void duration_refresh(void)
{
    char buf[16];
    ui_format_mm_ss(buf, sizeof(buf), s_duration_sec);
    lv_label_set_text(lbl_duration, buf);
}

static void duration_minus_cb(lv_event_t *e)
{
    (void)e;
    if (s_duration_sec > 60) {
        s_duration_sec -= 60;
    } else if (s_duration_sec > 0) {
        s_duration_sec = 0;
    }
    duration_refresh();
    ui_nav_reset_idle_timer();
}

static void duration_plus_cb(lv_event_t *e)
{
    (void)e;
    s_duration_sec += 60;
    if (s_duration_sec > 3600) {
        s_duration_sec = 3600;
    }
    duration_refresh();
    ui_nav_reset_idle_timer();
}

static void duration_back_cb(lv_event_t *e)
{
    (void)e;
    ui_nav_go(UI_SCREEN_MENU);
}

static void duration_next_cb(lv_event_t *e)
{
    (void)e;
    app_config_get()->timer_duration_sec = s_duration_sec;
    app_config_save_timer();
    ui_nav_go(UI_SCREEN_TIMER_STYLE);
}

static void style_back_cb(lv_event_t *e)
{
    (void)e;
    ui_nav_go(UI_SCREEN_TIMER_DURATION);
}

static void style_next_cb(lv_event_t *e)
{
    (void)e;
    app_config_get()->timer_style_id = s_style_id;
    app_config_save_timer();
    app_runtime_t *rt = app_runtime_get();
    rt->active_timer_remaining_sec = s_duration_sec;
    rt->timer_running = true;
    ui_nav_go(UI_SCREEN_TIMER_BRIGHT);
}

static void style_pick_cb(lv_event_t *e)
{
    s_style_id = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    ui_nav_reset_idle_timer();
}

static void timer_tap_cb(lv_event_t *e)
{
    (void)e;
    if (ui_nav_current() == UI_SCREEN_TIMER_DIM) {
        ui_nav_go(UI_SCREEN_TIMER_BRIGHT);
    }
}

static void timer_end_cb(lv_event_t *e)
{
    (void)e;
    ui_nav_start_aa(UI_SCREEN_TIMER_BRIGHT, UI_SCREEN_CONFIRM_END_TIMER);
}

static void confirm_no_cb(lv_event_t *e)
{
    (void)e;
    ui_nav_go(UI_SCREEN_TIMER_BRIGHT);
}

static void confirm_yes_cb(lv_event_t *e)
{
    (void)e;
    app_runtime_t *rt = app_runtime_get();
    rt->timer_running = false;
    rt->active_timer_remaining_sec = 0;
    ui_nav_go(UI_SCREEN_MENU);
}

static void triggered_ok_cb(lv_event_t *e)
{
    (void)e;
    app_runtime_t *rt = app_runtime_get();
    rt->timer_running = false;
    rt->active_timer_remaining_sec = 0;
    ui_nav_go(UI_SCREEN_TOD_BRIGHT);
}

static lv_obj_t *make_stepper_btn(lv_obj_t *parent, const char *txt, int x, int y, lv_event_cb_t cb)
{
    const ui_theme_t *t = ui_theme_get();
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 80, 80);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(btn, t->keypad, 0);
    lv_obj_t *l = lv_label_create(btn);
    lv_label_set_text(l, txt);
    lv_obj_set_style_text_color(l, t->white, 0);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_26, 0);
    lv_obj_center(l);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);
    return btn;
}

static void build_duration(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    const ui_theme_t *t = ui_theme_get();
    s_scr_duration = ui_widgets_create_screen();
    screens[UI_SCREEN_TIMER_DURATION] = s_scr_duration;
    s_duration_sec = app_config_get()->timer_duration_sec;

    ui_widgets_create_title(s_scr_duration, "Set Duration");

    lv_obj_t *box = ui_widgets_create_purple_box(s_scr_duration, 400, 120, 160, 200, false);
    lbl_duration = lv_label_create(box);
    lv_obj_set_style_text_color(lbl_duration, t->white, 0);
    lv_obj_set_style_text_font(lbl_duration, &lv_font_montserrat_26, 0);
    lv_obj_center(lbl_duration);
    duration_refresh();

    make_stepper_btn(s_scr_duration, "-", 200, 380, duration_minus_cb);
    make_stepper_btn(s_scr_duration, "+", 520, 380, duration_plus_cb);

    lv_obj_t *back = ui_widgets_create_side_btn(s_scr_duration, true, 36, 330, NULL);
    lv_obj_add_event_cb(back, duration_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *next = ui_widgets_create_side_next(s_scr_duration, UI_DISP - 36 - 64, 330);
    lv_obj_add_event_cb(next, duration_next_cb, LV_EVENT_CLICKED, NULL);
}

static void build_style(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    const ui_theme_t *t = ui_theme_get();
    s_scr_style = ui_widgets_create_screen();
    screens[UI_SCREEN_TIMER_STYLE] = s_scr_style;
    s_style_id = app_config_get()->timer_style_id;

    ui_widgets_create_title(s_scr_style, "Set Style");

    const char *names[] = {"Default", "Ring", "Minimal"};
    for (int i = 0; i < 3; i++) {
        lv_obj_t *btn = lv_button_create(s_scr_style);
        lv_obj_set_size(btn, 200, 48);
        lv_obj_set_pos(btn, 260, 200 + i * 60);
        lv_obj_set_style_bg_color(btn, t->panel, 0);
        lv_obj_t *l = lv_label_create(btn);
        lv_label_set_text(l, names[i]);
        lv_obj_set_style_text_color(l, t->white, 0);
        lv_obj_center(l);
        lv_obj_add_event_cb(btn, style_pick_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
    }

    lv_obj_t *back = ui_widgets_create_side_btn(s_scr_style, true, 36, 330, NULL);
    lv_obj_add_event_cb(back, style_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *next = ui_widgets_create_side_next(s_scr_style, UI_DISP - 36 - 64, 330);
    lv_obj_add_event_cb(next, style_next_cb, LV_EVENT_CLICKED, NULL);
}

static void build_countdown(lv_obj_t **scr, lv_obj_t **lbl, ui_screen_id_t id, lv_obj_t *screens[UI_SCREEN_COUNT])
{
    const ui_theme_t *t = ui_theme_get();
    *scr = ui_widgets_create_screen();
    screens[id] = *scr;

    *lbl = lv_label_create(*scr);
    lv_obj_set_style_text_color(*lbl, t->white, 0);
    lv_obj_set_style_text_font(*lbl, &lv_font_montserrat_26, 0);
    lv_obj_center(*lbl);

    lv_obj_add_event_cb(*scr, timer_tap_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *end = lv_button_create(*scr);
    lv_obj_set_size(end, 100, 40);
    lv_obj_align(end, LV_ALIGN_BOTTOM_MID, 0, -80);
    lv_obj_set_style_bg_color(end, t->orange, 0);
    lv_obj_t *el = lv_label_create(end);
    lv_label_set_text(el, "End");
    lv_obj_set_style_text_color(el, t->white, 0);
    lv_obj_center(el);
    lv_obj_add_event_cb(end, timer_end_cb, LV_EVENT_CLICKED, NULL);
}

static void build_triggered(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    const ui_theme_t *t = ui_theme_get();
    s_scr_triggered = ui_widgets_create_screen();
    screens[UI_SCREEN_TIMER_TRIGGERED] = s_scr_triggered;

    lv_obj_t *lbl = lv_label_create(s_scr_triggered);
    lv_label_set_text(lbl, "Timer Done!");
    lv_obj_set_style_text_color(lbl, t->white, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_26, 0);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, -40);

    lv_obj_t *ok = ui_widgets_create_side_btn(s_scr_triggered, false, UI_DISP - 36 - 64, 330, NULL);
    lv_obj_add_event_cb(ok, triggered_ok_cb, LV_EVENT_CLICKED, NULL);
}

static void build_confirm(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    s_scr_confirm = ui_widgets_create_screen();
    screens[UI_SCREEN_CONFIRM_END_TIMER] = s_scr_confirm;

    ui_widgets_create_title(s_scr_confirm, "Are you sure?");

    lv_obj_t *no = ui_widgets_create_side_btn(s_scr_confirm, true, 36, 330, "No");
    lv_obj_add_event_cb(no, confirm_no_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *yes = ui_widgets_create_side_btn(s_scr_confirm, false, UI_DISP - 36 - 64, 330, "Yes");
    lv_obj_add_event_cb(yes, confirm_yes_cb, LV_EVENT_CLICKED, NULL);
}

void ui_screen_timer_build(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    build_duration(screens);
    build_style(screens);
    build_countdown(&s_scr_bright, &lbl_countdown_bright, UI_SCREEN_TIMER_BRIGHT, screens);
    build_countdown(&s_scr_dim, &lbl_countdown_dim, UI_SCREEN_TIMER_DIM, screens);
    build_triggered(screens);
    build_confirm(screens);
}

void ui_screen_timer_on_show(ui_screen_id_t id)
{
    char buf[16];
    app_runtime_t *rt = app_runtime_get();
    ui_format_mm_ss(buf, sizeof(buf), rt->active_timer_remaining_sec);

    if (id == UI_SCREEN_TIMER_BRIGHT) {
        lv_label_set_text(lbl_countdown_bright, buf);
        ui_nav_apply_dim(false);
    } else if (id == UI_SCREEN_TIMER_DIM) {
        lv_label_set_text(lbl_countdown_dim, buf);
        ui_nav_apply_dim(true);
    }
}

void ui_screen_timer_tick(void)
{
    char buf[16];
    app_runtime_t *rt = app_runtime_get();
    if (!rt->timer_running) {
        return;
    }
    ui_format_mm_ss(buf, sizeof(buf), rt->active_timer_remaining_sec);
    ui_screen_id_t cur = ui_nav_current();
    if (cur == UI_SCREEN_TIMER_BRIGHT) {
        lv_label_set_text(lbl_countdown_bright, buf);
    } else if (cur == UI_SCREEN_TIMER_DIM) {
        lv_label_set_text(lbl_countdown_dim, buf);
    }
}

uint32_t ui_screen_duration_get_sec(void)
{
    return s_duration_sec;
}

uint8_t ui_screen_style_get_id(void)
{
    return s_style_id;
}
