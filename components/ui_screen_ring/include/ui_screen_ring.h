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
