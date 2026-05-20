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
#define UI_DURATION_EDITOR_BOX_X     160
#define UI_DURATION_EDITOR_BOX_Y     200
#define UI_DURATION_EDITOR_STEPPER   80
#define UI_DURATION_EDITOR_GAP       20
#define UI_DURATION_EDITOR_STEP_SEC  60
#define UI_DURATION_EDITOR_MAX_SEC   3600

typedef void (*ui_duration_editor_cb_t)(void *user_data);

typedef struct {
    uint32_t *value_sec;
    int box_x;
    int box_y;
    int box_w;
    int box_h;
    bool show_end_time;
    uint32_t max_sec;
    uint32_t step_sec;
    ui_duration_editor_cb_t on_change;
    void *user_data;
} ui_duration_editor_cfg_t;

typedef struct {
    lv_obj_t *lbl_value;
    lv_obj_t *lbl_subtitle;
} ui_duration_editor_t;

/** Caller-owned; passed to create and used by stepper event handlers. */
typedef struct {
    ui_duration_editor_t editor;
    ui_duration_editor_cfg_t cfg;
} ui_duration_editor_bundle_t;

void ui_duration_editor_create(lv_obj_t *parent, ui_duration_editor_bundle_t *bundle);

void ui_duration_editor_refresh(const ui_duration_editor_t *ed, const ui_duration_editor_cfg_t *cfg);

/** Re-apply theme colours to subtitle labels (e.g. after primary/secondary change). */
void ui_duration_editor_apply_theme(const ui_duration_editor_t *ed);
