/**
 * @file app_time.c
 */

#include "app_time.h"
#include "timezone_catalog.h"
#include "app_config.h"
#include <esp_log.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static const char *TAG = "app_time";

esp_err_t app_time_apply_timezone_id(const char *timezone_id)
{
    if (timezone_id == NULL || timezone_id[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }

    const char *posix = timezone_catalog_posix_tz_by_id(timezone_id);
    if (posix == NULL) {
        ESP_LOGW(TAG, "no POSIX TZ for %s", timezone_id);
        return ESP_ERR_NOT_FOUND;
    }

    if (setenv("TZ", posix, 1) != 0) {
        ESP_LOGE(TAG, "setenv TZ failed");
        return ESP_FAIL;
    }
    tzset();
    ESP_LOGI(TAG, "TZ applied: %s (%s)", timezone_id, posix);
    return ESP_OK;
}

void app_time_apply_from_config(void)
{
    const app_config_t *cfg = app_config_get();
    if (!cfg->timezone_set || cfg->timezone_id[0] == '\0') {
        return;
    }
    app_time_apply_timezone_id(cfg->timezone_id);
}
