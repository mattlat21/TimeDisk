/**
 * @file app_scheduled_button.c
 * @brief Active scheduled TOD button evaluation.
 */

#include "app_scheduled_button.h"

#include "app_config.h"

#include <time.h>

bool app_scheduled_button_time_in_window(uint16_t now_min, uint16_t start_min, uint16_t end_min)
{
    if (now_min >= 24U * 60U || start_min >= 24U * 60U || end_min >= 24U * 60U) {
        return false;
    }

    /* Equal bounds => all day. */
    if (start_min == end_min) {
        return true;
    }

    if (start_min < end_min) {
        return now_min >= start_min && now_min < end_min;
    }

    /* Overnight: active from start through midnight, then until end. */
    return now_min >= start_min || now_min < end_min;
}

const char *app_scheduled_button_label(uint8_t action)
{
    switch (action) {
    case APP_SCHEDULE_ACTION_WAKE:
        return "Start Wake";
    case APP_SCHEDULE_ACTION_START_SLEEP:
        return "Start Sleep";
    case APP_SCHEDULE_ACTION_START_REST:
        return "Start Rest";
    case APP_SCHEDULE_ACTION_START_WIND_DOWN:
        return "Start Wind Down";
    default:
        return NULL;
    }
}

const app_scheduled_button_t *app_scheduled_button_active(void)
{
    const app_runtime_t *rt = app_runtime_get();
    if (!rt->time_valid) {
        return NULL;
    }

    const time_t now = time(NULL);
    struct tm tm_local;
    localtime_r(&now, &tm_local);
    const uint16_t now_min = (uint16_t)(tm_local.tm_hour * 60 + tm_local.tm_min);
    const uint8_t mode_bit = (uint8_t)APP_MODE_BIT(rt->current_mode);

    const app_config_t *cfg = app_config_get();
    for (uint8_t i = 0; i < cfg->scheduled_button_count; i++) {
        const app_scheduled_button_t *btn = &cfg->scheduled_buttons[i];
        if (!btn->enabled) {
            continue;
        }
        if ((btn->show_modes & mode_bit) == 0) {
            continue;
        }
        if (!app_scheduled_button_time_in_window(now_min, btn->start_min, btn->end_min)) {
            continue;
        }
        if (app_scheduled_button_label(btn->action) == NULL) {
            continue;
        }
        return btn;
    }
    return NULL;
}
