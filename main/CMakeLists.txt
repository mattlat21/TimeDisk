idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES ui_nav bsp_extra waveshare__esp32_p4_wifi6_touch_lcd_xc
)
