/**
 * @file ui_screen_ring.c
 */

#include "ui_screen_ring.h"
#include "ui_layout.h"
#include "ui_theme.h"

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
    lv_obj_set_style_border_width(screen, 0, 0);
}

void ui_screen_ring_refresh(lv_obj_t *screen)
{
    if (screen == NULL) {
        return;
    }
    if (lv_obj_get_style_border_width(screen, LV_PART_MAIN) > 0) {
        const ui_theme_t *t = ui_theme_get();
        lv_obj_set_style_border_color(screen, t->ring, 0);
    }
}
