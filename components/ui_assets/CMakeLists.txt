idf_component_register(
    SRCS
        "src/splash.c"
        "src/wedge_shape_left.c"
        "src/wedge_shape_right.c"
        "src/wedge_shape_wide.c"
        "src/icon_wedge_cancel.c"
        "src/icon_wedge_confirm.c"
        "src/icon_wedge_next.c"
        "src/icon_wedge_settings.c"
        "src/icon_wedge_menu_wide.c"
        "src/tod_wake.c"
        "src/tod_winddown.c"
        "src/tod_sleep.c"
        "src/tod_rest.c"
    INCLUDE_DIRS "include"
    REQUIRES lvgl
)

# LVGLImage.py includes "lvgl.h" when LV_LVGL_H_INCLUDE_SIMPLE is set (ESP-IDF lvgl port).
target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_LVGL_H_INCLUDE_SIMPLE)
