#include "ui_common.h"
#include "ui_theme.h"
#include <stdio.h>

/* startup_wizard_ssid.svg wedge fill colours */
#define UI_WEDGE_COLOR_CANCEL 0xFC6700
#define UI_WEDGE_COLOR_CONFIRM 0x5FB001
#define UI_WEDGE_ICON_WIDTH   9

/*
 * Left wedge outline (wireframe), origin (142, 589). Right wedge mirrors X in [0, W].
 * Sampled from the SVG path cubic curves.
 */
#define UI_WEDGE_PT_COUNT 10

static const lv_point_precise_t s_wedge_pts_left[UI_WEDGE_PT_COUNT] = {
    {31, 0}, {175, 0}, {208, 22}, {208, 75}, {163, 99},
    {128, 88}, {27, 41}, {13, 24}, {0, 7}, {15, 0},
};

/* Triangle fan from index 1 covering the left wedge polygon */
static const uint8_t s_wedge_tri_fan[] = {
    1, 0, 2, 1, 2, 3, 1, 3, 4, 1, 4, 5, 1, 5, 6, 1, 6, 7, 1, 7, 8, 1, 8, 9, 1, 9, 0,
};

typedef struct {
    ui_corner_wedge_type_t type;
} ui_corner_wedge_ud_t;

void ui_common_style_circle_panel(lv_obj_t *obj)
{
    const ui_theme_t *t = ui_theme_get();

    lv_obj_set_size(obj, UI_DISP, UI_DISP);
    lv_obj_set_style_bg_color(obj, t->bg, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(obj, t->ring, 0);
    lv_obj_set_style_border_width(obj, 6, 0);
    lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_clip_corner(obj, true, 0);
}

lv_obj_t *ui_common_create_screen(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    ui_common_style_circle_panel(scr);
    return scr;
}

lv_obj_t *ui_common_create_title(lv_obj_t *parent, const char *text)
{
    const ui_theme_t *t = ui_theme_get();
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, t->white, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_26, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 24);
    return lbl;
}

lv_obj_t *ui_common_create_purple_box(lv_obj_t *parent, int w, int h, int x, int y, bool outline_only)
{
    const ui_theme_t *t = ui_theme_get();
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_set_size(box, w, h);
    lv_obj_set_pos(box, x, y);
    lv_obj_set_style_bg_color(box, outline_only ? t->bg : t->panel, 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(box, t->panel, 0);
    lv_obj_set_style_border_width(box, outline_only ? 3 : 0, 0);
    lv_obj_set_style_radius(box, 12, 0);
    lv_obj_set_style_pad_all(box, 0, 0);
    lv_obj_remove_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    return box;
}

static int32_t ui_corner_wedge_mirror_x(int32_t x, bool mirror)
{
    if (!mirror) {
        return x;
    }
    return UI_CORNER_WEDGE_W_WF - x;
}

static void ui_corner_wedge_draw_line_abs(lv_layer_t *layer, lv_color_t color, int32_t w,
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

static void ui_corner_wedge_draw_fill(lv_layer_t *layer, lv_obj_t *obj, lv_color_t fill,
                                      bool mirror)
{
    lv_area_t coords;
    lv_obj_get_coords(obj, &coords);
    const int32_t ox = coords.x1;
    const int32_t oy = coords.y1;

    lv_draw_triangle_dsc_t dsc;
    lv_draw_triangle_dsc_init(&dsc);
    dsc.bg_color = fill;
    dsc.bg_opa = LV_OPA_COVER;

    for (size_t i = 0; i < sizeof(s_wedge_tri_fan) / sizeof(s_wedge_tri_fan[0]); i += 3) {
        const uint8_t i0 = s_wedge_tri_fan[i];
        const uint8_t i1 = s_wedge_tri_fan[i + 1];
        const uint8_t i2 = s_wedge_tri_fan[i + 2];
        dsc.p[0].x = ui_corner_wedge_mirror_x(s_wedge_pts_left[i0].x, mirror) + ox;
        dsc.p[0].y = s_wedge_pts_left[i0].y + oy;
        dsc.p[1].x = ui_corner_wedge_mirror_x(s_wedge_pts_left[i1].x, mirror) + ox;
        dsc.p[1].y = s_wedge_pts_left[i1].y + oy;
        dsc.p[2].x = ui_corner_wedge_mirror_x(s_wedge_pts_left[i2].x, mirror) + ox;
        dsc.p[2].y = s_wedge_pts_left[i2].y + oy;
        lv_draw_triangle(layer, &dsc);
    }
}

static void ui_corner_wedge_draw_icon(lv_layer_t *layer, lv_obj_t *obj, ui_corner_wedge_type_t type)
{
    lv_area_t coords;
    lv_obj_get_coords(obj, &coords);
    const int32_t ox = coords.x1;
    const int32_t oy = coords.y1;
    const lv_color_t white = lv_color_white();
    const bool mirror = (type == UI_CORNER_WEDGE_CONFIRM);

    if (type == UI_CORNER_WEDGE_CANCEL) {
        /* X icon — wireframe lines, relative to wedge origin (142, 589) */
        const int32_t x1a = ui_corner_wedge_mirror_x(115, mirror) + ox;
        const int32_t y1a = 27 + oy;
        const int32_t x1b = ui_corner_wedge_mirror_x(144, mirror) + ox;
        const int32_t y1b = 56 + oy;
        const int32_t x2a = ui_corner_wedge_mirror_x(150, mirror) + ox;
        const int32_t y2a = 27 + oy;
        const int32_t x2b = ui_corner_wedge_mirror_x(115, mirror) + ox;
        const int32_t y2b = 56 + oy;
        ui_corner_wedge_draw_line_abs(layer, white, UI_WEDGE_ICON_WIDTH, x1a, y1a, x1b, y1b);
        ui_corner_wedge_draw_line_abs(layer, white, UI_WEDGE_ICON_WIDTH, x2a, y2a, x2b, y2b);
    } else {
        /* Check — wireframe polyline relative to wedge origin (376, 590) */
        const int32_t x0 = ui_corner_wedge_mirror_x(71, mirror) + ox;
        const int32_t y0 = 43 + oy;
        const int32_t x1 = ui_corner_wedge_mirror_x(83, mirror) + ox;
        const int32_t y1 = 56 + oy;
        const int32_t x2 = ui_corner_wedge_mirror_x(104, mirror) + ox;
        const int32_t y2 = 21 + oy;
        ui_corner_wedge_draw_line_abs(layer, white, UI_WEDGE_ICON_WIDTH, x0, y0, x1, y1);
        ui_corner_wedge_draw_line_abs(layer, white, UI_WEDGE_ICON_WIDTH, x1, y1, x2, y2);
    }
}

static void ui_corner_wedge_delete_cb(lv_event_t *e);

static void ui_corner_wedge_draw_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_DRAW_MAIN) {
        return;
    }
    lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);
    ui_corner_wedge_ud_t *ud = (ui_corner_wedge_ud_t *)lv_obj_get_user_data(obj);
    if (ud == NULL) {
        return;
    }

    lv_layer_t *layer = lv_event_get_layer(e);
    const bool mirror = (ud->type == UI_CORNER_WEDGE_CONFIRM);
    const lv_color_t fill = ui_theme_from_rgb(ud->type == UI_CORNER_WEDGE_CANCEL ? UI_WEDGE_COLOR_CANCEL
                                                                                 : UI_WEDGE_COLOR_CONFIRM);
    ui_corner_wedge_draw_fill(layer, obj, fill, mirror);
    ui_corner_wedge_draw_icon(layer, obj, ud->type);
}

lv_obj_t *ui_common_create_corner_wedge(lv_obj_t *parent, ui_corner_wedge_type_t type, int x, int y)
{
    ui_corner_wedge_ud_t *ud =
        (ui_corner_wedge_ud_t *)lv_malloc(sizeof(ui_corner_wedge_ud_t));
    if (ud == NULL) {
        return NULL;
    }
    ud->type = type;

    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_set_size(obj, UI_CORNER_WEDGE_W_WF, UI_CORNER_WEDGE_H_WF);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_user_data(obj, ud);
    lv_obj_add_event_cb(obj, ui_corner_wedge_draw_cb, LV_EVENT_DRAW_MAIN, NULL);
    lv_obj_add_event_cb(obj, ui_corner_wedge_delete_cb, LV_EVENT_DELETE, NULL);

    return obj;
}

static void ui_corner_wedge_delete_cb(lv_event_t *e)
{
    if (lv_event_get_code(e) == LV_EVENT_DELETE) {
        lv_free(lv_obj_get_user_data((lv_obj_t *)lv_event_get_target(e)));
    }
}

lv_obj_t *ui_common_create_side_btn(lv_obj_t *parent, bool is_back, int x, int y, const char *label)
{
    const ui_theme_t *t = ui_theme_get();
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 64, 110);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_radius(btn, 32, 0);
    lv_obj_set_style_bg_color(btn, is_back ? t->orange : t->green, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_border_width(btn, 0, 0);

    lv_obj_t *lab = lv_label_create(btn);
    lv_label_set_text(lab, label ? label : (is_back ? LV_SYMBOL_LEFT : LV_SYMBOL_OK));
    lv_obj_set_style_text_color(lab, t->white, 0);
    lv_obj_set_style_text_font(lab, &lv_font_montserrat_26, 0);
    lv_obj_center(lab);
    return btn;
}

lv_obj_t *ui_common_create_side_next(lv_obj_t *parent, int x, int y)
{
    const ui_theme_t *t = ui_theme_get();
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 64, 110);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_radius(btn, 32, 0);
    lv_obj_set_style_bg_color(btn, t->green, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_border_width(btn, 0, 0);

    lv_obj_t *lab = lv_label_create(btn);
    lv_label_set_text(lab, "Next");
    lv_obj_set_style_text_color(lab, t->white, 0);
    lv_obj_set_style_text_font(lab, &lv_font_montserrat_20, 0);
    lv_obj_center(lab);
    return btn;
}

lv_obj_t *ui_common_create_keypad_btn(lv_obj_t *parent, const char *txt, int x, int y, int size)
{
    const ui_theme_t *t = ui_theme_get();
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, size, size);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(btn, t->keypad, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_border_width(btn, 0, 0);

    lv_obj_t *lab = lv_label_create(btn);
    lv_label_set_text(lab, txt);
    lv_obj_set_style_text_color(lab, t->white, 0);
    lv_obj_set_style_text_font(lab, &lv_font_montserrat_26, 0);
    lv_obj_center(lab);
    return btn;
}

void ui_common_add_numeric_keypad(lv_obj_t *parent, int start_y, lv_event_cb_t digit_cb, void *user_ctx)
{
    (void)user_ctx;
    const int btn = 72;
    const int gap = 14;
    const int cols = 3;
    const int grid_w = cols * btn + (cols - 1) * gap;
    const int start_x = (UI_DISP - grid_w) / 2;

    static const char *keys[10] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"};
    for (int i = 0; i < 9; i++) {
        int row = i / 3;
        int col = i % 3;
        lv_obj_t *b = ui_common_create_keypad_btn(parent, keys[i],
                                                  start_x + col * (btn + gap),
                                                  start_y + row * (btn + gap), btn);
        lv_obj_add_event_cb(b, digit_cb, LV_EVENT_CLICKED, (void *)keys[i]);
    }
    lv_obj_t *b0 = ui_common_create_keypad_btn(parent, "0",
                                               start_x + btn + gap,
                                               start_y + 3 * (btn + gap), btn);
    lv_obj_add_event_cb(b0, digit_cb, LV_EVENT_CLICKED, (void *)"0");
}

void ui_common_format_mm_ss(char *buf, size_t len, uint32_t sec)
{
    snprintf(buf, len, "%02u:%02u", (unsigned)(sec / 60), (unsigned)(sec % 60));
}

void ui_common_format_hh_mm(char *buf, size_t len, int hour, int min)
{
    snprintf(buf, len, "%02d:%02d", hour, min);
}

#define UI_COMMON_LINE_PT_SLOTS 64

static lv_point_precise_t s_line_pt_pool[UI_COMMON_LINE_PT_SLOTS][2];
static int s_line_pt_pool_next;

static void ui_common_parent_content_size(lv_obj_t *parent, int32_t *w_out, int32_t *h_out)
{
    lv_area_t content;
    lv_obj_get_content_coords(parent, &content);
    *w_out = lv_area_get_width(&content);
    *h_out = lv_area_get_height(&content);
}

void ui_common_line_points_reset(void)
{
    s_line_pt_pool_next = 0;
}

void ui_common_get_content_size(lv_obj_t *parent, int32_t *w_out, int32_t *h_out)
{
    ui_common_parent_content_size(parent, w_out, h_out);
}

void ui_common_get_content_center(lv_obj_t *parent, int32_t *cx_out, int32_t *cy_out)
{
    int32_t cw;
    int32_t ch;
    ui_common_parent_content_size(parent, &cw, &ch);
    if (cx_out != NULL) {
        *cx_out = cw / 2;
    }
    if (cy_out != NULL) {
        *cy_out = ch / 2;
    }
}

int32_t ui_common_screen_border(const lv_obj_t *screen)
{
    if (screen == NULL) {
        return 0;
    }
    return lv_obj_get_style_border_width(screen, LV_PART_MAIN);
}

int ui_common_wf_to_content_x(lv_obj_t *screen, int x_wf)
{
    return x_wf - (int)ui_common_screen_border(screen);
}

int ui_common_wf_to_content_y(lv_obj_t *screen, int y_wf)
{
    return y_wf - (int)ui_common_screen_border(screen);
}

static lv_point_precise_t *ui_common_line_points_alloc(void)
{
    if (s_line_pt_pool_next >= UI_COMMON_LINE_PT_SLOTS) {
        return NULL;
    }
    return s_line_pt_pool[s_line_pt_pool_next++];
}

static lv_obj_t *ui_common_add_line_internal(lv_obj_t *parent, lv_point_precise_t *pts)
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

/*
 * x / y are in parent *content* coordinates (inside border/pad), not raw UI_DISP.
 * lv_line points are relative to the line widget; the widget is a child of content (0,0).
 */
lv_obj_t *ui_common_add_vertical_line(lv_obj_t *parent, int x)
{
    lv_point_precise_t *pts = ui_common_line_points_alloc();
    if (pts == NULL) {
        return NULL;
    }

    int32_t cw;
    int32_t ch;
    ui_common_parent_content_size(parent, &cw, &ch);

    pts[0].x = x;
    pts[0].y = 0;
    pts[1].x = x;
    pts[1].y = ch;
    return ui_common_add_line_internal(parent, pts);
}

lv_obj_t *ui_common_add_horizontal_line(lv_obj_t *parent, int y)
{
    lv_point_precise_t *pts = ui_common_line_points_alloc();
    if (pts == NULL) {
        return NULL;
    }

    int32_t cw;
    int32_t ch;
    ui_common_parent_content_size(parent, &cw, &ch);

    pts[0].x = 0;
    pts[0].y = y;
    pts[1].x = cw;
    pts[1].y = y;
    return ui_common_add_line_internal(parent, pts);
}
