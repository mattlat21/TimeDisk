/**
 * @file app_network_web_ui_internal.h
 * @brief Shared JSON builders for REST web UI.
 */

#pragma once

#include "app_config.h"

#include <cJSON.h>

void app_network_web_ui_json_add_config(cJSON *root, const app_config_t *cfg);
void app_network_web_ui_json_add_live_status(cJSON *root);

/** Full /api/status response (live fields + embedded config). Caller frees. */
char *app_network_web_ui_build_rest_status_json(void);
