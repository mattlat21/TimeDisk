/**
 * @file app_schedule.h
 * @brief Wall-clock schedule evaluation (fires registered handler on match).
 */

#pragma once

#include "app_config.h"

typedef void (*app_schedule_fire_cb_t)(const app_schedule_event_t *event);

/** Register handler invoked on the LVGL thread when an event fires. */
void app_schedule_set_fire_callback(app_schedule_fire_cb_t cb);

/** Evaluate enabled events against local time (call ~1 Hz when time_valid). */
void app_schedule_tick(void);
