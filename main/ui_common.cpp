#include "ui_common.h"
#include "ui_theme.h"
#include <stdio.h>

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
