/**
 * @file ui_nav.c
 * @brief Screen routing, idle timeouts, mode engine stub, and adult-auth session.
 */

#include "ui_nav.h"
#include "ui_screens_registry.h"
#include "ui_theme.h"
#include "app_config.h"
#include "app_time.h"
#include "app_network.h"
#include "bsp/esp-bsp.h"
#include <esp_err.h>
#include <esp_log.h>
#include <esp_random.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

#define TOD_FADE_TO_DIM_MS    10000U
#define TOD_FADE_TO_BRIGHT_MS 1000U
#define TOD_FADE_TICK_MS      16U

static uint32_t s_tod_menu_deadline_ms;
static bool s_tod_menu_timed_out;
static lv_timer_t *s_tod_fade_timer;

typedef struct {
    bool active;
    uint32_t start_ms;
    uint32_t duration_ms;
    uint8_t start_brightness;
    uint8_t end_brightness;
    ui_screen_id_t end_screen;
    bool on_dim_screen;
    bool to_dim;
} tod_fade_t;

static tod_fade_t s_tod_fade;

static void idle_timer_cb(lv_timer_t *t);
static void on_enter(ui_screen_id_t screen);

static uint32_t now_ms(void)
{
    return lv_tick_get();
}

void ui_nav_set_brightness(uint8_t pct)
{
    app_runtime_get()->display_brightness = pct;
    bsp_display_brightness_set(pct);
}

void ui_nav_apply_dim(bool dim)
{
    ui_nav_set_brightness(dim ? UI_NAV_BRIGHTNESS_DIM : UI_NAV_BRIGHTNESS_BRIGHT);
}

static uint8_t brightness_lerp(uint8_t from, uint8_t to, uint32_t elapsed_ms, uint32_t duration_ms)
{
    if (duration_ms == 0 || elapsed_ms >= duration_ms) {
        return to;
    }
    int32_t delta = (int32_t)to - (int32_t)from;
    int32_t value = (int32_t)from + (delta * (int32_t)elapsed_ms) / (int32_t)duration_ms;
    if (value < 0) {
        return 0;
    }
    if (value > 100) {
        return 100;
    }
    return (uint8_t)value;
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
    uint32_t elapsed = now_ms() - s_tod_fade.start_ms;
    if (elapsed > duration) {
        elapsed = duration;
    }

    ui_nav_set_brightness(
        brightness_lerp(s_tod_fade.start_brightness, s_tod_fade.end_brightness, elapsed, duration));

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

    s_tod_fade.active = true;
    s_tod_fade.start_ms = now_ms();
    s_tod_fade.to_dim = to_dim;
    s_tod_fade.duration_ms = to_dim ? TOD_FADE_TO_DIM_MS : TOD_FADE_TO_BRIGHT_MS;
    s_tod_fade.end_screen = to_dim ? UI_SCREEN_TOD_DIM : UI_SCREEN_TOD_BRIGHT;
    s_tod_fade.on_dim_screen = !to_dim;

    if (to_dim) {
        uint8_t current = app_runtime_get()->display_brightness;
        s_tod_fade.start_brightness =
            (current > UI_NAV_BRIGHTNESS_BRIGHT) ? UI_NAV_BRIGHTNESS_BRIGHT : current;
        s_tod_fade.end_brightness = UI_NAV_BRIGHTNESS_DIM;
        s_idle_deadline_ms = UINT32_MAX;
        s_tod_menu_deadline_ms = 0;
        ui_screen_tod_set_menu_visible(false);
    } else {
        uint8_t current = app_runtime_get()->display_brightness;
        s_tod_fade.start_brightness =
            (current < UI_NAV_BRIGHTNESS_DIM) ? UI_NAV_BRIGHTNESS_DIM : current;
        s_tod_fade.end_brightness = UI_NAV_BRIGHTNESS_BRIGHT;
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

void mode_engine_start_cycle(void)
{
    app_runtime_t *rt = app_runtime_get();

    rt->cycle_active = true;
    rt->current_mode = APP_MODE_WAKE;
    rt->mode_remaining_sec = 0;
}

static void mode_engine_tick(void)
{
    app_runtime_t *rt = app_runtime_get();
    app_config_t *cfg = app_config_get();

    if (!rt->cycle_active) {
        return;
    }

    if (rt->mode_remaining_sec > 0) {
        rt->mode_remaining_sec--;
        if (rt->mode_remaining_sec > 0) {
            return;
        }
    }

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

    if (!s_tod_fade.active &&
        (s_current == UI_SCREEN_TOD_BRIGHT || s_current == UI_SCREEN_TOD_DIM)) {
        ui_screen_tod_on_show(s_current == UI_SCREEN_TOD_DIM);
    }
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
            ui_nav_go(UI_SCREEN_TOD_BRIGHT);
        }
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
