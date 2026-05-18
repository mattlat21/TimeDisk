#pragma once

#include "lvgl.h"
#include "bsp/display.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UI_DISP BSP_LCD_H_RES

typedef void (*ui_btn_cb_t)(lv_event_t *e);

void ui_common_style_circle_panel(lv_obj_t *obj);
lv_obj_t *ui_common_create_screen(void);
lv_obj_t *ui_common_create_title(lv_obj_t *parent, const char *text);
lv_obj_t *ui_common_create_purple_box(lv_obj_t *parent, int w, int h, int x, int y, bool outline_only);
lv_obj_t *ui_common_create_side_btn(lv_obj_t *parent, bool is_back, int x, int y, const char *label);
lv_obj_t *ui_common_create_side_next(lv_obj_t *parent, int x, int y);
lv_obj_t *ui_common_create_keypad_btn(lv_obj_t *parent, const char *txt, int x, int y, int size);
void ui_common_add_numeric_keypad(lv_obj_t *parent, int start_y, lv_event_cb_t digit_cb, void *user_ctx);
void ui_common_format_mm_ss(char *buf, size_t len, uint32_t sec);
void ui_common_format_hh_mm(char *buf, size_t len, int hour, int min);

#ifdef __cplusplus
}
#endif
