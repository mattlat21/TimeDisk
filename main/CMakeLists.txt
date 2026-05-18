idf_component_register(
    SRCS
        "main.cpp"
        "app_config.cpp"
        "ui_theme.cpp"
        "ui_common.cpp"
        "ui_nav.cpp"
        "ui_screens_registry.cpp"
        "ui_screen_splash.cpp"
        "assets/splash.c"
        "ui_screen_startup_wifi_wizard.cpp"
        "ui_screen_loading.cpp"
        "ui_screen_tod.cpp"
        "ui_screen_aa.cpp"
        "ui_screen_menu.cpp"
        "ui_screen_timer.cpp"
        "ui_screen_schedule.cpp"
        "ui_screen_settings.cpp"
    INCLUDE_DIRS "."
    REQUIRES bsp_extra waveshare__esp32_p4_wifi6_touch_lcd_xc
)
