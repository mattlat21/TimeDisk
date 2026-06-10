/**
 * @file ui_wireframe_guides.h
 * @brief Guide lines (lv_line) for layout debugging and Wi-Fi wizard alignment.
 *
 * lv_line_set_points() stores a pointer — each line needs its own point pair.
 * Call ui_wireframe_guides_reset() once before adding lines on a screen.
 */

#pragma once

#include "lvgl.h"

void ui_wireframe_guides_reset(void);
lv_obj_t *ui_wireframe_guides_add_vertical(lv_obj_t *parent, int x);
lv_obj_t *ui_wireframe_guides_add_horizontal(lv_obj_t *parent, int y);

/**
 * Debug overlay on @p parent screen: 1px white crosshair (±10px vertical guides) + 700px/680px
 * circles, centered on the
 * full LCD (720×720). Placed above all screen children, including wedges and the theme ring.
 */
void ui_wireframe_guides_add_center_crosshair(lv_obj_t *parent);
