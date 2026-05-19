/**
 * @file ui_wedge.c
 * @brief Corner wedge buttons (cancel / confirm) as pre-rendered images.
 *
 * Source: docs/wireframes/startup_wizard_ssid.svg
 *   → components/ui_assets/assets/wedge_{cancel,confirm}.svg
 *   → PNG (rsvg-convert) → wedge_*.c (LVGLImage.py, RGB565A8 for transparency)
 */

#include "ui_wedge.h"
#include "ui_assets.h"

lv_obj_t *ui_wedge_create(lv_obj_t *parent, ui_wedge_type_t type, int x, int y)
{
    const lv_image_dsc_t *src = (type == UI_WEDGE_CANCEL) ? &wedge_cancel : &wedge_confirm;

    lv_obj_t *img = lv_image_create(parent);
    lv_image_set_src(img, src);
    lv_obj_set_pos(img, x, y);
    lv_obj_remove_flag(img, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(img, LV_OBJ_FLAG_CLICKABLE);

    return img;
}
