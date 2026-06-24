idf_component_register(
    SRCS
        "src/ui_widgets.c"
        "src/ui_duration_editor.c"
        "src/ui_time_editor.c"
        "src/ui_large_time_picker.c"
    INCLUDE_DIRS "include"
    REQUIRES ui_fonts ui_theme ui_layout ui_format ui_screen_ring lvgl
)
