/**
 * @file ui_duration_editor.h
 * @brief Shared duration stepper UI (timer Set Duration layout).
 */

#pragma once

#include "lvgl.h"
#include <stdbool.h>
#include <stdint.h>

#define UI_DURATION_EDITOR_BOX_W     400
#define UI_DURATION_EDITOR_BOX_H     120
/** @deprecated box_x is ignored; editor centres horizontally on the full LCD. */
#define UI_DURATION_EDITOR_BOX_X     160
#define UI_DURATION_EDITOR_BOX_Y     200
#define UI_DURATION_EDITOR_STEPPER   80
#define UI_DURATION_EDITOR_GAP       20
#define UI_DURATION_EDITOR_STEP_SEC  60
#define UI_DURATION_EDITOR_MAX_SEC   3600
#define UI_DURATION_EDITOR_SLIDER_W  400
#define UI_DURATION_EDITOR_SLIDER_H  8
#define UI_DURATION_EDITOR_SLIDER_GAP 24

/** Wind-down schedule screen: compact minute box, large steppers, slider. */
#define UI_DURATION_EDITOR_WD_BOX_Y_WF     255
#define UI_DURATION_EDITOR_WD_BOX_W        280
#define UI_DURATION_EDITOR_WD_BOX_PAD      20
#define UI_DURATION_EDITOR_WD_VALUE_LINE_H 72
#define UI_DURATION_EDITOR_WD_UNIT_LINE_H  43
#define UI_DURATION_EDITOR_WD_BOX_H \
    (UI_DURATION_EDITOR_WD_BOX_PAD + UI_DURATION_EDITOR_WD_VALUE_LINE_H + \
     UI_DURATION_EDITOR_WD_UNIT_LINE_H + UI_DURATION_EDITOR_WD_BOX_PAD)
#define UI_DURATION_EDITOR_WD_STEPPER      104
#define UI_DURATION_EDITOR_WD_GAP          24
#define UI_DURATION_EDITOR_WD_SLIDER_W     440
#define UI_DURATION_EDITOR_WD_SLIDER_TRACK 9
#define UI_DURATION_EDITOR_WD_SLIDER_KNOB  40
#define UI_DURATION_EDITOR_WD_SLIDER_GAP   64
/** Max gross/net rest duration in schedule wizards and Settings → Schedule. */
#define UI_SCHEDULE_REST_MAX_SEC     (24U * 3600U)

typedef void (*ui_duration_editor_cb_t)(void *user_data);

/** When set, overrides fixed @p step_sec (e.g. timer tiered +/- steps). */
typedef uint32_t (*ui_duration_editor_step_fn_t)(uint32_t value_sec, void *user_data);

typedef enum {
    UI_DURATION_DISPLAY_MINUTES = 0,
    /** Shows seconds when under 1 minute (timer Set Duration). */
    UI_DURATION_DISPLAY_HUMAN,
    /** Shows a 0–100 percentage value. */
    UI_DURATION_DISPLAY_PERCENT,
    /** Large value with a separate unit label: sec, mm:ss+min, or minutes+min. */
    UI_DURATION_DISPLAY_WIND_DOWN,
} ui_duration_display_t;

typedef enum {
    UI_DURATION_EDITOR_STYLE_DEFAULT = 0,
    UI_DURATION_EDITOR_STYLE_WIND_DOWN,
} ui_duration_editor_style_t;

typedef struct {
    uint32_t *value_sec;
    int box_x;
    int box_y;
    int box_w;
    int box_h;
    bool show_end_time;
    /** Horizontal slider below the value box (maps min_sec..max_sec in step_sec). */
    bool show_slider;
    /** Added to *value_sec when computing the end-time subtitle (default 0). */
    uint32_t end_time_offset_sec;
    uint32_t max_sec;
    uint32_t min_sec;
    uint32_t step_sec;
    ui_duration_editor_style_t style;
    ui_duration_display_t display;
    ui_duration_editor_step_fn_t get_step_sec;
    ui_duration_editor_cb_t on_change;
    void *user_data;
} ui_duration_editor_cfg_t;

typedef struct {
    lv_obj_t *box;
    lv_obj_t *btn_minus;
    lv_obj_t *btn_plus;
    lv_obj_t *lbl_value;
    lv_obj_t *lbl_unit;
    lv_obj_t *lbl_subtitle;
    lv_obj_t *slider;
} ui_duration_editor_t;

/** Caller-owned; passed to create and used by stepper event handlers. */
typedef struct {
    ui_duration_editor_t editor;
    ui_duration_editor_cfg_t cfg;
} ui_duration_editor_bundle_t;

void ui_duration_editor_create(lv_obj_t *parent, ui_duration_editor_bundle_t *bundle);

void ui_duration_editor_refresh(const ui_duration_editor_t *ed, const ui_duration_editor_cfg_t *cfg);

/** Show or hide all editor widgets (box, steppers, optional subtitle). */
void ui_duration_editor_set_visible(const ui_duration_editor_t *ed, bool visible);

/** Re-apply theme colours to subtitle labels (e.g. after primary/secondary change). */
void ui_duration_editor_apply_theme(const ui_duration_editor_t *ed);
