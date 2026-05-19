idf_component_register(
    SRCS "src/app_network.c"
    INCLUDE_DIRS "include"
    REQUIRES app_config esp_wifi esp_netif esp_event esp_timer lwip espressif__esp_wifi_remote espressif__esp_hosted
)
