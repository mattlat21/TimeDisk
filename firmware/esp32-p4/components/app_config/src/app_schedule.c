/**
 * @file app_schedule.c
 * @brief Wall-clock schedule: match local minute, fire once per day per event.
 */

#include "app_schedule.h"

#include "app_config.h"

#include <esp_log.h>
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
