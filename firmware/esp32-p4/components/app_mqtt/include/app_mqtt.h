/**
 * @file app_mqtt.h
 * @brief TimeDisk MQTT client for Home Assistant integration (docs/mqtt_protocol.md).
 */

#pragma once

#include <esp_err.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** Initialise MQTT client, hooks, and periodic status timer. Call after app_network_init(). */
esp_err_t app_mqtt_init(void);

/** Disconnect and reconnect using current app_config MQTT fields. */
void app_mqtt_reconnect(void);

/** Publish retained status snapshot immediately. */
void app_mqtt_publish_status(void);

/** Debounced status publish (~500 ms). */
void app_mqtt_publish_status_debounced(void);

/** True when the MQTT client session is connected. */
bool app_mqtt_is_connected(void);

/**
 * Write device id (e.g. timedisk-AABBCCDDEEFF) into @p out.
 * @return true if @p out was written.
 */
bool app_mqtt_get_device_id(char *out, size_t out_len);
