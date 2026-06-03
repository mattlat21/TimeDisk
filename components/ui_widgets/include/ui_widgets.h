/**
 * @file ui_widgets.h
 * @brief Reusable LVGL controls: circular screen chrome, titles, boxes, side buttons, keypad.
 */

#pragma once

#include "lvgl.h"

/** Apply standard TimeDisk circular panel styling (black fill + purple ring). */
void ui_widgets_style_circle_panel(lv_obj_t *obj);

/** Circular panel with theme background but no chrome ring (e.g. active timer countdown). */
void ui_widgets_style_circle_panel_no_ring(lv_obj_t *obj);

/** Apply the standard 14 px purple circular edge ring (theme ring colour). */
void ui_widgets_apply_screen_ring(lv_obj_t *screen);

/** Create a loadable LVGL screen with standard 14 px ring border. */
lv_obj_t *ui_widgets_create_screen(void);

/** Create a loadable LVGL screen without the chrome ring border. */
lv_obj_t *ui_widgets_create_screen_no_ring(void);

/**
 * Full-bleed circular backdrop behind screen content (ringless countdown screens).
 * Call after all children are created so it stays at the back of the z-order.
 */
void ui_widgets_attach_screen_edge_fill(lv_obj_t *screen);

/** Re-send the edge backdrop to the bottom of the screen z-order (after adding other children). */
void ui_widgets_send_edge_fill_to_back(lv_obj_t *screen);

lv_obj_t *ui_widgets_create_title(lv_obj_t *parent, const char *text);
lv_obj_t *ui_widgets_create_purple_box(lv_obj_t *parent, int w, int h, int x, int y, bool outline_only);
lv_obj_t *ui_widgets_create_side_btn(lv_obj_t *parent, bool is_back, int x, int y, const char *label);
lv_obj_t *ui_widgets_create_side_next(lv_obj_t *parent, int x, int y);
lv_obj_t *ui_widgets_create_keypad_btn(lv_obj_t *parent, const char *txt, int x, int y, int size);
