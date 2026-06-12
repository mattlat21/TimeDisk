idf_component_register(
    SRCS "src/ui_nav.c"
    REQUIRES ui_core ui_screens ui_theme app_config app_time app_network lvgl waveshare__esp32_p4_wifi6_touch_lcd_xc esp_driver_ledc esp_timer
)
