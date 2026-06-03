/**
 * @file ui_assets.h
 * @brief Embedded LVGL image descriptors (LVGLImage.py from assets PNGs).
 */

#pragma once

#include "lvgl.h"

extern const lv_image_dsc_t splash;

/** Full-screen Time of Day backgrounds (720×720 RGB565). */
extern const lv_image_dsc_t tod_wake;
extern const lv_image_dsc_t tod_winddown;
extern const lv_image_dsc_t tod_sleep;
extern const lv_image_dsc_t tod_rest;

/** A8 wedge silhouettes — tint at runtime via image_recolor. */
extern const lv_image_dsc_t wedge_shape_left;
extern const lv_image_dsc_t wedge_shape_right;
extern const lv_image_dsc_t wedge_shape_wide;

/** White icon overlays, composed on top of a wedge button. */
extern const lv_image_dsc_t icon_wedge_cancel;
extern const lv_image_dsc_t icon_wedge_confirm;
extern const lv_image_dsc_t icon_wedge_next;
extern const lv_image_dsc_t icon_wedge_settings;
extern const lv_image_dsc_t icon_wedge_menu_wide;
