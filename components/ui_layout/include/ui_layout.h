/**
 * @file ui_layout.h
 * @brief Display geometry: wireframe ↔ content coordinates (720×720 round LCD).
 *
 * Wireframe coordinates match docs/wireframes/ SVG viewBox (full LCD pixels).
 * Content coordinates are LVGL child positions inside the screen border ring.
 */

#pragma once

#include "lvgl.h"
#include "bsp/display.h"

/** Logical display width/height in pixels (from board BSP). */
#define UI_DISP BSP_LCD_H_RES

#define UI_SCREEN_W  UI_DISP
#define UI_SCREEN_H  UI_DISP
#define UI_SCREEN_CX (UI_SCREEN_W / 2)
#define UI_SCREEN_CY (UI_SCREEN_H / 2)

/** Standard purple edge ring on ringed screens (startup_wizard_ssid.svg / Wi-Fi password). */
#define UI_RING_BORDER 14

/** @deprecated Use UI_RING_BORDER. */
#define UI_RING_BORDER_DEFAULT UI_RING_BORDER
/** @deprecated Use UI_RING_BORDER. */
#define UI_RING_BORDER_WIFI UI_RING_BORDER

#define UI_CONTENT_W(border)  (UI_SCREEN_W - 2 * (border))
#define UI_CONTENT_H(border)  (UI_SCREEN_H - 2 * (border))
#define UI_CONTENT_CX(border) (UI_CONTENT_W(border) / 2)
#define UI_CONTENT_CY(border) (UI_CONTENT_H(border) / 2)

/** Wireframe X/Y → LVGL child position for a screen with uniform border. */
#define UI_WF_X(x, border) ((x) - (border))
#define UI_WF_Y(y, border) ((y) - (border))

int32_t ui_layout_screen_border(const lv_obj_t *screen);
int ui_layout_wf_to_content_x(lv_obj_t *screen, int x_wf);
int ui_layout_wf_to_content_y(lv_obj_t *screen, int y_wf);
void ui_layout_get_content_size(lv_obj_t *parent, int32_t *w_out, int32_t *h_out);
void ui_layout_get_content_center(lv_obj_t *parent, int32_t *cx_out, int32_t *cy_out);

/** Walk parents to the top-level loadable screen (parent == NULL). */
lv_obj_t *ui_layout_find_screen(lv_obj_t *obj);

/**
 * Wireframe LCD (x_wf, y_wf) → position in the root screen content area (same on every screen).
 */
void ui_layout_screen_pos_from_wf(lv_obj_t *screen, int x_wf, int y_wf, int *x_out, int *y_out);

/**
 * Wireframe LCD → position relative to parent (legacy; prefer screen-absolute wedges).
 */
void ui_layout_parent_pos_from_wf(lv_obj_t *parent, int x_wf, int y_wf, int *x_out, int *y_out);
