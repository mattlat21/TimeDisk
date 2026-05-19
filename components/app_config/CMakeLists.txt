idf_component_register(
    SRCS "src/app_config.c" "src/app_nvs.c"
    INCLUDE_DIRS "include"
    REQUIRES nvs_flash
)
