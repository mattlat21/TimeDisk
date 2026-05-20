/**
 * @file ui_screens_registry.h
 * @brief Declarations for all LVGL screens and lifecycle hooks.
 */

#pragma once

#include "lvgl.h"
#include "ui_nav.h"

void ui_screen_splash_build(lv_obj_t *screens[UI_SCREEN_COUNT]);
void ui_screen_startup_wifi_wizard_build(lv_obj_t *screens[UI_SCREEN_COUNT]);
void ui_screen_startup_timezone_wizard_build(lv_obj_t *screens[UI_SCREEN_COUNT]);
void ui_screen_startup_timezone_wizard_on_show(void);
void ui_screen_startup_timezone_wizard_on_hide(void);
void ui_screen_loading_build(lv_obj_t *screens[UI_SCREEN_COUNT]);
void ui_screen_tod_build(lv_obj_t *screens[UI_SCREEN_COUNT]);
void ui_screen_aa_build(lv_obj_t *screens[UI_SCREEN_COUNT]);
void ui_screen_menu_build(lv_obj_t *screens[UI_SCREEN_COUNT]);
void ui_screen_timer_build(lv_obj_t *screens[UI_SCREEN_COUNT]);
void ui_screen_schedule_build(lv_obj_t *screens[UI_SCREEN_COUNT]);
void ui_screen_settings_build(lv_obj_t *screens[UI_SCREEN_COUNT]);

void ui_screens_build_all(lv_obj_t *screens[UI_SCREEN_COUNT]);

void ui_screen_splash_on_show(void);
void ui_screen_loading_on_show(void);
void ui_screen_loading_set_status(const char *text);
void ui_screen_tod_on_show(bool dim);
void ui_screen_aa_on_show(void);
void ui_screen_aa_show_pin(void);
void ui_screen_aa_show_maths(void);
void ui_screen_aa_reset_pin(void);
void ui_screen_aa_reset_maths(void);
void ui_screen_aa_update_maths_labels(int a, int b);
bool ui_screen_aa_pin_ok_pressed(void);
bool ui_screen_aa_maths_ok_pressed(void);
void ui_screen_timer_on_show(ui_screen_id_t id);
void ui_screen_timer_tick(void);
void ui_screen_startup_wifi_wizard_ssid_get_text(char *out, size_t len);
void ui_screen_startup_wifi_wizard_password_get_text(char *out, size_t len);
uint32_t ui_screen_duration_get_sec(void);
uint8_t ui_screen_style_get_id(void);
uint32_t ui_screen_schedule_get_sec(void);
void ui_screen_schedule_set_sec(uint32_t sec);
