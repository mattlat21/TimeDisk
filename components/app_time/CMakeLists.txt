idf_component_register(
    SRCS "src/app_time.c" "src/timezone_catalog.c"
    INCLUDE_DIRS "include"
    REQUIRES app_config
)
