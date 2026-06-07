    idf_component_register(
    SRCS "src/app_network.c" "src/app_network_setup.c"
    INCLUDE_DIRS "include"
    REQUIRES app_config esp_wifi esp_netif esp_event esp_timer esp_http_server lwip espressif__esp_wifi_remote espressif__esp_hosted
)
