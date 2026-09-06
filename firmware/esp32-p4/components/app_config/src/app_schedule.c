/**
 * @file app_schedule.c
 * @brief Wall-clock schedule: match local minute, fire once per day per event.
 */

#include "app_schedule.h"

#include "app_config.h"

#include <esp_log.h>
#include <limits.h>
#include <stdint.h>
#include <time.h>

static const char *TAG = "app_schedule";

static app_schedule_fire_cb_t s_fire_cb;
static int s_fired_ymd[APP_SCHEDULE_EVENT_MAX];

static int local_ymd(time_t now)
{
    struct tm tm_local;
    localtime_r(&now, &tm_local);
    return (tm_local.tm_year + 1900) * 10000 + (tm_local.tm_mon + 1) * 100 + tm_local.tm_mday;
}

static uint16_t local_minute(time_t now)
{
    struct tm tm_local;
    localtime_r(&now, &tm_local);
    return (uint16_t)(tm_local.tm_hour * 60 + tm_local.tm_min);
}

static int local_sec_of_day(time_t now)
{
    struct tm tm_local;
    localtime_r(&now, &tm_local);
    return tm_local.tm_hour * 3600 + tm_local.tm_min * 60 + tm_local.tm_sec;
}

app_mode_t app_schedule_action_target_mode(uint8_t action)
{
    switch ((app_schedule_action_t)action) {
    case APP_SCHEDULE_ACTION_START_WIND_DOWN:
        return APP_MODE_WIND_DOWN;
    case APP_SCHEDULE_ACTION_START_SLEEP:
        return APP_MODE_SLEEP;
    case APP_SCHEDULE_ACTION_START_REST:
        return APP_MODE_REST;
    case APP_SCHEDULE_ACTION_WAKE:
    default:
        return APP_MODE_WAKE;
    }
}

const char *app_schedule_action_str(uint8_t action)
{
    switch ((app_schedule_action_t)action) {
    case APP_SCHEDULE_ACTION_START_SLEEP:
        return "start_sleep";
    case APP_SCHEDULE_ACTION_START_REST:
        return "start_rest";
    case APP_SCHEDULE_ACTION_START_WIND_DOWN:
        return "start_wind_down";
    case APP_SCHEDULE_ACTION_WAKE:
    default:
        return "wake";
    }
}

static uint32_t remaining_until_time_min(int now_sec_of_day, uint16_t time_min)
{
    const int event_sec = (int)time_min * 60;
    int delta = event_sec - now_sec_of_day;
    if (delta <= 0) {
        delta += 24 * 60 * 60;
    }
    return (uint32_t)delta;
}

static bool next_event_internal(time_t now, bool require_mode_change, app_mode_t current_mode,
                                app_schedule_next_tod_t *out)
{
    if (out == NULL) {
        return false;
    }

    *out = (app_schedule_next_tod_t){0};

    const app_config_t *cfg = app_config_get();
    const int now_sec = local_sec_of_day(now);
    uint32_t best_remaining = UINT32_MAX;
    int best_index = -1;

    for (uint8_t i = 0; i < cfg->schedule_event_count; i++) {
        const app_schedule_event_t *ev = &cfg->schedule_events[i];
        if (!ev->enabled) {
            continue;
        }

        const bool changes = app_schedule_action_target_mode(ev->action) != current_mode;
        if (require_mode_change && !changes) {
            continue;
        }

        const uint32_t rem = remaining_until_time_min(now_sec, ev->time_min);
        if (rem < best_remaining) {
            best_remaining = rem;
            best_index = (int)i;
        }
    }

    if (best_index < 0) {
        return false;
    }

    const app_schedule_event_t *best = &cfg->schedule_events[best_index];
    out->found = true;
    out->index = (uint8_t)best_index;
    out->time_min = best->time_min;
    out->action = best->action;
    out->duration_sec = best->duration_sec;
    out->remaining_sec = best_remaining;
    out->changes_mode = app_schedule_action_target_mode(best->action) != current_mode;
    return true;
}

bool app_schedule_next_tod_event(time_t now, app_mode_t current_mode, app_schedule_next_tod_t *out)
{
    return next_event_internal(now, false, current_mode, out);
}

bool app_schedule_next_mode_change_event(time_t now, app_mode_t current_mode,
                                         app_schedule_next_tod_t *out)
{
    return next_event_internal(now, true, current_mode, out);
}

uint32_t app_schedule_mode_display_remaining_sec(time_t now, const app_runtime_t *rt)
{
    if (rt == NULL) {
        return 0;
    }

    uint32_t mode_rem = 0;
    if (rt->cycle_active) {
        mode_rem = rt->mode_remaining_sec;
    }

    app_schedule_next_tod_t next_change = {0};
    if (!rt->time_valid ||
        !app_schedule_next_mode_change_event(now, rt->current_mode, &next_change)) {
        return mode_rem;
    }

    if (mode_rem == 0) {
        return next_change.remaining_sec;
    }
    return mode_rem < next_change.remaining_sec ? mode_rem : next_change.remaining_sec;
}

void app_schedule_set_fire_callback(app_schedule_fire_cb_t cb)
{
    s_fire_cb = cb;
}

void app_schedule_tick(void)
{
    app_runtime_t *rt = app_runtime_get();
    if (!rt->time_valid || s_fire_cb == NULL) {
        return;
    }

    const time_t now = time(NULL);
    const int ymd = local_ymd(now);
    const uint16_t now_min = local_minute(now);

    static int s_last_ymd = -1;
    static uint16_t s_last_min = UINT16_MAX;
    if (ymd == s_last_ymd && now_min == s_last_min) {
        return;
    }
    s_last_ymd = ymd;
    s_last_min = now_min;

    const app_config_t *cfg = app_config_get();
    for (uint8_t i = 0; i < cfg->schedule_event_count; i++) {
        const app_schedule_event_t *ev = &cfg->schedule_events[i];
        if (!ev->enabled || ev->time_min != now_min) {
            continue;
        }
        if (s_fired_ymd[i] == ymd) {
            continue;
        }

        s_fired_ymd[i] = ymd;
        ESP_LOGI(TAG, "fire event %u action=%u at %02u:%02u",
                 (unsigned)i, (unsigned)ev->action,
                 (unsigned)(ev->time_min / 60), (unsigned)(ev->time_min % 60));
        s_fire_cb(ev);
    }
}
