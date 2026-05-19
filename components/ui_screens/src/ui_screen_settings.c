#include "ui_screens_registry.h"
#include "ui_layout.h"
#include "ui_widgets.h"
#include "ui_format.h"
#include "ui_wedge.h"
#include "ui_lines.h"
#include "ui_nav.h"
#include "app_config.h"

static lv_obj_t *s_scr;

static void back_cb(lv_event_t *e)
{
    (void)e;
    app_runtime_t *rt = app_runtime_get();
    if (rt->time_valid) {
        ui_nav_go(UI_SCREEN_MENU);
    } else {
        ui_nav_go(UI_SCREEN_LOADING);
    }
}

void ui_screen_settings_build(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    s_scr = ui_widgets_create_screen();
    screens[UI_SCREEN_SETTINGS] = s_scr;

    ui_widgets_create_title(s_scr, "Settings (stub)");

    lv_obj_t *back = ui_widgets_create_side_btn(s_scr, true, 36, 330, NULL);
    lv_obj_add_event_cb(back, back_cb, LV_EVENT_CLICKED, NULL);
}
