idf_component_register(
    SRCS "src/app_config.c" "src/app_nvs.c" "src/app_version.c"
    INCLUDE_DIRS "include"
    REQUIRES nvs_flash
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE
    APP_VERSION_MAJ=${FIRMWARE_VERSION_MAJ}
    APP_VERSION_MIN=${FIRMWARE_VERSION_MIN}
    APP_VERSION_REV=${FIRMWARE_VERSION_REV}
)
