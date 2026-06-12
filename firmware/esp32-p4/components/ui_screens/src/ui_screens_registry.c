#include "ui_screens_registry.h"
#include "ui_theme.h"

void ui_screen_loading_apply_theme(void);
void ui_screen_tod_apply_theme(void);
void ui_screen_menu_apply_theme(void);
void ui_screen_timer_apply_theme(void);
void ui_screen_schedule_apply_theme(void);
void ui_screen_settings_apply_theme(void);
void ui_screen_aa_apply_theme(void);
void ui_screen_startup_theme_wizard_apply_theme(void);
void ui_screen_startup_wifi_wizard_apply_theme(void);
void ui_screen_startup_timezone_wizard_apply_theme(void);

void ui_theme_apply(void)
{
    ui_theme_init();
    ui_screen_loading_apply_theme();
    ui_screen_tod_apply_theme();
    ui_screen_menu_apply_theme();
    ui_screen_timer_apply_theme();
    ui_screen_schedule_apply_theme();
    ui_screen_settings_apply_theme();
    ui_screen_aa_apply_theme();
    ui_screen_startup_theme_wizard_apply_theme();
    ui_screen_startup_wifi_wizard_apply_theme();
    ui_screen_startup_timezone_wizard_apply_theme();
}

void ui_screens_build_all(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    ui_screen_splash_build(screens);
    ui_screen_startup_theme_wizard_build(screens);
    ui_screen_startup_wifi_wizard_build(screens);
    ui_screen_startup_timezone_wizard_build(screens);
    ui_screen_loading_build(screens);
    ui_screen_tod_build(screens);
    ui_screen_aa_build(screens);
    ui_screen_menu_build(screens);
    ui_screen_timer_build(screens);
    ui_screen_schedule_build(screens);
    ui_screen_settings_build(screens);
}
