idf_component_register(
    SRCS "src/app_mqtt.c"
    INCLUDE_DIRS "include"
    REQUIRES mqtt json app_config app_network ui_nav lvgl esp_timer esp_app_format esp_netif
)
