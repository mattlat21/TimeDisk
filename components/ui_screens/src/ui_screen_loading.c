#include "ui_screens_registry.h"
#include "ui_widgets.h"
#include "ui_theme.h"

static lv_obj_t *s_scr;

void ui_screen_loading_build(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    const ui_theme_t *t = ui_theme_get();
    s_scr = ui_widgets_create_screen();
    screens[UI_SCREEN_LOADING] = s_scr;

    lv_obj_t *lbl = lv_label_create(s_scr);
    lv_label_set_text(lbl, "Loading...");
    lv_obj_set_style_text_color(lbl, t->white, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_26, 0);
    lv_obj_center(lbl);
}

void ui_screen_loading_on_show(void)
{
    (void)s_scr;
}
