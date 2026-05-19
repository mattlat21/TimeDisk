/**
 * @file ui_lines.h
 * @brief Guide lines (lv_line) for layout debugging and Wi-Fi wizard alignment.
 *
 * lv_line_set_points() stores a pointer — each line needs its own point pair.
 * Call ui_lines_reset() once before adding lines on a screen.
 */

#pragma once

#include "lvgl.h"

void ui_lines_reset(void);
lv_obj_t *ui_lines_add_vertical(lv_obj_t *parent, int x);
lv_obj_t *ui_lines_add_horizontal(lv_obj_t *parent, int y);
