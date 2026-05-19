#pragma once

#include "lvgl.h"
#include "bsp/display.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UI_DISP BSP_LCD_H_RES

/*
 * Coordinate spaces (720×720 circular display):
 *
 * Wireframe / design (UI_SCREEN_*): docs/wireframes/ SVG viewBox — full LCD pixels.
 * Content (LVGL child coords): inside the screen's border ring; origin is top-left of
 * the drawable area. Use ui_common_wf_to_content_*() or UI_WF_*() when placing widgets.
 */
#define UI_SCREEN_W  UI_DISP
#define UI_SCREEN_H  UI_DISP
#define UI_SCREEN_CX (UI_SCREEN_W / 2)
#define UI_SCREEN_CY (UI_SCREEN_H / 2)

/** Ring border widths used by ui_common_create_screen() and wifi wizard screens. */
#define UI_RING_BORDER_DEFAULT 6
#define UI_RING_BORDER_WIFI    14

#define UI_CONTENT_W(border)  (UI_SCREEN_W - 2 * (border))
#define UI_CONTENT_H(border)  (UI_SCREEN_H - 2 * (border))
#define UI_CONTENT_CX(border) (UI_CONTENT_W(border) / 2)
#define UI_CONTENT_CY(border) (UI_CONTENT_H(border) / 2)

/** Wireframe coordinate → LVGL child position on a screen with uniform border. */
#define UI_WF_X(x, border) ((x) - (border))
#define UI_WF_Y(y, border) ((y) - (border))

typedef void (*ui_btn_cb_t)(lv_event_t *e);

/** Bottom-corner wedges from docs/wireframes/startup_wizard_ssid.svg (mirror shapes). */
typedef enum {
    UI_CORNER_WEDGE_CANCEL = 0,  /* Orange, lower-left, X icon */
    UI_CORNER_WEDGE_CONFIRM,     /* Green, lower-right, check icon */
} ui_corner_wedge_type_t;

/** Wireframe bounding box of one wedge (both sides share width/height). */
#define UI_CORNER_WEDGE_W_WF 209
#define UI_CORNER_WEDGE_H_WF 106
#define UI_CORNER_WEDGE_CANCEL_X_WF 142
#define UI_CORNER_WEDGE_CANCEL_Y_WF 589
#define UI_CORNER_WEDGE_CONFIRM_X_WF 376
#define UI_CORNER_WEDGE_CONFIRM_Y_WF 590

void ui_common_style_circle_panel(lv_obj_t *obj);
lv_obj_t *ui_common_create_screen(void);
lv_obj_t *ui_common_create_title(lv_obj_t *parent, const char *text);
lv_obj_t *ui_common_create_purple_box(lv_obj_t *parent, int w, int h, int x, int y, bool outline_only);
lv_obj_t *ui_common_create_side_btn(lv_obj_t *parent, bool is_back, int x, int y, const char *label);
lv_obj_t *ui_common_create_corner_wedge(lv_obj_t *parent, ui_corner_wedge_type_t type, int x, int y);
lv_obj_t *ui_common_create_side_next(lv_obj_t *parent, int x, int y);
lv_obj_t *ui_common_create_keypad_btn(lv_obj_t *parent, const char *txt, int x, int y, int size);
void ui_common_add_numeric_keypad(lv_obj_t *parent, int start_y, lv_event_cb_t digit_cb, void *user_ctx);
void ui_common_format_mm_ss(char *buf, size_t len, uint32_t sec);
void ui_common_format_hh_mm(char *buf, size_t len, int hour, int min);

/*
 * Guide lines (LVGL lv_line). lv_line_set_points() keeps a pointer — each line needs
 * its own point pair. Call ui_common_line_points_reset() once before adding lines on a screen.
 * x / y are in the parent's content area (inside border), not full UI_DISP pixels.
 */
void ui_common_line_points_reset(void);
lv_obj_t *ui_common_add_vertical_line(lv_obj_t *parent, int x);
lv_obj_t *ui_common_add_horizontal_line(lv_obj_t *parent, int y);
void ui_common_get_content_size(lv_obj_t *parent, int32_t *w_out, int32_t *h_out);
void ui_common_get_content_center(lv_obj_t *parent, int32_t *cx_out, int32_t *cy_out);
int32_t ui_common_screen_border(const lv_obj_t *screen);
int ui_common_wf_to_content_x(lv_obj_t *screen, int x_wf);
int ui_common_wf_to_content_y(lv_obj_t *screen, int y_wf);

#ifdef __cplusplus
}
#endif
