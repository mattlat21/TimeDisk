/**
 * @file ui_screen_ring.c
 */

#include "ui_screen_ring.h"
#include "ui_layout.h"
#include "ui_theme.h"

static lv_obj_t *ring_overlay_get(lv_obj_t *screen)
{
    return (lv_obj_t *)lv_obj_get_user_data(screen);
}

static lv_obj_t *ring_overlay_ensure(lv_obj_t *screen)
{
    const ui_theme_t *t = ui_theme_get();
    const int border = UI_RING_BORDER;

    lv_obj_add_flag(screen, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    lv_obj_set_style_border_width(screen, 0, 0);

    lv_obj_t *overlay = ring_overlay_get(screen);
    if (overlay != NULL && lv_obj_get_parent(overlay) == screen) {
        lv_obj_set_style_border_color(overlay, t->ring, 0);
        return overlay;
    }

    overlay = lv_obj_create(screen);
    lv_obj_set_user_data(screen, overlay);
    /* Screen border is cleared; content origin is the full LCD top-left. */
    lv_obj_set_pos(overlay, 0, 0);
    lv_obj_set_size(overlay, UI_SCREEN_W, UI_SCREEN_H);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(overlay, t->ring, 0);
    lv_obj_set_style_border_width(overlay, border, 0);
    lv_obj_set_style_radius(overlay, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_pad_all(overlay, 0, 0);
    lv_obj_remove_flag(overlay, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(overlay, LV_OBJ_FLAG_FLOATING | LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    return overlay;
}

void ui_screen_ring_apply(lv_obj_t *screen)
{
    if (screen == NULL) {
        return;
    }
    const ui_theme_t *t = ui_theme_get();

    lv_obj_set_style_border_color(screen, t->ring, 0);
    lv_obj_set_style_border_width(screen, UI_RING_BORDER, 0);
    lv_obj_set_style_radius(screen, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_clip_corner(screen, true, 0);
}

void ui_screen_ring_clear(lv_obj_t *screen)
{
    if (screen == NULL) {
        return;
    }
    const ui_theme_t *t = ui_theme_get();

    lv_obj_set_style_border_width(screen, 0, 0);
    lv_obj_set_style_border_opa(screen, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(screen, t->bg, 0);
    lv_obj_set_style_outline_width(screen, 0, 0);
    lv_obj_set_style_outline_opa(screen, LV_OPA_TRANSP, 0);
}

void ui_screen_ring_refresh(lv_obj_t *screen)
{
    if (screen == NULL) {
        return;
    }
    const ui_theme_t *t = ui_theme_get();

    if (lv_obj_get_style_border_width(screen, LV_PART_MAIN) > 0) {
        lv_obj_set_style_border_color(screen, t->ring, 0);
    }

    lv_obj_t *overlay = ring_overlay_get(screen);
    if (overlay != NULL && lv_obj_get_parent(overlay) == screen) {
        lv_obj_set_style_border_color(overlay, t->ring, 0);
    }
}

void ui_screen_ring_raise_overlay(lv_obj_t *screen)
{
    if (screen == NULL) {
        return;
    }
    lv_obj_t *overlay = ring_overlay_ensure(screen);
    lv_obj_move_foreground(overlay);
}
