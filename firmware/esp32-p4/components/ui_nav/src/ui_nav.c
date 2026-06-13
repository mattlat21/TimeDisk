/**
 * @file ui_nav.c
 * @brief Screen routing, idle timeouts, mode engine stub, and adult-auth session.
 */

#include "ui_nav.h"
#include "ui_screens_registry.h"
#include "ui_theme.h"
#include "app_config.h"
#include "app_checkpoint.h"
#include "app_nvs.h"
#include "app_time.h"
#include "app_network.h"
#include "bsp/esp-bsp.h"
#include "driver/ledc.h"
#include "esp_timer.h"
#include "sdkconfig.h"
#include <esp_err.h>
#include <esp_log.h>
#include <esp_random.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static const char *TAG = "ui_nav";

static lv_obj_t *s_screens[UI_SCREEN_COUNT];
static ui_screen_id_t s_current = UI_SCREEN_SPLASH;
static aa_session_t s_aa;
static lv_timer_t *s_idle_timer;
static lv_timer_t *s_tick_timer;
static uint32_t s_idle_deadline_ms;
static uint32_t s_loading_retry_at_ms;

#define LOADING_RETRY_MS 10000U

#define UI_NAV_BRIGHTNESS_BRIGHT  100
#define UI_NAV_BRIGHTNESS_DIM     30

#define TOD_FADE_TO_DIM_MS    5000U
#define TOD_FADE_TO_BRIGHT_MS 1000U
#define TOD_FADE_TICK_MS      8U

/** Matches BSP LEDC_TIMER_10_BIT (see bsp_display_brightness_set). */
#define BACKLIGHT_DUTY_MAX 1023U

static uint32_t s_tod_menu_deadline_ms;
static bool s_tod_menu_timed_out;
static lv_timer_t *s_tod_fade_timer;

typedef struct {
    bool active;
    uint32_t start_ms;
    uint32_t duration_ms;
    uint32_t start_duty;
    uint32_t end_duty;
    ui_screen_id_t end_screen;
    bool on_dim_screen;
    bool to_dim;
} tod_fade_t;

static tod_fade_t s_tod_fade;
static uint32_t s_last_backlight_duty = UINT32_MAX;
static void (*s_mqtt_status_hook)(void);
static app_mode_t s_mqtt_last_mode = APP_MODE_WAKE;
static uint32_t s_mqtt_last_mode_rem;
static bool s_mqtt_last_cycle;
static bool s_mqtt_last_timer;
static uint32_t s_mqtt_last_timer_rem;

static void idle_timer_cb(lv_timer_t *t);
static void on_enter(ui_screen_id_t screen);

/** Monotonic ms for timeouts and fades (not LVGL tick). */
static uint32_t now_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

static uint32_t backlight_percent_to_duty(uint8_t pct)
{
    if (pct > 100) {
        pct = 100;
    }
    return (BACKLIGHT_DUTY_MAX * (uint32_t)pct) / 100U;
}

static void backlight_apply_duty(uint32_t duty, bool force_update)
{
    if (duty > BACKLIGHT_DUTY_MAX) {
        duty = BACKLIGHT_DUTY_MAX;
    }
    if (!force_update && duty == s_last_backlight_duty) {
        return;
    }
    s_last_backlight_duty = duty;
    app_runtime_get()->display_brightness =
        (uint8_t)((duty * 100U + BACKLIGHT_DUTY_MAX / 2U) / BACKLIGHT_DUTY_MAX);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, CONFIG_BSP_DISPLAY_BRIGHTNESS_LEDC_CH, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, CONFIG_BSP_DISPLAY_BRIGHTNESS_LEDC_CH);
}

void ui_nav_set_brightness(uint8_t pct)
{
    backlight_apply_duty(backlight_percent_to_duty(pct), false);
}

void ui_nav_apply_dim(bool dim)
{
    ui_nav_set_brightness(dim ? UI_NAV_BRIGHTNESS_DIM : UI_NAV_BRIGHTNESS_BRIGHT);
}

static uint32_t backlight_duty_lerp(uint32_t from, uint32_t to, uint32_t elapsed_ms, uint32_t duration_ms)
{
    if (duration_ms == 0 || elapsed_ms >= duration_ms) {
        return to;
    }
    int32_t delta = (int32_t)to - (int32_t)from;
    int32_t value = (int32_t)from + (delta * (int32_t)elapsed_ms) / (int32_t)duration_ms;
    if (value < 0) {
        return 0;
    }
    if (value > (int32_t)BACKLIGHT_DUTY_MAX) {
        return BACKLIGHT_DUTY_MAX;
    }
    return (uint32_t)value;
}

static void tod_fade_cancel(void)
{
    if (s_tod_fade_timer != NULL) {
        lv_timer_delete(s_tod_fade_timer);
        s_tod_fade_timer = NULL;
    }
    memset(&s_tod_fade, 0, sizeof(s_tod_fade));
}

static void arm_tod_timers(void)
{
    const app_config_t *cfg = app_config_get();
    uint32_t now = now_ms();

    s_idle_deadline_ms = now + cfg->timeout_tod_dim_sec * 1000U;
    s_tod_menu_deadline_ms = now + cfg->timeout_tod_menu_sec * 1000U;
    s_tod_menu_timed_out = false;
    ui_screen_tod_set_menu_visible(true);
    if (s_idle_timer == NULL) {
        s_idle_timer = lv_timer_create(idle_timer_cb, 200, NULL);
    }
}

static void tod_fade_finish(ui_screen_id_t screen)
{
    if (screen >= UI_SCREEN_COUNT || s_screens[screen] == NULL) {
        tod_fade_cancel();
        return;
    }

    tod_fade_cancel();
    s_current = screen;
    lv_screen_load(s_screens[screen]);
    on_enter(screen);
    ui_nav_reset_idle_timer();
    ESP_LOGI(TAG, "screen -> %d", (int)screen);
}

static void tod_fade_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (!s_tod_fade.active) {
        return;
    }

    uint32_t duration = s_tod_fade.duration_ms;
    uint32_t esp_ms = now_ms();
    uint32_t elapsed = esp_ms - s_tod_fade.start_ms;
    if (elapsed > duration) {
        elapsed = duration;
    }

    uint32_t duty =
        backlight_duty_lerp(s_tod_fade.start_duty, s_tod_fade.end_duty, elapsed, duration);
    backlight_apply_duty(duty, true);

    uint8_t blend;
    if (duration == 0) {
        blend = s_tod_fade.to_dim ? 255U : 0U;
    } else if (s_tod_fade.to_dim) {
        blend = (uint8_t)((uint32_t)elapsed * 255U / duration);
    } else {
        blend = (uint8_t)(255U - (uint32_t)elapsed * 255U / duration);
    }
    ui_screen_tod_apply_dim_blend(blend, s_tod_fade.on_dim_screen);

    if (elapsed >= duration) {
        tod_fade_finish(s_tod_fade.end_screen);
    }
}

static void tod_fade_start(bool to_dim)
{
    tod_fade_cancel();

    s_last_backlight_duty = UINT32_MAX;
    s_tod_fade.active = true;
    s_tod_fade.start_ms = now_ms();
    s_tod_fade.to_dim = to_dim;
    s_tod_fade.duration_ms = to_dim ? TOD_FADE_TO_DIM_MS : TOD_FADE_TO_BRIGHT_MS;
    s_tod_fade.end_screen = to_dim ? UI_SCREEN_TOD_DIM : UI_SCREEN_TOD_BRIGHT;
    s_tod_fade.on_dim_screen = !to_dim;

    if (to_dim) {
        uint8_t current = app_runtime_get()->display_brightness;
        if (current > UI_NAV_BRIGHTNESS_BRIGHT) {
            current = UI_NAV_BRIGHTNESS_BRIGHT;
        }
        s_tod_fade.start_duty = backlight_percent_to_duty(current);
        s_tod_fade.end_duty = backlight_percent_to_duty(UI_NAV_BRIGHTNESS_DIM);
        s_idle_deadline_ms = UINT32_MAX;
        s_tod_menu_deadline_ms = 0;
        ui_screen_tod_set_menu_visible(false);
    } else {
        uint8_t current = app_runtime_get()->display_brightness;
        if (current < UI_NAV_BRIGHTNESS_DIM) {
            current = UI_NAV_BRIGHTNESS_DIM;
        }
        s_tod_fade.start_duty = backlight_percent_to_duty(current);
        s_tod_fade.end_duty = backlight_percent_to_duty(UI_NAV_BRIGHTNESS_BRIGHT);
    }

    if (s_tod_fade_timer == NULL) {
        s_tod_fade_timer = lv_timer_create(tod_fade_timer_cb, TOD_FADE_TICK_MS, NULL);
    } else {
        lv_timer_set_period(s_tod_fade_timer, TOD_FADE_TICK_MS);
    }
    tod_fade_timer_cb(s_tod_fade_timer);
}

void ui_nav_tod_fade_to_dim(void)
{
    if (s_current != UI_SCREEN_TOD_BRIGHT || s_tod_fade.active) {
        return;
    }
    tod_fade_start(true);
}

void ui_nav_tod_fade_to_bright(void)
{
    if (s_current != UI_SCREEN_TOD_DIM || s_tod_fade.active) {
        return;
    }
    tod_fade_start(false);
}

static void aa_generate_maths(void)
{
    int a, b, sum;
    do {
        a = (int)(esp_random() % 10);
        b = (int)(esp_random() % 10);
        sum = a + b;
    } while (sum < 10 || sum > 18);
    s_aa.maths_a = a;
    s_aa.maths_b = b;
    s_aa.maths_sum = sum;
}

void ui_nav_aa_new_maths(void)
{
    aa_generate_maths();
}

static void cancel_idle_timer(void)
{
    if (s_idle_timer) {
        lv_timer_delete(s_idle_timer);
        s_idle_timer = NULL;
    }
}

static void idle_timer_cb(lv_timer_t *t)
{
    (void)t;
    const app_config_t *cfg = app_config_get();
    uint32_t now = now_ms();

    if (s_aa.active && s_aa.step != AA_STEP_DONE) {
        if (now - s_aa.last_input_ms >= cfg->timeout_aa_sec * 1000U) {
            ui_nav_go(s_aa.entry_screen);
            return;
        }
    }

    if (s_current == UI_SCREEN_TOD_BRIGHT && !s_tod_fade.active) {
        if (!s_tod_menu_timed_out && s_tod_menu_deadline_ms != 0 && now >= s_tod_menu_deadline_ms) {
            ui_screen_tod_set_menu_visible(false);
            s_tod_menu_timed_out = true;
        }
    }

    if (now < s_idle_deadline_ms) {
        return;
    }

    switch (s_current) {
    case UI_SCREEN_SPLASH:
        if (app_config_theme_unset()) {
            ui_nav_go(UI_SCREEN_STARTUP_THEME);
        } else if (app_config_wifi_ssid_missing()) {
            ui_nav_go(UI_SCREEN_STARTUP_SSID);
        } else if (app_config_wifi_password_unset()) {
            ui_nav_go(UI_SCREEN_STARTUP_PASSWORD);
        } else {
            ui_nav_go(UI_SCREEN_LOADING);
        }
        break;
    case UI_SCREEN_TOD_BRIGHT:
        ui_nav_tod_fade_to_dim();
        break;
    case UI_SCREEN_MENU:
    case UI_SCREEN_CONFIRM_SWITCH_WAKE:
        ui_nav_go(UI_SCREEN_TOD_BRIGHT);
        break;
    case UI_SCREEN_TIMER_BRIGHT:
        ui_nav_go(UI_SCREEN_TIMER_DIM);
        break;
    case UI_SCREEN_TIMER_TRIGGERED:
        ui_nav_go(UI_SCREEN_TOD_BRIGHT);
        break;
    default:
        break;
    }
}

static void arm_idle_timer(uint32_t timeout_sec)
{
    s_idle_deadline_ms = now_ms() + timeout_sec * 1000U;
    if (!s_idle_timer) {
        s_idle_timer = lv_timer_create(idle_timer_cb, 200, NULL);
    }
}

static void on_enter(ui_screen_id_t screen)
{
    const app_config_t *cfg = app_config_get();

    cancel_idle_timer();

    switch (screen) {
    case UI_SCREEN_SPLASH:
        arm_idle_timer(cfg->timeout_splash_sec);
        ui_screen_splash_on_show();
        break;
    case UI_SCREEN_LOADING:
        s_loading_retry_at_ms = 0;
        ui_screen_loading_on_show();
        app_network_start_boot_sync();
        break;
    case UI_SCREEN_STARTUP_THEME:
        ui_screen_startup_theme_wizard_on_show();
        break;
    case UI_SCREEN_STARTUP_TIMEZONE:
        ui_screen_startup_timezone_wizard_on_show();
        break;
    case UI_SCREEN_TOD_BRIGHT:
        ui_screen_tod_on_show(false);
        arm_tod_timers();
        break;
    case UI_SCREEN_TOD_DIM:
        ui_screen_tod_on_show(true);
        break;
    case UI_SCREEN_ADULT_AUTH:
        ui_screen_aa_on_show();
        s_aa.last_input_ms = now_ms();
        arm_idle_timer(cfg->timeout_aa_sec);
        break;
    case UI_SCREEN_MENU:
        ui_screen_menu_on_show();
        arm_idle_timer(cfg->timeout_main_menu_sec);
        break;
    case UI_SCREEN_CONFIRM_SWITCH_WAKE:
        arm_idle_timer(cfg->timeout_main_menu_sec);
        break;
    case UI_SCREEN_TIMER_BRIGHT:
        ui_screen_timer_on_show(UI_SCREEN_TIMER_BRIGHT);
        arm_idle_timer(cfg->timeout_timer_dim_sec);
        break;
    case UI_SCREEN_TIMER_DIM:
        ui_screen_timer_on_show(UI_SCREEN_TIMER_DIM);
        break;
    case UI_SCREEN_TIMER_TRIGGERED:
        arm_idle_timer(30);
        break;
    case UI_SCREEN_SETTINGS:
        ui_screen_settings_on_show();
        arm_idle_timer(cfg->timeout_main_menu_sec);
        break;
    case UI_SCREEN_SLEEP_WAKE:
    case UI_SCREEN_SLEEP_REST_END:
    case UI_SCREEN_SLEEP_WIND_DOWN:
    case UI_SCREEN_REST_REST_END:
    case UI_SCREEN_REST_WIND_DOWN:
        ui_screen_schedule_on_show(screen);
        arm_idle_timer(cfg->timeout_main_menu_sec);
        break;
    default:
        break;
    }
}

void ui_nav_go(ui_screen_id_t screen)
{
    if (screen >= UI_SCREEN_COUNT || s_screens[screen] == NULL) {
        ESP_LOGE(TAG, "invalid screen %d", (int)screen);
        return;
    }

    tod_fade_cancel();

    if (s_current == UI_SCREEN_LOADING && screen != UI_SCREEN_LOADING) {
        app_network_cancel_boot_sync();
    }

    if (s_current == UI_SCREEN_STARTUP_TIMEZONE && screen != UI_SCREEN_STARTUP_TIMEZONE) {
        ui_screen_startup_timezone_wizard_on_hide();
    }

    if (s_aa.active && screen != UI_SCREEN_ADULT_AUTH) {
        memset(&s_aa, 0, sizeof(s_aa));
    }

    s_current = screen;
    lv_screen_load(s_screens[screen]);
    on_enter(screen);
    ui_nav_reset_idle_timer();
    ESP_LOGI(TAG, "screen -> %d", (int)screen);
}

ui_screen_id_t ui_nav_current(void)
{
    return s_current;
}

aa_session_t *ui_nav_aa_session(void)
{
    return &s_aa;
}

void ui_nav_reset_idle_timer(void)
{
    const app_config_t *cfg = app_config_get();
    switch (s_current) {
    case UI_SCREEN_SPLASH:
        arm_idle_timer(cfg->timeout_splash_sec);
        break;
    case UI_SCREEN_TOD_BRIGHT:
        if (s_tod_fade.active) {
            tod_fade_cancel();
            ui_screen_tod_on_show(false);
            ui_nav_apply_dim(false);
        }
        arm_tod_timers();
        break;
    case UI_SCREEN_MENU:
    case UI_SCREEN_CONFIRM_SWITCH_WAKE:
        arm_idle_timer(cfg->timeout_main_menu_sec);
        break;
    case UI_SCREEN_TIMER_BRIGHT:
        arm_idle_timer(cfg->timeout_timer_dim_sec);
        break;
    case UI_SCREEN_ADULT_AUTH:
        s_aa.last_input_ms = now_ms();
        arm_idle_timer(cfg->timeout_aa_sec);
        break;
    default:
        break;
    }
}

void ui_nav_aa_touch(void)
{
    s_aa.last_input_ms = now_ms();
    ui_nav_reset_idle_timer();
}

static void aa_begin_steps(void)
{
    const app_config_t *cfg = app_config_get();
    memset(s_aa.pin_buf, 0, sizeof(s_aa.pin_buf));
    memset(s_aa.answer_buf, 0, sizeof(s_aa.answer_buf));

    if ((cfg->aa_methods & AA_METHOD_PIN) != 0) {
        s_aa.step = AA_STEP_PIN;
    } else if ((cfg->aa_methods & AA_METHOD_MATHS) != 0) {
        s_aa.step = AA_STEP_MATHS;
        aa_generate_maths();
    } else {
        s_aa.step = AA_STEP_DONE;
    }
}

void ui_nav_start_aa(ui_screen_id_t entry, ui_screen_id_t on_pass)
{
    const app_config_t *cfg = app_config_get();

    s_aa.active = true;
    s_aa.entry_screen = entry;
    s_aa.on_pass_screen = on_pass;
    s_aa.last_input_ms = now_ms();

    if (cfg->aa_methods == 0) {
        ui_nav_aa_pass();
        return;
    }

    aa_begin_steps();

    if (s_aa.step == AA_STEP_DONE) {
        ui_nav_aa_pass();
        return;
    }

    if (s_aa.step == AA_STEP_MATHS) {
        aa_generate_maths();
    }

    ui_nav_go(UI_SCREEN_ADULT_AUTH);

    if (s_aa.step == AA_STEP_PIN) {
        ui_screen_aa_reset_pin();
        ui_screen_aa_show_pin();
    } else {
        ui_screen_aa_reset_maths();
        ui_screen_aa_update_maths_labels(s_aa.maths_a, s_aa.maths_b);
        ui_screen_aa_show_maths();
    }
}

void ui_nav_aa_pass(void)
{
    ui_screen_id_t dest = s_aa.on_pass_screen;
    memset(&s_aa, 0, sizeof(s_aa));
    ui_nav_go(dest);
}

void ui_nav_aa_cancel(void)
{
    ui_screen_id_t dest = s_aa.entry_screen;
    memset(&s_aa, 0, sizeof(s_aa));
    ui_nav_go(dest);
}

void ui_nav_set_mqtt_status_hook(void (*hook)(void))
{
    s_mqtt_status_hook = hook;
}

static void notify_mqtt_status(void)
{
    if (s_mqtt_status_hook != NULL) {
        s_mqtt_status_hook();
    }
}

static void mqtt_track_runtime_snapshot(void)
{
    app_runtime_t *rt = app_runtime_get();
    bool changed = (rt->current_mode != s_mqtt_last_mode) ||
                   (rt->mode_remaining_sec != s_mqtt_last_mode_rem) ||
                   (rt->cycle_active != s_mqtt_last_cycle) ||
                   (rt->timer_running != s_mqtt_last_timer) ||
                   (rt->active_timer_remaining_sec != s_mqtt_last_timer_rem);

    s_mqtt_last_mode = rt->current_mode;
    s_mqtt_last_mode_rem = rt->mode_remaining_sec;
    s_mqtt_last_cycle = rt->cycle_active;
    s_mqtt_last_timer = rt->timer_running;
    s_mqtt_last_timer_rem = rt->active_timer_remaining_sec;

    if (changed) {
        notify_mqtt_status();
    }
}

static void mode_engine_refresh_tod_if_visible(void)
{
    if (!s_tod_fade.active &&
        (s_current == UI_SCREEN_TOD_BRIGHT || s_current == UI_SCREEN_TOD_DIM)) {
        ui_screen_tod_on_show(s_current == UI_SCREEN_TOD_DIM);
    }
}

/** Advance when mode_remaining_sec hits zero (see docs/mode_flow.md). */
static void mode_engine_advance_if_expired(void)
{
    app_runtime_t *rt = app_runtime_get();
    app_config_t *cfg = app_config_get();

    if (!rt->cycle_active || rt->mode_remaining_sec > 0) {
        return;
    }

    const bool cycle_was_active = rt->cycle_active;

    switch (rt->current_mode) {
    case APP_MODE_WAKE:
        if (cfg->wind_down_sec > 0) {
            rt->current_mode = APP_MODE_WIND_DOWN;
            rt->mode_remaining_sec = cfg->wind_down_sec;
        } else if (cfg->sleep_sec > 0) {
            rt->current_mode = APP_MODE_SLEEP;
            rt->mode_remaining_sec = cfg->sleep_sec;
        } else if (cfg->rest_sec > 0) {
            rt->current_mode = APP_MODE_REST;
            rt->mode_remaining_sec = cfg->rest_sec;
        } else {
            rt->cycle_active = false;
        }
        break;
    case APP_MODE_WIND_DOWN:
        if (cfg->sleep_sec > 0) {
            rt->current_mode = APP_MODE_SLEEP;
            rt->mode_remaining_sec = cfg->sleep_sec;
        } else if (cfg->rest_sec > 0) {
            rt->current_mode = APP_MODE_REST;
            rt->mode_remaining_sec = cfg->rest_sec;
        } else {
            rt->current_mode = APP_MODE_WAKE;
            rt->cycle_active = false;
        }
        break;
    case APP_MODE_SLEEP:
        if (cfg->rest_sec > 0) {
            rt->current_mode = APP_MODE_REST;
            rt->mode_remaining_sec = cfg->rest_sec;
        } else {
            rt->current_mode = APP_MODE_WAKE;
            rt->cycle_active = false;
        }
        break;
    case APP_MODE_REST:
        rt->current_mode = APP_MODE_WAKE;
        rt->cycle_active = false;
        break;
    default:
        break;
    }

    if (cycle_was_active && !rt->cycle_active) {
        app_checkpoint_clear_cycle();
    }

    mode_engine_refresh_tod_if_visible();
    notify_mqtt_status();
}

void mode_engine_start_cycle(void)
{
    app_runtime_t *rt = app_runtime_get();
    app_config_t *cfg = app_config_get();

    if (rt->time_valid) {
        app_checkpoint_start_cycle(time(NULL),
                                   cfg->wind_down_sec,
                                   cfg->sleep_sec,
                                   cfg->rest_sec);
    }

    rt->cycle_active = true;
    rt->current_mode = APP_MODE_WAKE;
    rt->mode_remaining_sec = 0;
    /* Skip the transient Wake frame before TOD loads (mode_flow.md auto-advance). */
    mode_engine_advance_if_expired();
    notify_mqtt_status();
}

void mode_engine_switch_to_wake(void)
{
    app_runtime_t *rt = app_runtime_get();

    app_checkpoint_clear_cycle();
    rt->cycle_active = false;
    rt->current_mode = APP_MODE_WAKE;
    rt->mode_remaining_sec = 0;
    notify_mqtt_status();
}

ui_screen_id_t mode_engine_restore_from_nvs(void)
{
    app_runtime_t *rt = app_runtime_get();
    if (!rt->time_valid) {
        return UI_SCREEN_TOD_BRIGHT;
    }

    esp_err_t err = app_checkpoint_apply_to_runtime(time(NULL));
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "checkpoint restore: %s", esp_err_to_name(err));
        return UI_SCREEN_TOD_BRIGHT;
    }

    if (rt->timer_running) {
        notify_mqtt_status();
        return UI_SCREEN_TIMER_BRIGHT;
    }

    if (rt->cycle_active) {
        mode_engine_refresh_tod_if_visible();
    }
    notify_mqtt_status();
    return UI_SCREEN_TOD_BRIGHT;
}

void ui_nav_mqtt_start_sleep_cycle(void)
{
    mode_engine_start_cycle();
    ui_nav_go(UI_SCREEN_TOD_BRIGHT);
}

void ui_nav_mqtt_start_rest_cycle(void)
{
    app_config_t *cfg = app_config_get();
    cfg->sleep_sec = 0;
    app_config_save_schedule();
    mode_engine_start_cycle();
    ui_nav_go(UI_SCREEN_TOD_BRIGHT);
}

void ui_nav_mqtt_end_cycle(void)
{
    mode_engine_switch_to_wake();
    if (s_current == UI_SCREEN_TOD_BRIGHT || s_current == UI_SCREEN_TOD_DIM) {
        ui_screen_tod_on_show(s_current == UI_SCREEN_TOD_DIM);
    }
    notify_mqtt_status();
}

void ui_nav_mqtt_start_timer(uint32_t duration_sec, uint8_t style_id)
{
    app_config_t *cfg = app_config_get();
    app_runtime_t *rt = app_runtime_get();

    if (duration_sec == 0) {
        duration_sec = cfg->timer_duration_sec;
    }
    if (duration_sec == 0) {
        return;
    }
    if (style_id >= APP_TIMER_STYLE_COUNT) {
        style_id = 0;
    }

    cfg->timer_duration_sec = duration_sec;
    cfg->timer_style_id = style_id;
    app_config_save_timer();

    rt->active_timer_total_sec = duration_sec;
    rt->active_timer_remaining_sec = duration_sec;
    rt->active_timer_anim_start_ms = now_ms();
    rt->timer_running = true;
    ui_screen_timer_set_running(true);
    if (rt->time_valid) {
        const time_t start = time(NULL);
        const time_t end = start + (time_t)duration_sec;
        rt->active_timer_start_utc = start;
        rt->active_timer_end_utc = end;
        app_checkpoint_set_timer(start, end);
    }
    ui_nav_go(UI_SCREEN_TIMER_BRIGHT);
    notify_mqtt_status();
}

void ui_nav_mqtt_cancel_timer(void)
{
    app_runtime_t *rt = app_runtime_get();
    if (rt->time_valid) {
        app_checkpoint_cancel_timer(time(NULL));
    }
    rt->timer_running = false;
    rt->active_timer_remaining_sec = 0;
    rt->active_timer_total_sec = 0;
    rt->active_timer_start_utc = 0;
    rt->active_timer_end_utc = 0;
    rt->active_timer_anim_start_ms = 0;
    ui_screen_timer_set_running(false);
    if (s_current == UI_SCREEN_TIMER_BRIGHT || s_current == UI_SCREEN_TIMER_DIM ||
        s_current == UI_SCREEN_CONFIRM_END_TIMER) {
        ui_nav_go(UI_SCREEN_MENU);
    }
    notify_mqtt_status();
}

static void mode_engine_tick(void)
{
    app_runtime_t *rt = app_runtime_get();

    if (!rt->cycle_active) {
        return;
    }

    if (rt->mode_remaining_sec > 0) {
        rt->mode_remaining_sec--;
        if (rt->mode_remaining_sec > 0) {
            return;
        }
    }

    mode_engine_advance_if_expired();
}

typedef struct {
    bool time_ok;
} loading_async_t;

static void loading_boot_async_cb(void *user_data)
{
    loading_async_t *arg = (loading_async_t *)user_data;
    if (arg == NULL) {
        return;
    }

    if (s_current != UI_SCREEN_LOADING) {
        free(arg);
        return;
    }

    if (arg->time_ok) {
        if (app_config_timezone_unset()) {
            ui_nav_go(UI_SCREEN_STARTUP_TIMEZONE);
        } else {
            app_time_apply_from_config();
            ui_nav_go(mode_engine_restore_from_nvs());
        }
    } else if (app_network_setup_ap_active()) {
        ui_nav_go(UI_SCREEN_TOD_BRIGHT);
    } else {
        s_loading_retry_at_ms = now_ms() + LOADING_RETRY_MS;
        ui_screen_loading_set_status(app_network_get_status_text());
    }

    free(arg);
}

static void network_boot_done_cb(bool time_ok, void *user_data)
{
    (void)user_data;
    loading_async_t *arg = calloc(1, sizeof(*arg));
    if (arg == NULL) {
        return;
    }
    arg->time_ok = time_ok;
    lv_async_call(loading_boot_async_cb, arg);
}

static void loading_screen_tick(void)
{
    ui_screen_loading_set_status(app_network_get_status_text());

    if (app_network_boot_sync_active()) {
        return;
    }

    if (app_network_setup_ap_active()) {
        return;
    }

    if (s_loading_retry_at_ms != 0 && now_ms() < s_loading_retry_at_ms) {
        return;
    }

    s_loading_retry_at_ms = 0;
    app_network_start_boot_sync();
}

static void tick_timer_cb(lv_timer_t *t)
{
    (void)t;
    app_runtime_t *rt = app_runtime_get();

    if (s_current == UI_SCREEN_LOADING) {
        loading_screen_tick();
        return;
    }

    mode_engine_tick();
    mqtt_track_runtime_snapshot();

    if (s_current == UI_SCREEN_TOD_BRIGHT || s_current == UI_SCREEN_TOD_DIM) {
        ui_screen_tod_tick();
    }

    if (!rt->timer_running) {
        return;
    }

    if (rt->active_timer_remaining_sec == 0) {
        return;
    }

    rt->active_timer_remaining_sec--;

    ui_screen_timer_tick();

    if (rt->active_timer_remaining_sec == 0) {
        app_checkpoint_clear_timer();
        ui_screen_timer_set_running(false);
        ui_nav_go(UI_SCREEN_TIMER_TRIGGERED);
    }
}

void ui_nav_init(void)
{
    esp_err_t cfg_err = app_config_init();
    if (cfg_err != ESP_OK) {
        ESP_LOGW(TAG, "app_config_init: %s", esp_err_to_name(cfg_err));
    }
    app_time_apply_from_config();

    esp_err_t net_err = app_network_init();
    if (net_err != ESP_OK) {
        ESP_LOGW(TAG, "app_network_init: %s", esp_err_to_name(net_err));
    }
    app_network_set_boot_done_callback(network_boot_done_cb, NULL);

    ui_theme_init();

    ui_screens_build_all(s_screens);

    s_tick_timer = lv_timer_create(tick_timer_cb, 1000, NULL);

    ui_nav_go(UI_SCREEN_SPLASH);
}
