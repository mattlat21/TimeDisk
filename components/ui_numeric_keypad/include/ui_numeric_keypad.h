/**
 * @file ui_numeric_keypad.h
 * @brief Standard numeric keypad widget (1-9, 0, backspace).
 */
 
#pragma once

#include "lvgl.h"
#include <stdbool.h>

typedef struct ui_numeric_keypad ui_numeric_keypad_t;

typedef struct {
    /**
     * Called for digit keys.
     *
     * - event user_data = user_ctx
     * - digit string is stored on the target button via lv_obj_get_user_data(target) ("0".."9")
     */
    lv_event_cb_t digit_cb;
    /** Called when backspace key pressed. event user_data = user_ctx. */
    lv_event_cb_t backspace_cb;
    /** Passed as LVGL event user_data for all keypad keys. */
    void *user_ctx;
} ui_numeric_keypad_cfg_t;

ui_numeric_keypad_t *ui_numeric_keypad_create(lv_obj_t *parent, const ui_numeric_keypad_cfg_t *cfg);
/**
 * Create keypad as a screen-root overlay with absolute LCD positioning.
 *
 * @param screen Root LVGL screen object (parent == NULL).
 */
ui_numeric_keypad_t *ui_numeric_keypad_create_overlay(lv_obj_t *screen, const ui_numeric_keypad_cfg_t *cfg);
void ui_numeric_keypad_destroy(ui_numeric_keypad_t *kp);

/** Update callbacks / context without recreating the keypad. */
void ui_numeric_keypad_set_cfg(ui_numeric_keypad_t *kp, const ui_numeric_keypad_cfg_t *cfg);

/** Show/hide the keypad container. */
void ui_numeric_keypad_set_visible(ui_numeric_keypad_t *kp, bool visible);

/** Access underlying LVGL container for overlay registries. */
lv_obj_t *ui_numeric_keypad_get_container(ui_numeric_keypad_t *kp);

