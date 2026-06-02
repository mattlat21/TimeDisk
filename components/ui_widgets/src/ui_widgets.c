/**
 * @file ui_widgets.c
 * @brief Shared LVGL widget factories for TimeDisk screens.
 */

#include "ui_widgets.h"
#include "ui_layout.h"
#include "ui_theme.h"

void ui_widgets_apply_screen_ring(lv_obj_t *screen)
{
    const ui_theme_t *t = ui_theme_get();

    lv_obj_set_style_border_color(screen, t->ring, 0);
    lv_obj_set_style_border_width(screen, UI_RING_BORDER, 0);
    lv_obj_set_style_radius(screen, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_clip_corner(screen, true, 0);
}

void ui_widgets_style_circle_panel(lv_obj_t *obj)
{
    const ui_theme_t *t = ui_theme_get();

    lv_obj_set_size(obj, UI_DISP, UI_DISP);
    lv_obj_set_style_bg_color(obj, t->bg, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    ui_widgets_apply_screen_ring(obj);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

lv_obj_t *ui_widgets_create_screen(void)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    ui_widgets_style_circle_panel(scr);
    return scr;
}

lv_obj_t *ui_widgets_create_title(lv_obj_t *parent, const char *text)
{
    const ui_theme_t *t = ui_theme_get();
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, t->white, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_26, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 24);
    return lbl;
}

lv_obj_t *ui_widgets_create_purple_box(lv_obj_t *parent, int w, int h, int x, int y, bool outline_only)
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

lv_obj_t *ui_widgets_create_side_btn(lv_obj_t *parent, bool is_back, int x, int y, const char *label)
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

lv_obj_t *ui_widgets_create_side_next(lv_obj_t *parent, int x, int y)
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

lv_obj_t *ui_widgets_create_keypad_btn(lv_obj_t *parent, const char *txt, int x, int y, int size)
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

/* Numeric keypad is now a standalone component: ui_numeric_keypad. */
