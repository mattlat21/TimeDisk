/**
 * @file app_checkpoint.c
 * @brief Persist and infer mode cycle / timer state from UTC end timestamps.
 */

#include "app_checkpoint.h"
#include "app_nvs.h"

#include <esp_log.h>

static const char *TAG = "app_checkpoint";

static uint32_t remaining_sec(time_t end, time_t now)
{
    if (end <= now) {
        return 0;
    }
    time_t delta = end - now;
    if (delta > UINT32_MAX) {
        return UINT32_MAX;
    }
    return (uint32_t)delta;
}

static time_t timer_start_from_ends(const app_checkpoint_ends_t *ends,
                                    time_t now,
                                    uint32_t remaining_sec_val)
{
    if (ends->timer_start > 0 && ends->timer_start < ends->timer_end) {
        return ends->timer_start;
    }

    const app_config_t *cfg = app_config_get();
    if (cfg->timer_duration_sec > 0) {
        time_t inferred = ends->timer_end - (time_t)cfg->timer_duration_sec;
        if (inferred > 0 && inferred < ends->timer_end && inferred <= now) {
            return inferred;
        }
    }

    if (remaining_sec_val > 0) {
        time_t inferred = ends->timer_end - (time_t)remaining_sec_val;
        if (inferred > 0 && inferred < ends->timer_end) {
            return inferred;
        }
    }

    return 0;
}

esp_err_t app_checkpoint_load_ends(app_checkpoint_ends_t *out)
{
    if (out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return app_nvs_checkpoint_load(&out->winddown_end, &out->sleep_end,
                                   &out->rest_end, &out->timer_start, &out->timer_end);
}

esp_err_t app_checkpoint_save_ends(const app_checkpoint_ends_t *ends)
{
    if (ends == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = app_nvs_checkpoint_save_cycle(ends->winddown_end,
                                                  ends->sleep_end,
                                                  ends->rest_end);
    if (err != ESP_OK) {
        return err;
    }
    return app_nvs_checkpoint_save_timer(ends->timer_start, ends->timer_end);
}

esp_err_t app_checkpoint_start_cycle(time_t start_utc,
                                     uint32_t wind_down_sec,
                                     uint32_t sleep_sec,
                                     uint32_t rest_sec)
{
    time_t t = start_utc;
    time_t winddown_end = 0;
    time_t sleep_end = 0;
    time_t rest_end = 0;

    if (wind_down_sec > 0) {
        t += (time_t)wind_down_sec;
        winddown_end = t;
    }
    if (sleep_sec > 0) {
        t += (time_t)sleep_sec;
        sleep_end = t;
    }
    if (rest_sec > 0) {
        t += (time_t)rest_sec;
        rest_end = t;
    }

    ESP_LOGI(TAG, "cycle ends wd=%ld sl=%ld rs=%ld",
             (long)winddown_end, (long)sleep_end, (long)rest_end);
    return app_nvs_checkpoint_save_cycle(winddown_end, sleep_end, rest_end);
}

esp_err_t app_checkpoint_clear_cycle(void)
{
    return app_nvs_checkpoint_save_cycle(0, 0, 0);
}

esp_err_t app_checkpoint_set_timer(time_t start_utc, time_t end_utc)
{
    ESP_LOGI(TAG, "timer_start=%ld timer_end=%ld", (long)start_utc, (long)end_utc);
    return app_nvs_checkpoint_save_timer(start_utc, end_utc);
}

esp_err_t app_checkpoint_cancel_timer(time_t now)
{
    return app_nvs_checkpoint_save_timer(0, now);
}

esp_err_t app_checkpoint_clear_timer(void)
{
    return app_nvs_checkpoint_save_timer(0, 0);
}

void app_checkpoint_status_at(const app_checkpoint_ends_t *ends,
                              time_t now,
                              app_checkpoint_status_t *out)
{
    if (ends == NULL || out == NULL) {
        return;
    }

    out->cycle_active = false;
    out->cycle_mode = APP_MODE_WAKE;
    out->cycle_remaining_sec = 0;
    out->timer_running = false;
    out->timer_remaining_sec = 0;

    if (ends->winddown_end > 0 && now < ends->winddown_end) {
        out->cycle_active = true;
        out->cycle_mode = APP_MODE_WIND_DOWN;
        out->cycle_remaining_sec = remaining_sec(ends->winddown_end, now);
    } else if (ends->sleep_end > 0 && now < ends->sleep_end) {
        out->cycle_active = true;
        out->cycle_mode = APP_MODE_SLEEP;
        out->cycle_remaining_sec = remaining_sec(ends->sleep_end, now);
    } else if (ends->rest_end > 0 && now < ends->rest_end) {
        out->cycle_active = true;
        out->cycle_mode = APP_MODE_REST;
        out->cycle_remaining_sec = remaining_sec(ends->rest_end, now);
    }

    if (ends->timer_end > 0 && now < ends->timer_end) {
        out->timer_running = true;
        out->timer_remaining_sec = remaining_sec(ends->timer_end, now);
    }
}

esp_err_t app_checkpoint_get_status(time_t now, app_checkpoint_status_t *out)
{
    if (out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    app_checkpoint_ends_t ends;
    esp_err_t err = app_checkpoint_load_ends(&ends);
    if (err != ESP_OK) {
        return err;
    }

    app_checkpoint_status_at(&ends, now, out);
    return ESP_OK;
}

esp_err_t app_checkpoint_apply_to_runtime(time_t now)
{
    app_checkpoint_ends_t ends;
    esp_err_t err = app_checkpoint_load_ends(&ends);
    if (err != ESP_OK) {
        return err;
    }

    app_checkpoint_status_t status;
    app_checkpoint_status_at(&ends, now, &status);

    if ((ends.winddown_end || ends.sleep_end || ends.rest_end) && !status.cycle_active) {
        app_checkpoint_clear_cycle();
        ends.winddown_end = 0;
        ends.sleep_end = 0;
        ends.rest_end = 0;
    }

    if (ends.timer_end > 0 && !status.timer_running) {
        app_checkpoint_clear_timer();
    }

    app_runtime_t *rt = app_runtime_get();
    rt->cycle_active = status.cycle_active;
    rt->current_mode = status.cycle_mode;
    rt->mode_remaining_sec = status.cycle_remaining_sec;
    rt->timer_running = status.timer_running;
    rt->active_timer_remaining_sec = status.timer_remaining_sec;

    if (status.timer_running) {
        const time_t start = timer_start_from_ends(&ends, now, status.timer_remaining_sec);
        rt->active_timer_start_utc = start;
        rt->active_timer_end_utc = ends.timer_end;
        if (start > 0 && ends.timer_end > start) {
            rt->active_timer_total_sec = (uint32_t)(ends.timer_end - start);
        } else {
            const app_config_t *cfg = app_config_get();
            rt->active_timer_total_sec = cfg->timer_duration_sec;
            if (rt->active_timer_total_sec == 0) {
                rt->active_timer_total_sec = status.timer_remaining_sec;
            }
        }
    } else {
        rt->active_timer_total_sec = 0;
        rt->active_timer_start_utc = 0;
        rt->active_timer_end_utc = 0;
        rt->active_timer_anim_start_ms = 0;
    }

    return ESP_OK;
}
