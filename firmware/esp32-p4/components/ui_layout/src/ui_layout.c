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

lv_obj_t *ui_layout_find_screen(lv_obj_t *obj)
{
    if (obj == NULL) {
        return NULL;
    }
    while (lv_obj_get_parent(obj) != NULL) {
        obj = lv_obj_get_parent(obj);
    }
    return obj;
}

void ui_layout_screen_pos_from_wf(lv_obj_t *screen, int x_wf, int y_wf, int *x_out, int *y_out)
{
    if (screen == NULL || x_out == NULL || y_out == NULL) {
        return;
    }
    *x_out = ui_layout_wf_to_content_x(screen, x_wf);
    *y_out = ui_layout_wf_to_content_y(screen, y_wf);
}

void ui_layout_parent_pos_from_wf(lv_obj_t *parent, int x_wf, int y_wf, int *x_out, int *y_out)
{
    if (parent == NULL || x_out == NULL || y_out == NULL) {
        return;
    }

    lv_obj_t *screen = ui_layout_find_screen(parent);
    if (screen == NULL) {
        return;
    }

    int sx = 0;
    int sy = 0;
    ui_layout_screen_pos_from_wf(screen, x_wf, y_wf, &sx, &sy);

    lv_area_t screen_content;
    lv_obj_get_content_coords(screen, &screen_content);

    /*
     * Some callers build UI into containers that start out hidden (e.g. panels that are
     * shown later). In that case, LVGL can report 0,0 for content coords before the
     * layout pass, which would skew absolute wireframe placement.
     *
     * Fall back to deriving the parent content origin from object coords + padding.
     */
    lv_area_t parent_content;
    lv_obj_get_content_coords(parent, &parent_content);
    if (lv_obj_has_flag(parent, LV_OBJ_FLAG_HIDDEN) && (parent_content.x1 == 0 && parent_content.y1 == 0)) {
        lv_area_t parent_coords;
        lv_obj_get_coords(parent, &parent_coords);

        const int32_t pad_l = lv_obj_get_style_pad_left(parent, LV_PART_MAIN);
        const int32_t pad_t = lv_obj_get_style_pad_top(parent, LV_PART_MAIN);
        const int32_t border = lv_obj_get_style_border_width(parent, LV_PART_MAIN);

        parent_content.x1 = parent_coords.x1 + border + pad_l;
        parent_content.y1 = parent_coords.y1 + border + pad_t;
    }

    *x_out = (screen_content.x1 + sx) - parent_content.x1;
    *y_out = (screen_content.y1 + sy) - parent_content.y1;
}

int ui_layout_parent_center_x_wf(lv_obj_t *parent, int32_t w)
{
    int x = 0;
    int y = 0;
    const int x_wf = (int)UI_SCREEN_CX - (int)(w / 2);
    ui_layout_parent_pos_from_wf(parent, x_wf, 0, &x, &y);
    return x;
}

int ui_layout_parent_center_y_wf(lv_obj_t *parent, int32_t h)
{
    int x = 0;
    int y = 0;
    const int y_wf = (int)UI_SCREEN_CY - (int)(h / 2);
    ui_layout_parent_pos_from_wf(parent, 0, y_wf, &x, &y);
    return y;
}
