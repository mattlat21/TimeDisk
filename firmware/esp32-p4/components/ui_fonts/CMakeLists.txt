idf_component_register(
    SRCS
        "src/lv_font_montserrat_64.c"
        "src/lv_font_montserrat_80.c"
    INCLUDE_DIRS "include"
    REQUIRES lvgl
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_LVGL_H_INCLUDE_SIMPLE)
