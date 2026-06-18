    idf_component_register(
    SRCS "src/app_network.c" "src/app_network_setup.c" "src/app_network_web_ui.c"
         "src/app_network_web_ui_json.c"
    INCLUDE_DIRS "include"
    REQUIRES app_config app_time json esp_wifi esp_netif esp_event esp_timer esp_http_server esp_app_format spiffs lwip app_ota espressif__esp_wifi_remote espressif__esp_hosted
)
