/**
 * @file ui_assets_spiffs.c
 * @brief Mount SPIFFS storage partition and build LVGL S: drive paths for large UI images.
 */

#include "ui_assets.h"

#include "esp_log.h"
#include "esp_spiffs.h"

#include <stdio.h>

static const char *TAG = "ui_assets";

esp_err_t ui_assets_init(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "storage",
        .max_files = 8,
        .format_if_mount_failed = false,
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "SPIFFS mount failed (partition missing or corrupt)");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "SPIFFS partition 'storage' not found");
        } else {
            ESP_LOGE(TAG, "SPIFFS register failed: %s", esp_err_to_name(ret));
        }
        return ret;
    }

    size_t total = 0;
    size_t used = 0;
    ret = esp_spiffs_info("storage", &total, &used);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "SPIFFS mounted at /spiffs — %u / %u bytes used", (unsigned)used, (unsigned)total);
    }

    return ESP_OK;
}

const char *ui_assets_spiffs_path(const char *name)
{
    static char path[48];

    if (name == NULL) {
        return NULL;
    }
    snprintf(path, sizeof(path), "S:/%s.bin", name);
    return path;
}
