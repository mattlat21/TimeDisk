idf_component_register(
    SRCS
        "src/ui_widgets.c"
        "src/ui_duration_editor.c"
    INCLUDE_DIRS "include"
    REQUIRES ui_theme ui_layout ui_format lvgl
)
