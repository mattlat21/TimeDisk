/**
 * @file ui_widgets.h
 * @brief Reusable LVGL controls: circular screen chrome, titles, boxes, side buttons, keypad.
 */

#pragma once

#include "lvgl.h"

/** Apply standard TimeDisk circular panel styling (black fill + purple ring). */
void ui_widgets_style_circle_panel(lv_obj_t *obj);

/** Create a loadable LVGL screen with default 6 px ring border. */
lv_obj_t *ui_widgets_create_screen(void);

lv_obj_t *ui_widgets_create_title(lv_obj_t *parent, const char *text);
lv_obj_t *ui_widgets_create_purple_box(lv_obj_t *parent, int w, int h, int x, int y, bool outline_only);
lv_obj_t *ui_widgets_create_side_btn(lv_obj_t *parent, bool is_back, int x, int y, const char *label);
lv_obj_t *ui_widgets_create_side_next(lv_obj_t *parent, int x, int y);
lv_obj_t *ui_widgets_create_keypad_btn(lv_obj_t *parent, const char *txt, int x, int y, int size);
void ui_widgets_add_numeric_keypad(lv_obj_t *parent, int start_y, lv_event_cb_t digit_cb, void *user_ctx);
