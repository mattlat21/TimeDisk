idf_component_register(
    SRCS
        "src/ui_assets_spiffs.c"
        # BEGIN GENERATED ASSETS
        "src/icon_wedge_cancel.c"
        "src/icon_wedge_confirm.c"
        "src/icon_wedge_menu_wide.c"
        "src/icon_wedge_next.c"
        "src/icon_wedge_settings.c"
        "src/wedge_shape_left.c"
        "src/wedge_shape_right.c"
        "src/wedge_shape_wide.c"
        # END GENERATED ASSETS
    INCLUDE_DIRS "include"
    REQUIRES lvgl spiffs
)

# LVGLImage.py includes "lvgl.h" when LV_LVGL_H_INCLUDE_SIMPLE is set (ESP-IDF lvgl port).
target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_LVGL_H_INCLUDE_SIMPLE)
