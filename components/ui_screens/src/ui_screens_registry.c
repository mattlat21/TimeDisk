#include "ui_screens_registry.h"

void ui_screens_build_all(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    ui_screen_splash_build(screens);
    ui_screen_startup_wifi_wizard_build(screens);
    ui_screen_loading_build(screens);
    ui_screen_tod_build(screens);
    ui_screen_aa_build(screens);
    ui_screen_menu_build(screens);
    ui_screen_timer_build(screens);
    ui_screen_schedule_build(screens);
    ui_screen_settings_build(screens);
}
