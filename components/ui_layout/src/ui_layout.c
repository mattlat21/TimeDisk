/**
 * @file ui_layout.c
 * @brief Coordinate conversion between wireframe space and LVGL content area.
 */

#include "ui_layout.h"

static void parent_content_size(lv_obj_t *parent, int32_t *w_out, int32_t *h_out)
{
    lv_area_t content;
    lv_obj_get_content_coords(parent, &content);
    *w_out = lv_area_get_width(&content);
    *h_out = lv_area_get_height(&content);
}

int32_t ui_layout_screen_border(const lv_obj_t *screen)
{
    if (screen == NULL) {
        return 0;
    }
    return lv_obj_get_style_border_width(screen, LV_PART_MAIN);
}

int ui_layout_wf_to_content_x(lv_obj_t *screen, int x_wf)
{
    return x_wf - (int)ui_layout_screen_border(screen);
}

int ui_layout_wf_to_content_y(lv_obj_t *screen, int y_wf)
{
    return y_wf - (int)ui_layout_screen_border(screen);
}

void ui_layout_get_content_size(lv_obj_t *parent, int32_t *w_out, int32_t *h_out)
{
    parent_content_size(parent, w_out, h_out);
}

void ui_layout_get_content_center(lv_obj_t *parent, int32_t *cx_out, int32_t *cy_out)
{
    int32_t cw;
    int32_t ch;
    parent_content_size(parent, &cw, &ch);
    if (cx_out != NULL) {
        *cx_out = cw / 2;
    }
    if (cy_out != NULL) {
        *cy_out = ch / 2;
    }
}
