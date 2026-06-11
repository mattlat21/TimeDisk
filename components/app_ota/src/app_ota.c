/**
 * @file app_ota.c
 * @brief HTTP(S) OTA firmware download and install.
 */

#include "app_ota.h"
#include "app_version.h"

#include <stdlib.h>
#include <esp_https_ota.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>

static const char *TAG = "app_ota";

#define OTA_TASK_STACK 8192
#define OTA_TASK_PRIO  5

typedef struct {
    char url[APP_UPDATE_URL_MAX];
    app_update_progress_cb_t progress_cb;
    app_update_done_cb_t done_cb;
    void *user_data;
} ota_job_t;

static app_update_state_t s_state = APP_UPDATE_STATE_IDLE;
static TaskHandle_t s_task;

static void report_progress(ota_job_t *job, int percent, const char *status)
{
    if (job->progress_cb != NULL) {
        job->progress_cb(percent, status, job->user_data);
    }
}

static void report_done(ota_job_t *job, esp_err_t err, const char *message)
{
    if (job->done_cb != NULL) {
        job->done_cb(err, message, job->user_data);
    }
}

static void ota_task(void *arg)
{
    ota_job_t *job = (ota_job_t *)arg;
    esp_https_ota_handle_t handle = NULL;
    esp_err_t err;

    s_state = APP_UPDATE_STATE_RUNNING;
    report_progress(job, 0, "Connecting...");

    esp_http_client_config_t http_cfg = {
        .url = job->url,
        .timeout_ms = 30000,
        .keep_alive_enable = true,
    };

    esp_https_ota_config_t ota_cfg = {
        .http_config = &http_cfg,
    };

    err = esp_https_ota_begin(&ota_cfg, &handle);
    if (err != ESP_OK) {
        char fail_msg[96];
        ESP_LOGE(TAG, "OTA begin failed: %s", esp_err_to_name(err));
        snprintf(fail_msg, sizeof(fail_msg), "Update failed: %s", esp_err_to_name(err));
        s_state = APP_UPDATE_STATE_FAILED;
        report_done(job, err, fail_msg);
        goto cleanup;
    }

    while (1) {
        err = esp_https_ota_perform(handle);
        if (err == ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            int read = esp_https_ota_get_image_len_read(handle);
            int total = esp_https_ota_get_image_size(handle);
            int percent = 0;
            if (total > 0) {
                percent = (read * 100) / total;
                if (percent > 100) {
                    percent = 100;
                }
            }
            report_progress(job, percent, "Downloading...");
            continue;
        }
        break;
    }

    if (!esp_https_ota_is_complete_data_received(handle)) {
        ESP_LOGE(TAG, "Incomplete image received");
        esp_https_ota_abort(handle);
        s_state = APP_UPDATE_STATE_FAILED;
        report_done(job, ESP_FAIL, "Incomplete download");
        goto cleanup;
    }

    err = esp_https_ota_finish(handle);
    handle = NULL;
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA finish failed: %s", esp_err_to_name(err));
        s_state = APP_UPDATE_STATE_FAILED;
        report_done(job, err, "Install failed");
        goto cleanup;
    }

    s_state = APP_UPDATE_STATE_SUCCESS;
    report_progress(job, 100, "Update complete");
    report_done(job, ESP_OK, "Rebooting...");
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();

cleanup:
    if (handle != NULL) {
        esp_https_ota_abort(handle);
    }
    s_task = NULL;
    free(job);
    vTaskDelete(NULL);
}

const char *app_update_get_version(void)
{
    return app_version_string();
}

app_update_state_t app_update_get_state(void)
{
    return s_state;
}

bool app_update_active(void)
{
    return s_state == APP_UPDATE_STATE_RUNNING;
}

esp_err_t app_update_start(const char *url, app_update_progress_cb_t progress_cb,
                           app_update_done_cb_t done_cb, void *user_data)
{
    if (url == NULL || url[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_state == APP_UPDATE_STATE_RUNNING) {
        return ESP_ERR_INVALID_STATE;
    }

    ota_job_t *job = calloc(1, sizeof(*job));
    if (job == NULL) {
        return ESP_ERR_NO_MEM;
    }

    snprintf(job->url, sizeof(job->url), "%s", url);
    job->progress_cb = progress_cb;
    job->done_cb = done_cb;
    job->user_data = user_data;

    s_state = APP_UPDATE_STATE_IDLE;

    BaseType_t ok = xTaskCreate(ota_task, "app_ota", OTA_TASK_STACK, job, OTA_TASK_PRIO, &s_task);
    if (ok != pdPASS) {
        free(job);
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}
