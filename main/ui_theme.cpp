#include "ui_theme.h"
#include "app_config.h"

static ui_theme_t s_theme;

lv_color_t ui_theme_from_rgb(uint32_t rgb888)
{
    return lv_color_make((rgb888 >> 16) & 0xFF, (rgb888 >> 8) & 0xFF, rgb888 & 0xFF);
}

void ui_theme_init(void)
{
    const app_config_t *cfg = app_config_get();

    s_theme.bg = lv_color_black();
    s_theme.ring = ui_theme_from_rgb(cfg->ui_primary_color);
    s_theme.panel = lv_color_make(0x5A, 0x18, 0x9A);
    s_theme.keypad = lv_color_make(0x1D, 0x4E, 0xD8);
    s_theme.orange = lv_color_make(0xE8, 0x7A, 0x2E);
    s_theme.green = lv_color_make(0x3C, 0xB8, 0x5C);
    s_theme.white = lv_color_white();
    s_theme.secondary = ui_theme_from_rgb(cfg->ui_secondary_color);
    s_theme.menu_petal = lv_color_make(0x0F, 0x5B, 0xF9);
}

const ui_theme_t *ui_theme_get(void)
{
    return &s_theme;
}
