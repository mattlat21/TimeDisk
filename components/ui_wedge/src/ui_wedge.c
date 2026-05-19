/**
 * @file ui_wedge.c
 * @brief Custom-drawn corner wedges with X / check icons.
 */

#include "ui_wedge.h"
#include "ui_layout.h"
#include "ui_theme.h"

#define UI_WEDGE_COLOR_CANCEL  0xFC6700
#define UI_WEDGE_COLOR_CONFIRM 0x5FB001
#define UI_WEDGE_ICON_WIDTH    9
#define UI_WEDGE_PT_COUNT      10

static const lv_point_precise_t s_wedge_pts_left[UI_WEDGE_PT_COUNT] = {
    {31, 0}, {175, 0}, {208, 22}, {208, 75}, {163, 99},
    {128, 88}, {27, 41}, {13, 24}, {0, 7}, {15, 0},
};

static const uint8_t s_wedge_tri_fan[] = {
    1, 0, 2, 1, 2, 3, 1, 3, 4, 1, 4, 5, 1, 5, 6, 1, 6, 7, 1, 7, 8, 1, 8, 9, 1, 9, 0,
};

typedef struct {
    ui_wedge_type_t type;
} ui_wedge_ud_t;

static int32_t wedge_mirror_x(int32_t x, bool mirror)
{
    if (!mirror) {
        return x;
    }
    return UI_WEDGE_W_WF - x;
}

static void wedge_draw_line_abs(lv_layer_t *layer, lv_color_t color, int32_t w,
                                int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
    lv_draw_line_dsc_t dsc;
    lv_draw_line_dsc_init(&dsc);
    dsc.color = color;
    dsc.width = w;
    dsc.round_start = 1;
    dsc.round_end = 1;
    dsc.p1.x = x1;
    dsc.p1.y = y1;
    dsc.p2.x = x2;
    dsc.p2.y = y2;
    lv_draw_line(layer, &dsc);
}

static void wedge_draw_fill(lv_layer_t *layer, lv_obj_t *obj, lv_color_t fill, bool mirror)
{
    lv_area_t coords;
    lv_obj_get_coords(obj, &coords);
    const int32_t ox = coords.x1;
    const int32_t oy = coords.y1;

    lv_draw_triangle_dsc_t dsc;
    lv_draw_triangle_dsc_init(&dsc);
    dsc.color = fill;
    dsc.opa = LV_OPA_COVER;

    for (size_t i = 0; i < sizeof(s_wedge_tri_fan) / sizeof(s_wedge_tri_fan[0]); i += 3) {
        const uint8_t i0 = s_wedge_tri_fan[i];
        const uint8_t i1 = s_wedge_tri_fan[i + 1];
        const uint8_t i2 = s_wedge_tri_fan[i + 2];
        dsc.p[0].x = wedge_mirror_x(s_wedge_pts_left[i0].x, mirror) + ox;
        dsc.p[0].y = s_wedge_pts_left[i0].y + oy;
        dsc.p[1].x = wedge_mirror_x(s_wedge_pts_left[i1].x, mirror) + ox;
        dsc.p[1].y = s_wedge_pts_left[i1].y + oy;
        dsc.p[2].x = wedge_mirror_x(s_wedge_pts_left[i2].x, mirror) + ox;
        dsc.p[2].y = s_wedge_pts_left[i2].y + oy;
        lv_draw_triangle(layer, &dsc);
    }
}

static void wedge_draw_icon(lv_layer_t *layer, lv_obj_t *obj, ui_wedge_type_t type)
{
    lv_area_t coords;
    lv_obj_get_coords(obj, &coords);
    const int32_t ox = coords.x1;
    const int32_t oy = coords.y1;
    const lv_color_t white = lv_color_white();
    const bool mirror = (type == UI_WEDGE_CONFIRM);

    if (type == UI_WEDGE_CANCEL) {
        const int32_t x1a = wedge_mirror_x(115, mirror) + ox;
        const int32_t y1a = 27 + oy;
        const int32_t x1b = wedge_mirror_x(144, mirror) + ox;
        const int32_t y1b = 56 + oy;
        const int32_t x2a = wedge_mirror_x(150, mirror) + ox;
        const int32_t y2a = 27 + oy;
        const int32_t x2b = wedge_mirror_x(115, mirror) + ox;
        const int32_t y2b = 56 + oy;
        wedge_draw_line_abs(layer, white, UI_WEDGE_ICON_WIDTH, x1a, y1a, x1b, y1b);
        wedge_draw_line_abs(layer, white, UI_WEDGE_ICON_WIDTH, x2a, y2a, x2b, y2b);
    } else {
        const int32_t x0 = wedge_mirror_x(71, mirror) + ox;
        const int32_t y0 = 43 + oy;
        const int32_t x1 = wedge_mirror_x(83, mirror) + ox;
        const int32_t y1 = 56 + oy;
        const int32_t x2 = wedge_mirror_x(104, mirror) + ox;
        const int32_t y2 = 21 + oy;
        wedge_draw_line_abs(layer, white, UI_WEDGE_ICON_WIDTH, x0, y0, x1, y1);
        wedge_draw_line_abs(layer, white, UI_WEDGE_ICON_WIDTH, x1, y1, x2, y2);
    }
}

static void wedge_delete_cb(lv_event_t *e);
static void wedge_draw_cb(lv_event_t *e);

static void wedge_draw_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_DRAW_MAIN) {
        return;
    }
    lv_obj_t *obj = lv_event_get_target(e);
    ui_wedge_ud_t *ud = lv_obj_get_user_data(obj);
    if (ud == NULL) {
        return;
    }

    lv_layer_t *layer = lv_event_get_layer(e);
    const bool mirror = (ud->type == UI_WEDGE_CONFIRM);
    const lv_color_t fill = ui_theme_from_rgb(ud->type == UI_WEDGE_CANCEL ? UI_WEDGE_COLOR_CANCEL
                                                                        : UI_WEDGE_COLOR_CONFIRM);
    wedge_draw_fill(layer, obj, fill, mirror);
    wedge_draw_icon(layer, obj, ud->type);
}

static void wedge_delete_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_DELETE) {
        lv_free(lv_obj_get_user_data(lv_event_get_target(e)));
    }
}

lv_obj_t *ui_wedge_create(lv_obj_t *parent, ui_wedge_type_t type, int x, int y)
{
    ui_wedge_ud_t *ud = lv_malloc(sizeof(ui_wedge_ud_t));
    if (ud == NULL) {
        return NULL;
    }
    ud->type = type;

    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_set_size(obj, UI_WEDGE_W_WF, UI_WEDGE_H_WF);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_user_data(obj, ud);
    lv_obj_add_event_cb(obj, wedge_draw_cb, LV_EVENT_DRAW_MAIN, NULL);
    lv_obj_add_event_cb(obj, wedge_delete_cb, LV_EVENT_DELETE, NULL);

    return obj;
}
