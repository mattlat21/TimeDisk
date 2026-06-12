idf_component_register(
    SRCS "src/app_ota.c"
    INCLUDE_DIRS "include"
    REQUIRES app_config app_update esp_http_client esp_https_ota
)
