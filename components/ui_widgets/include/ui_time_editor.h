/**
 * @file ui_time_editor.h
 * @brief End-time editor with separate hour and minute steppers (synced to duration).
 */

#pragma once

#include "lvgl.h"
#include <stdint.h>

#define UI_TIME_EDITOR_BOX_W     400
#define UI_TIME_EDITOR_BOX_H     130
#define UI_TIME_EDITOR_BOX_Y     315
#define UI_TIME_EDITOR_STEPPER   44
#define UI_TIME_EDITOR_COL_W     72

typedef void (*ui_time_editor_cb_t)(void *user_data);

typedef struct {
    uint32_t *value_sec;
    /** Added to *value_sec when computing end time (default 0). */
    uint32_t end_time_offset_sec;
    uint32_t max_sec;
    uint32_t min_sec;
    int box_y;
    int box_w;
    int box_h;
    ui_time_editor_cb_t on_change;
    void *user_data;
} ui_time_editor_cfg_t;

typedef struct {
    lv_obj_t *box;
    lv_obj_t *lbl_hour;
    lv_obj_t *lbl_min;
    lv_obj_t *lbl_ampm;
    lv_obj_t *btn_h_minus;
    lv_obj_t *btn_h_plus;
    lv_obj_t *btn_m_minus;
    lv_obj_t *btn_m_plus;
} ui_time_editor_t;

typedef struct {
    ui_time_editor_t editor;
    ui_time_editor_cfg_t cfg;
} ui_time_editor_bundle_t;

void ui_time_editor_create(lv_obj_t *parent, ui_time_editor_bundle_t *bundle);
void ui_time_editor_refresh(const ui_time_editor_t *ed, const ui_time_editor_cfg_t *cfg);
void ui_time_editor_set_visible(const ui_time_editor_t *ed, bool visible);
