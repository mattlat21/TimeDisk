#pragma once

#include "lvgl.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

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

/** Call once before any ui_keyboard_create() (idempotent). */
void ui_keyboard_module_init(void);

/**
 * Four-layout on-screen keyboard (lower / upper / number / symbol).
 * Binds to buf + label; updates label on each key. on_activity runs on key/mode press.
 */
ui_keyboard_t *ui_keyboard_create(lv_obj_t *parent, const ui_keyboard_config_t *config);

ui_keyboard_mode_t ui_keyboard_get_mode(const ui_keyboard_t *kb);

void ui_keyboard_set_mode(ui_keyboard_t *kb, ui_keyboard_mode_t mode);

#ifdef __cplusplus
}
#endif
