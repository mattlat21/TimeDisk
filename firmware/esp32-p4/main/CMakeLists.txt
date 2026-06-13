idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES ui_assets ui_display ui_nav app_network app_mqtt bsp_extra espressif__esp_hosted waveshare__esp32_p4_wifi6_touch_lcd_xc
)
