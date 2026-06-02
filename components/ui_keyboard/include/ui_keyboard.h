/**
 * @file ui_keyboard.h
 * @brief Four-layout on-screen keyboard (lower / upper / number / symbol), four rows.
 *
 * Default key row placement matches startup_wizard_ssid.svg (Wi-Fi password screen).
 * parent may be any object on the target screen; layers are children of the root screen.
 */

#pragma once

#include "lvgl.h"
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    UI_KEYBOARD_MODE_LOWER = 0,
    UI_KEYBOARD_MODE_UPPER,
    UI_KEYBOARD_MODE_NUMBER,
    UI_KEYBOARD_MODE_SYMBOL,
} ui_keyboard_mode_t;

#define UI_KEYBOARD_MODE_COUNT 4

typedef struct {
    char *buf;
    size_t buf_len;
    lv_obj_t *label;
    ui_keyboard_mode_t initial_mode;
    void (*on_activity)(void *user_data);
    void *user_data;
} ui_keyboard_config_t;

typedef struct ui_keyboard ui_keyboard_t;

void ui_keyboard_module_init(void);
ui_keyboard_t *ui_keyboard_create(lv_obj_t *parent, const ui_keyboard_config_t *config);
/** Create keyboard as a screen-root overlay (absolute LCD positioning). */
ui_keyboard_t *ui_keyboard_create_overlay(lv_obj_t *screen, const ui_keyboard_config_t *config);
void ui_keyboard_destroy(ui_keyboard_t *kb);
ui_keyboard_mode_t ui_keyboard_get_mode(const ui_keyboard_t *kb);
void ui_keyboard_set_mode(ui_keyboard_t *kb, ui_keyboard_mode_t mode);

/** Re-bind target buffer/label without recreating the keyboard. */
void ui_keyboard_bind(ui_keyboard_t *kb, const ui_keyboard_config_t *config);

/** Show/hide the keyboard overlays (all layers + mode button). */
void ui_keyboard_set_visible(ui_keyboard_t *kb, bool visible);

/** Expose overlay objects for screen-level overlay registries. */
lv_obj_t *ui_keyboard_get_layer(ui_keyboard_t *kb, ui_keyboard_mode_t mode);
lv_obj_t *ui_keyboard_get_mode_button(ui_keyboard_t *kb);
