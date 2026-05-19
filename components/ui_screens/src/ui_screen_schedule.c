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

static lv_obj_t *s_screens[5];
static lv_obj_t *lbl_values[5];
static uint32_t s_wizard_vals[5];
static bool s_is_rest_flow;

static ui_screen_id_t s_sleep_ids[3] = {
    UI_SCREEN_SLEEP_WAKE,
    UI_SCREEN_SLEEP_REST_END,
    UI_SCREEN_SLEEP_WIND_DOWN,
};

static ui_screen_id_t s_rest_ids[2] = {
    UI_SCREEN_REST_REST_END,
    UI_SCREEN_REST_WIND_DOWN,
};

static const char *s_sleep_titles[3] = {
    "Sleep: Wake up Time",
    "Sleep: Rest End Time",
    "Sleep: Wind Down Time",
};

static const char *s_rest_titles[2] = {
    "Rest: Rest End Time",
    "Rest: Wind Down Time",
};

static int screen_index(ui_screen_id_t id)
{
    for (int i = 0; i < 3; i++) {
        if (s_sleep_ids[i] == id) {
            return i;
        }
    }
    for (int i = 0; i < 2; i++) {
        if (s_rest_ids[i] == id) {
            return i + 3;
        }
    }
    return 0;
}

static void value_refresh(int idx)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%u s", (unsigned)s_wizard_vals[idx]);
    lv_label_set_text(lbl_values[idx], buf);
}

static void minus_cb(lv_event_t *e)
{
    int idx = (int)(uintptr_t)lv_event_get_user_data(e);
    if (s_wizard_vals[idx] >= 60) {
        s_wizard_vals[idx] -= 60;
    }
    value_refresh(idx);
    ui_nav_reset_idle_timer();
}

static void plus_cb(lv_event_t *e)
{
    int idx = (int)(uintptr_t)lv_event_get_user_data(e);
    s_wizard_vals[idx] += 60;
    value_refresh(idx);
    ui_nav_reset_idle_timer();
}

static void finish_sleep_wizard(void)
{
    app_config_t *cfg = app_config_get();
    app_runtime_t *rt = app_runtime_get();
    cfg->sleep_sec = s_wizard_vals[0];
    cfg->rest_sec = s_wizard_vals[1];
    cfg->wind_down_sec = s_wizard_vals[2];
    rt->current_mode = APP_MODE_WAKE;
    rt->cycle_active = true;
    rt->mode_remaining_sec = cfg->wind_down_sec > 0 ? cfg->wind_down_sec : cfg->sleep_sec;
    app_config_save_schedule();
    ui_nav_go(UI_SCREEN_TOD_BRIGHT);
}

static void finish_rest_wizard(void)
{
    app_config_t *cfg = app_config_get();
    app_runtime_t *rt = app_runtime_get();
    cfg->rest_sec = s_wizard_vals[3];
    cfg->wind_down_sec = s_wizard_vals[4];
    cfg->sleep_sec = 0;
    rt->current_mode = APP_MODE_WAKE;
    rt->cycle_active = true;
    rt->mode_remaining_sec = cfg->wind_down_sec > 0 ? cfg->wind_down_sec : cfg->rest_sec;
    app_config_save_schedule();
    ui_nav_go(UI_SCREEN_TOD_BRIGHT);
}

static void next_cb(lv_event_t *e)
{
    ui_screen_id_t id = (ui_screen_id_t)(uintptr_t)lv_event_get_user_data(e);
    int idx = screen_index(id);

    if (id == UI_SCREEN_SLEEP_WAKE) {
        ui_nav_go(UI_SCREEN_SLEEP_REST_END);
    } else if (id == UI_SCREEN_SLEEP_REST_END) {
        ui_nav_go(UI_SCREEN_SLEEP_WIND_DOWN);
    } else if (id == UI_SCREEN_SLEEP_WIND_DOWN) {
        finish_sleep_wizard();
    } else if (id == UI_SCREEN_REST_REST_END) {
        ui_nav_go(UI_SCREEN_REST_WIND_DOWN);
    } else if (id == UI_SCREEN_REST_WIND_DOWN) {
        finish_rest_wizard();
    }
    (void)idx;
}

static void back_cb(lv_event_t *e)
{
    ui_screen_id_t id = (ui_screen_id_t)(uintptr_t)lv_event_get_user_data(e);
    if (id == UI_SCREEN_SLEEP_WAKE) {
        ui_nav_go(UI_SCREEN_MENU);
    } else if (id == UI_SCREEN_SLEEP_REST_END) {
        ui_nav_go(UI_SCREEN_SLEEP_WAKE);
    } else if (id == UI_SCREEN_SLEEP_WIND_DOWN) {
        ui_nav_go(UI_SCREEN_SLEEP_REST_END);
    } else if (id == UI_SCREEN_REST_REST_END) {
        ui_nav_go(UI_SCREEN_MENU);
    } else if (id == UI_SCREEN_REST_WIND_DOWN) {
        ui_nav_go(UI_SCREEN_REST_REST_END);
    }
}

static void build_wizard_screen(lv_obj_t *screens[UI_SCREEN_COUNT], ui_screen_id_t id,
                                const char *title, int idx)
{
    const ui_theme_t *t = ui_theme_get();
    lv_obj_t *scr = ui_widgets_create_screen();
    screens[id] = scr;
    s_screens[idx] = scr;

    ui_widgets_create_title(scr, title);

    lv_obj_t *box = ui_widgets_create_purple_box(scr, 400, 100, 160, 220, false);
    lbl_values[idx] = lv_label_create(box);
    lv_obj_set_style_text_color(lbl_values[idx], t->white, 0);
    lv_obj_set_style_text_font(lbl_values[idx], &lv_font_montserrat_26, 0);
    lv_obj_center(lbl_values[idx]);
    s_wizard_vals[idx] = 300;
    value_refresh(idx);

    lv_obj_t *minus = lv_button_create(scr);
    lv_obj_set_size(minus, 80, 80);
    lv_obj_set_pos(minus, 200, 380);
    lv_obj_set_style_radius(minus, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(minus, t->keypad, 0);
    lv_obj_t *ml = lv_label_create(minus);
    lv_label_set_text(ml, "-");
    lv_obj_set_style_text_color(ml, t->white, 0);
    lv_obj_center(ml);
    lv_obj_add_event_cb(minus, minus_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)idx);

    lv_obj_t *plus = lv_button_create(scr);
    lv_obj_set_size(plus, 80, 80);
    lv_obj_set_pos(plus, 520, 380);
    lv_obj_set_style_radius(plus, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(plus, t->keypad, 0);
    lv_obj_t *pl = lv_label_create(plus);
    lv_label_set_text(pl, "+");
    lv_obj_set_style_text_color(pl, t->white, 0);
    lv_obj_center(pl);
    lv_obj_add_event_cb(plus, plus_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)idx);

    lv_obj_t *back = ui_widgets_create_side_btn(scr, true, 36, 330, NULL);
    lv_obj_add_event_cb(back, back_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)id);
    lv_obj_t *next = ui_widgets_create_side_next(scr, UI_DISP - 36 - 64, 330);
    lv_obj_add_event_cb(next, next_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)id);
}

void ui_screen_schedule_build(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    build_wizard_screen(screens, UI_SCREEN_SLEEP_WAKE, s_sleep_titles[0], 0);
    build_wizard_screen(screens, UI_SCREEN_SLEEP_REST_END, s_sleep_titles[1], 1);
    build_wizard_screen(screens, UI_SCREEN_SLEEP_WIND_DOWN, s_sleep_titles[2], 2);
    build_wizard_screen(screens, UI_SCREEN_REST_REST_END, s_rest_titles[0], 3);
    build_wizard_screen(screens, UI_SCREEN_REST_WIND_DOWN, s_rest_titles[1], 4);
    (void)s_is_rest_flow;
}

uint32_t ui_screen_schedule_get_sec(void)
{
    return s_wizard_vals[0];
}

void ui_screen_schedule_set_sec(uint32_t sec)
{
    s_wizard_vals[0] = sec;
}
