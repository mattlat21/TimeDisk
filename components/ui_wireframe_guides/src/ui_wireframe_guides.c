/**
 * @file ui_wireframe_guides.c
 */

#include "ui_wireframe_guides.h"
#include "ui_layout.h"
#include "ui_theme.h"

#define UI_WIREFRAME_GUIDES_PT_SLOTS 64
#define UI_WIREFRAME_GUIDES_CROSSHAIR_CIRCLE_D_OUTER 690
#define UI_WIREFRAME_GUIDES_CROSSHAIR_CIRCLE_D_INNER 660
#define UI_WIREFRAME_GUIDES_CROSSHAIR_OFFSET_X     10
#define UI_WIREFRAME_GUIDES_SIDE_OFFSET_X         210
#define UI_WIREFRAME_GUIDES_BELOW_CENTER_OFFSET_Y 230

static lv_point_precise_t s_line_pt_pool[UI_WIREFRAME_GUIDES_PT_SLOTS][2];
static int s_line_pt_pool_next;

static void parent_content_size(lv_obj_t *parent, int32_t *w_out, int32_t *h_out)
{
    lv_area_t content;
    lv_obj_get_content_coords(parent, &content);
    *w_out = lv_area_get_width(&content);
    *h_out = lv_area_get_height(&content);
}

void ui_wireframe_guides_reset(void)
{
    s_line_pt_pool_next = 0;
}

static lv_point_precise_t *line_points_alloc(void)
{
    if (s_line_pt_pool_next >= UI_WIREFRAME_GUIDES_PT_SLOTS) {
        return NULL;
    }
    return s_line_pt_pool[s_line_pt_pool_next++];
}

static lv_obj_t *add_line_internal(lv_obj_t *parent, lv_point_precise_t *pts, lv_color_t color)
{
    if (pts == NULL) {
        return NULL;
    }

    lv_obj_t *line = lv_line_create(parent);
    lv_line_set_points(line, pts, 2);
    lv_obj_set_style_line_color(line, color, 0);
    lv_obj_set_style_line_width(line, 1, 0);
    lv_obj_set_style_line_rounded(line, true, 0);
    lv_obj_remove_flag(line, LV_OBJ_FLAG_CLICKABLE);
    return line;
}

lv_obj_t *ui_wireframe_guides_add_vertical(lv_obj_t *parent, int x)
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

    const ui_theme_t *t = ui_theme_get();
    return add_line_internal(parent, pts, t->white);
}

lv_obj_t *ui_wireframe_guides_add_horizontal(lv_obj_t *parent, int y)
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

    const ui_theme_t *t = ui_theme_get();
    return add_line_internal(parent, pts, t->white);
}

void ui_wireframe_guides_add_center_crosshair(lv_obj_t *parent)
{
    if (parent == NULL) {
        return;
    }

    lv_obj_t *screen = ui_layout_find_screen(parent);
    if (screen == NULL) {
        screen = parent;
    }

    const ui_theme_t *t = ui_theme_get();
    const int border = (int)ui_layout_screen_border(screen);
    const int center = (int)UI_SCREEN_CX;

    lv_obj_add_flag(screen, LV_OBJ_FLAG_OVERFLOW_VISIBLE);

    lv_obj_t *overlay = lv_obj_create(screen);
    lv_obj_set_pos(overlay, -border, -border);
    lv_obj_set_size(overlay, UI_SCREEN_W, UI_SCREEN_H);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(overlay, 0, 0);
    lv_obj_set_style_pad_all(overlay, 0, 0);
    lv_obj_remove_flag(overlay, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(overlay, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_OVERFLOW_VISIBLE);

    static const int circle_diameters[] = {
        UI_WIREFRAME_GUIDES_CROSSHAIR_CIRCLE_D_OUTER,
        UI_WIREFRAME_GUIDES_CROSSHAIR_CIRCLE_D_INNER,
    };
    for (size_t i = 0; i < sizeof(circle_diameters) / sizeof(circle_diameters[0]); i++) {
        const int d = circle_diameters[i];
        const int origin = center - (d / 2);

        lv_obj_t *circle = lv_obj_create(overlay);
        lv_obj_set_size(circle, d, d);
        lv_obj_set_pos(circle, origin, origin);
        lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(circle, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_color(circle, t->white, 0);
        lv_obj_set_style_border_width(circle, 1, 0);
        lv_obj_remove_flag(circle, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    }

    static const int vertical_x[] = {
        center - UI_WIREFRAME_GUIDES_SIDE_OFFSET_X,
        center - UI_WIREFRAME_GUIDES_CROSSHAIR_OFFSET_X,
        center,
        center + UI_WIREFRAME_GUIDES_CROSSHAIR_OFFSET_X,
        center + UI_WIREFRAME_GUIDES_SIDE_OFFSET_X,
    };
    for (size_t i = 0; i < sizeof(vertical_x) / sizeof(vertical_x[0]); i++) {
        lv_point_precise_t *vpts = line_points_alloc();
        if (vpts == NULL) {
            continue;
        }
        vpts[0].x = vertical_x[i];
        vpts[0].y = 0;
        vpts[1].x = vertical_x[i];
        vpts[1].y = UI_SCREEN_H;
        add_line_internal(overlay, vpts, t->white);
    }

    static const int horizontal_y[] = {
        center,
        center + UI_WIREFRAME_GUIDES_BELOW_CENTER_OFFSET_Y,
    };
    for (size_t i = 0; i < sizeof(horizontal_y) / sizeof(horizontal_y[0]); i++) {
        lv_point_precise_t *hpts = line_points_alloc();
        if (hpts == NULL) {
            continue;
        }
        hpts[0].x = 0;
        hpts[0].y = horizontal_y[i];
        hpts[1].x = UI_SCREEN_W;
        hpts[1].y = horizontal_y[i];
        add_line_internal(overlay, hpts, t->white);
    }

    lv_obj_move_foreground(overlay);
}
