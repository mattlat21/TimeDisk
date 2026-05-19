/**
 * @file ui_lines.c
 */

#include "ui_lines.h"
#include "ui_layout.h"
#include "ui_theme.h"

#define UI_LINES_PT_SLOTS 64

static lv_point_precise_t s_line_pt_pool[UI_LINES_PT_SLOTS][2];
static int s_line_pt_pool_next;

static void parent_content_size(lv_obj_t *parent, int32_t *w_out, int32_t *h_out)
{
    lv_area_t content;
    lv_obj_get_content_coords(parent, &content);
    *w_out = lv_area_get_width(&content);
    *h_out = lv_area_get_height(&content);
}

void ui_lines_reset(void)
{
    s_line_pt_pool_next = 0;
}

static lv_point_precise_t *line_points_alloc(void)
{
    if (s_line_pt_pool_next >= UI_LINES_PT_SLOTS) {
        return NULL;
    }
    return s_line_pt_pool[s_line_pt_pool_next++];
}

static lv_obj_t *add_line_internal(lv_obj_t *parent, lv_point_precise_t *pts)
{
    if (pts == NULL) {
        return NULL;
    }

    const ui_theme_t *t = ui_theme_get();
    lv_obj_t *line = lv_line_create(parent);
    lv_line_set_points(line, pts, 2);
    lv_obj_set_style_line_color(line, t->white, 0);
    lv_obj_set_style_line_width(line, 1, 0);
    lv_obj_set_style_line_rounded(line, true, 0);
    lv_obj_remove_flag(line, LV_OBJ_FLAG_CLICKABLE);
    return line;
}

lv_obj_t *ui_lines_add_vertical(lv_obj_t *parent, int x)
{
    lv_point_precise_t *pts = line_points_alloc();
    if (pts == NULL) {
        return NULL;
    }

    int32_t cw;
    int32_t ch;
    parent_content_size(parent, &cw, &ch);

    pts[0].x = x;
    pts[0].y = 0;
    pts[1].x = x;
    pts[1].y = ch;
    return add_line_internal(parent, pts);
}

lv_obj_t *ui_lines_add_horizontal(lv_obj_t *parent, int y)
{
    lv_point_precise_t *pts = line_points_alloc();
    if (pts == NULL) {
        return NULL;
    }

    int32_t cw;
    int32_t ch;
    parent_content_size(parent, &cw, &ch);

    pts[0].x = 0;
    pts[0].y = y;
    pts[1].x = cw;
    pts[1].y = y;
    return add_line_internal(parent, pts);
}
