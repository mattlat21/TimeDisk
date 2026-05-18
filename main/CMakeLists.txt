idf_component_register(
    SRCS "main.cpp" "ui_screens.cpp"
    INCLUDE_DIRS "."
    REQUIRES bsp_extra waveshare__esp32_p4_wifi6_touch_lcd_xc
)
