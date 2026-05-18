/*
 * ui_screen_splash.cpp — Boot splash screen (first screen after power-on).
 *
 * Wireframe: docs/wireframes/splash.svg
 *   - 720×720 circular display, white fill, black "TimeDisk" wordmark + clock hands.
 *
 * We do NOT use ui_common_create_screen() here. That helper builds the standard
 * TimeDisk chrome: black background + 6 px purple ring. The splash wireframe is
 * intentionally plain white with no accent border.
 *
 * Graphics are baked into firmware as an LVGL image descriptor (RGB565):
 *   - Source SVG:  docs/wireframes/splash.svg
 *   - Raster:      rsvg-convert (or similar) → main/assets/splash.png
 *   - Embed:       LVGLImage.py → main/assets/splash.c  (symbol: splash)
 *   - Build:       listed in main/CMakeLists.txt as assets/splash.c
 *
 * Navigation (see ui_nav.cpp):
 *   - ui_nav_init() loads UI_SCREEN_SPLASH first.
 *   - ui_screen_splash_on_show() arms the idle timer from app_config.timeout_splash_sec;
 *     when it fires, nav advances to WiFi wizard or loading/TOD depending on config.
 *   - This file does not start timers itself — only registers the LVGL screen object.
 */

#include "ui_screens_registry.h"
#include "ui_common.h"   /* UI_DISP — matches BSP_LCD_H_RES (720 on this board) */
#include "bsp/display.h"
#include "lvgl.h"

/*
 * Image asset compiled from main/assets/splash.c (C translation unit).
 * Declared extern here; defined in splash.c as:
 *   const lv_image_dsc_t splash = { ... RGB565, 720×720 ... };
 */
extern "C" const lv_image_dsc_t splash;

/* Root screen object; kept so on_show() can reference it later if needed. */
static lv_obj_t *s_scr;

/*
 * ui_screen_splash_build — Called once from ui_screens_registry during ui_nav_init().
 * Fills screens[UI_SCREEN_SPLASH] with a dedicated white splash screen.
 */
extern "C" void ui_screen_splash_build(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    /* Top-level screen: parent NULL means this is a loadable LVGL screen, not a widget. */
    s_scr = lv_obj_create(NULL);

    /* Full circular panel — same logical size as the physical round LCD. */
    lv_obj_set_size(s_scr, UI_DISP, UI_DISP);

    /* White background per wireframe (<circle fill="white"/>). */
    lv_obj_set_style_bg_color(s_scr, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(s_scr, LV_OPA_COVER, 0);

    /* No border — unlike ui_common_style_circle_panel() purple ring. */
    lv_obj_set_style_border_width(s_scr, 0, 0);

    /* Circular mask so corners outside the round bezel stay hidden if anything bleeds. */
    lv_obj_set_style_radius(s_scr, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_clip_corner(s_scr, true, 0);

    /* Splash is static; no scrolling or gesture capture on the root. */
    lv_obj_remove_flag(s_scr, LV_OBJ_FLAG_SCROLLABLE);

    /*
     * Centered bitmap: pre-rendered wireframe at native resolution.
     * lv_image_set_src() uses the embedded descriptor; no filesystem or PNG decoder at runtime.
     */
    lv_obj_t *img = lv_image_create(s_scr);
    lv_image_set_src(img, &splash);
    lv_obj_center(img);

    /* Register in the global screen table used by ui_nav_go() / lv_screen_load(). */
    screens[UI_SCREEN_SPLASH] = s_scr;
}

/*
 * ui_screen_splash_on_show — Hook when nav transitions TO the splash screen.
 * Currently a no-op: timeout and next-screen logic live in ui_nav.cpp.
 * Reserved for future use (e.g. fade-in, restart animation, backlight ramp).
 */
extern "C" void ui_screen_splash_on_show(void)
{
    (void)s_scr;
}
