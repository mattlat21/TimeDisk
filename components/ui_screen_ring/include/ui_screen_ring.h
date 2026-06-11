/**
 * @file ui_screen_ring.h
 * @brief Primary-coloured circular screen chrome (14 px theme ring border).
 */

#pragma once

#include "lvgl.h"

/** Apply circular clip + theme ring border (standard ringed screens). */
void ui_screen_ring_apply(lv_obj_t *screen);

/** Remove ring border (e.g. splash). */
void ui_screen_ring_clear(lv_obj_t *screen);

/** Re-apply ring colour after theme change (border width unchanged). */
void ui_screen_ring_refresh(lv_obj_t *screen);

/**
 * Draw the theme ring above screen children (e.g. menu buttons under the bezel band).
 * Clears the screen object's own border so the ring is not drawn twice.
 */
void ui_screen_ring_raise_overlay(lv_obj_t *screen);
