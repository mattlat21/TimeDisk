#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    lv_color_t bg;
    lv_color_t ring;
    lv_color_t panel;
    lv_color_t keypad;
    lv_color_t orange;
    lv_color_t green;
    lv_color_t white;
    lv_color_t secondary;
    lv_color_t menu_petal;
} ui_theme_t;

void ui_theme_init(void);
const ui_theme_t *ui_theme_get(void);
lv_color_t ui_theme_from_rgb(uint32_t rgb888);

#ifdef __cplusplus
}
#endif
