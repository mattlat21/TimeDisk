idf_component_register(
    SRCS
        "src/splash.c"
        "src/wedge_cancel.c"
        "src/wedge_confirm.c"
        "src/wedge_settings.c"
    INCLUDE_DIRS "include"
    REQUIRES lvgl
)
