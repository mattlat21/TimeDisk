/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * TimeDisk patch (applied via scripts/apply_managed_patches.sh):
 * defer esp_hosted_init() to app_main after display init. Stock early init can
 * exhaust internal RAM before the main FreeRTOS task is created.
 */

#include "esp_log.h"
#include "esp_hosted.h"

#include "port_esp_hosted_host_log.h"

#include "esp_private/startup_internal.h"

DEFINE_LOG_TAG(host_init);

//ESP_SYSTEM_INIT_FN(esp_hosted_host_init, BIT(0), 120)
static void __attribute__((constructor)) esp_hosted_host_init(void)
{
	/* Defer esp_hosted_init() to app_main (after display). Early constructor init
	 * runs before the main FreeRTOS task is created and can exhaust internal RAM,
	 * causing xTaskCreate(main) to fail in esp_startup_start_app(). */
	(void)TAG;
}

static void __attribute__((destructor)) esp_hosted_host_deinit(void)
{
	ESP_LOGI(TAG, "ESP Hosted deinit");
	esp_hosted_deinit();
}
