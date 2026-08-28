/**
 * @file ui_spiffs_pixelart.h
 * @brief Draw small SPIFFS RGB565 assets with integer block upscale (nearest-neighbor).
 */

#pragma once

#include "lvgl.h"

/** Max source dimension cached in RAM for integer block upscale (e.g. 180×180 wake art). */
#define UI_SPIFFS_PIXELART_CACHE_MAX 256

/**
 * True when @p path decodes to a small image that divides @p dest_w × @p dest_h evenly.
 */
bool ui_spiffs_pixelart_is_block_scalable(const char *path, int32_t dest_w, int32_t dest_h);

/** Decode a block-scalable asset into RAM so later draws skip SPIFFS I/O. */
void ui_spiffs_pixelart_cache_preload(const char *path);

/** Drop the RAM cache (e.g. on asset change). */
void ui_spiffs_pixelart_cache_drop(void);

/**
 * Draw @p path into @p dest using block nearest-neighbor when scalable, else 1:1 file blit.
 * Use for full-bleed backgrounds (image fills @p dest exactly).
 */
void ui_spiffs_pixelart_draw(lv_layer_t *layer, const lv_area_t *dest, const char *path);

/**
 * Draw the entire image scaled to fit inside @p dest (letterboxed if aspect differs).
 * Uses block nearest-neighbor when evenly divisible; otherwise LVGL scaled blit.
 */
void ui_spiffs_pixelart_draw_contain(lv_layer_t *layer, const lv_area_t *dest, const char *path);
