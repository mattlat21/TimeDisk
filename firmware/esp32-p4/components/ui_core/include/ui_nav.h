/**
 * @file ui_nav.h
 * @brief Screen identifiers and navigation API (no LVGL screen implementations).
 *
 * Kept in ui_core so ui_screens and ui_nav can depend on shared types without
 * a circular component dependency.
 */

#pragma once

#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    UI_SCREEN_SPLASH = 0,
    UI_SCREEN_STARTUP_THEME,
    UI_SCREEN_STARTUP_SSID,
    UI_SCREEN_STARTUP_PASSWORD,
    UI_SCREEN_STARTUP_TIMEZONE,
    UI_SCREEN_LOADING,
    UI_SCREEN_TOD_BRIGHT,
    UI_SCREEN_TOD_DIM,
    UI_SCREEN_ADULT_AUTH,
    UI_SCREEN_MENU,
    UI_SCREEN_SETTINGS,
    UI_SCREEN_TIMER_DURATION,
    UI_SCREEN_TIMER_STYLE,
    UI_SCREEN_TIMER_BRIGHT,
    UI_SCREEN_TIMER_DIM,
    UI_SCREEN_TIMER_TRIGGERED,
    UI_SCREEN_CONFIRM_END_TIMER,
    UI_SCREEN_SLEEP_WAKE,
    UI_SCREEN_SLEEP_REST_END,
    UI_SCREEN_SLEEP_WIND_DOWN,
    UI_SCREEN_REST_REST_END,
    UI_SCREEN_REST_WIND_DOWN,
    UI_SCREEN_CONFIRM_SWITCH_WAKE,
    UI_SCREEN_COUNT,
} ui_screen_id_t;

typedef enum {
    AA_STEP_NONE = 0,
    AA_STEP_PIN,
    AA_STEP_MATHS,
    AA_STEP_DONE,
} aa_step_t;

typedef struct {
    bool active;
    aa_step_t step;
    ui_screen_id_t entry_screen;
    ui_screen_id_t on_pass_screen;
    uint32_t last_input_ms;
    int maths_a;
    int maths_b;
    int maths_sum;
    char pin_buf[5];
    char answer_buf[4];
} aa_session_t;

void ui_nav_init(void);
void ui_nav_go(ui_screen_id_t screen);
ui_screen_id_t ui_nav_current(void);
aa_session_t *ui_nav_aa_session(void);

void ui_nav_reset_idle_timer(void);
void ui_nav_aa_touch(void);
void ui_nav_start_aa(ui_screen_id_t entry, ui_screen_id_t on_pass);
void ui_nav_aa_pass(void);
void ui_nav_aa_cancel(void);
void ui_nav_aa_new_maths(void);

void ui_nav_set_brightness(uint8_t pct);
void ui_nav_apply_dim(bool dim);

/** Fade TOD to dim over 5 s (from bright idle timeout). */
void ui_nav_tod_fade_to_dim(void);
/** Fade TOD to bright over 1 s (tap on dim screen). */
void ui_nav_tod_fade_to_bright(void);

/** Begin automatic Wake→… mode cycle after Sleep/Rest schedule wizard. */
void mode_engine_start_cycle(void);

/** Stop the automatic cycle and return to Wake mode. */
void mode_engine_switch_to_wake(void);
