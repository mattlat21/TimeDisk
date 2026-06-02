/**
 * @file ui_numeric_keypad.h
 * @brief Standard numeric keypad widget (1-9, 0, backspace).
 */
 
#pragma once

#include "lvgl.h"

typedef struct ui_numeric_keypad ui_numeric_keypad_t;

typedef struct {
    /** Called with user_data = "0".."9" (const char *). */
    lv_event_cb_t digit_cb;
    /** Called when backspace key pressed. user_data = user_ctx. */
    lv_event_cb_t backspace_cb;
    /** Passed as LVGL event user_data for backspace. */
    void *user_ctx;
} ui_numeric_keypad_cfg_t;

ui_numeric_keypad_t *ui_numeric_keypad_create(lv_obj_t *parent, const ui_numeric_keypad_cfg_t *cfg);
void ui_numeric_keypad_destroy(ui_numeric_keypad_t *kp);

