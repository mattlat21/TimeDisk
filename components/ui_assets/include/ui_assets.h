/**
 * @file ui_assets.h
 * @brief Embedded LVGL image descriptors (LVGLImage.py from assets PNGs).
 */

#pragma once

#include "lvgl.h"

extern const lv_image_dsc_t splash;

/** A8 wedge silhouettes — tint at runtime via image_recolor. */
extern const lv_image_dsc_t wedge_shape_left;
extern const lv_image_dsc_t wedge_shape_right;

/** White icon overlays (209×106), composed on top of a wedge button. */
extern const lv_image_dsc_t icon_wedge_cancel;
extern const lv_image_dsc_t icon_wedge_confirm;
extern const lv_image_dsc_t icon_wedge_settings;
