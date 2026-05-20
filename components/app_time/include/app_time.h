/**
 * @file app_time.h
 * @brief Apply persisted or preview timezone to libc (localtime_r).
 */

#pragma once

#include <esp_err.h>

esp_err_t app_time_apply_timezone_id(const char *timezone_id);

void app_time_apply_from_config(void);
