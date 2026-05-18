#include "ui_screens_registry.h"

extern "C" void ui_screens_build_all(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    ui_screen_splash_build(screens);
    ui_screen_wizard_build(screens);
    ui_screen_loading_build(screens);
    ui_screen_tod_build(screens);
    ui_screen_aa_build(screens);
    ui_screen_menu_build(screens);
    ui_screen_timer_build(screens);
    ui_screen_schedule_build(screens);
    ui_screen_settings_build(screens);
}
