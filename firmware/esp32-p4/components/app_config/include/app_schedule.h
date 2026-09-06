/**
 * @file app_schedule.h
 * @brief Wall-clock schedule evaluation (fires registered handler on match).
 */

#pragma once

#include "app_config.h"

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

typedef void (*app_schedule_fire_cb_t)(const app_schedule_event_t *event);

/** Snapshot of the next enabled TOD schedule event after @p now. */
typedef struct {
    bool found;
    uint8_t index;
    uint16_t time_min;
    uint8_t action;
    uint32_t duration_sec;
    /** Seconds from @p now until the event's local clock time (may wrap to tomorrow). */
    uint32_t remaining_sec;
    /** True when applying the event would leave a different mode than @p current_mode. */
    bool changes_mode;
} app_schedule_next_tod_t;

/** Register handler invoked on the LVGL thread when an event fires. */
void app_schedule_set_fire_callback(app_schedule_fire_cb_t cb);

/** Evaluate enabled events against local time (call ~1 Hz when time_valid). */
void app_schedule_tick(void);

/** Map a schedule action to the mode it drives on the TOD display. */
app_mode_t app_schedule_action_target_mode(uint8_t action);

/** Stable string for a schedule action (e.g. "wake", "start_sleep"). */
const char *app_schedule_action_str(uint8_t action);

/**
 * Next enabled wall-clock event after @p now (device local time).
 * @p current_mode is only used to set @c changes_mode on the result.
 */
bool app_schedule_next_tod_event(time_t now, app_mode_t current_mode, app_schedule_next_tod_t *out);

/** Next enabled event whose target mode differs from @p current_mode. */
bool app_schedule_next_mode_change_event(time_t now, app_mode_t current_mode,
                                         app_schedule_next_tod_t *out);

/**
 * Remaining seconds until the active TOD screen is expected to change:
 * the sooner of cycle @c mode_remaining_sec (when a cycle is active) and the
 * next mode-changing schedule event.
 */
uint32_t app_schedule_mode_display_remaining_sec(time_t now, const app_runtime_t *rt);
