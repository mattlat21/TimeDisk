/**
 * @file ui_large_time_picker.h
 * @brief Large time picker with hour/minute stepper columns (synced to duration).
 */

#pragma once

#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>

#define UI_LARGE_TIME_PICKER_BOX_Y     180
#define UI_LARGE_TIME_PICKER_X_OFFSET_WF 36
#define UI_LARGE_TIME_PICKER_COL_W     176
#define UI_LARGE_TIME_PICKER_STEP_H    88
#define UI_LARGE_TIME_PICKER_VAL_H     112
#define UI_LARGE_TIME_PICKER_COLON_W   48
#define UI_LARGE_TIME_PICKER_AMPM_W    132
#define UI_LARGE_TIME_PICKER_AMPM_X_OFFSET 12
#define UI_LARGE_TIME_PICKER_CORNER_R  20
#define UI_LARGE_TIME_PICKER_BTN_GAP   7
#define UI_LARGE_TIME_PICKER_FONT_64_LETTER_SPACE 2
#define UI_LARGE_TIME_PICKER_FONT_64_LINE_H    57

typedef void (*ui_large_time_picker_cb_t)(void *user_data);

/** END_TIME: wall-clock end (needs valid RTC). DURATION: H:MM length, no clock. */
typedef enum {
    UI_LARGE_TIME_PICKER_MODE_END_TIME = 0,
    UI_LARGE_TIME_PICKER_MODE_DURATION,
} ui_large_time_picker_mode_t;

typedef struct {
    uint32_t *value_sec;
    /** Added to *value_sec when computing end time (default 0). */
    uint32_t end_time_offset_sec;
    uint32_t max_sec;
    uint32_t min_sec;
    int box_y;
    ui_large_time_picker_mode_t mode;
    ui_large_time_picker_cb_t on_change;
    void *user_data;
} ui_large_time_picker_cfg_t;

typedef struct {
    lv_obj_t *box;
    lv_obj_t *lbl_hour;
    lv_obj_t *lbl_min;
    lv_obj_t *lbl_ampm;
    lv_obj_t *btn_h_minus;
    lv_obj_t *btn_h_plus;
    lv_obj_t *btn_m_minus;
    lv_obj_t *btn_m_plus;
} ui_large_time_picker_t;

typedef struct {
    ui_large_time_picker_t picker;
    ui_large_time_picker_cfg_t cfg;
} ui_large_time_picker_bundle_t;

void ui_large_time_picker_create(lv_obj_t *parent, ui_large_time_picker_bundle_t *bundle);
void ui_large_time_picker_refresh(const ui_large_time_picker_t *picker, const ui_large_time_picker_cfg_t *cfg);
void ui_large_time_picker_set_visible(const ui_large_time_picker_t *picker, bool visible);
