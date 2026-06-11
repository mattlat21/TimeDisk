#include "ui_screens_registry.h"
#include "ui_layout.h"
#include "ui_widgets.h"
#include "ui_duration_editor.h"
#include "ui_format.h"
#include "ui_wedge.h"
#include "ui_theme.h"
#include "ui_nav.h"
#include "app_config.h"
#include <math.h>

#define TIMER_STYLE_TILE_W   200
#define TIMER_STYLE_TILE_H   160
#define TIMER_STYLE_TILE_Y   180
#define TIMER_STYLE_TILE_GAP 40

/** Progress ring: outer edge at display bounds, centered on 720×720. */
#define TIMER_RING_DIAMETER  UI_SCREEN_W
#define TIMER_RING_STROKE    56
#define TIMER_RING_VALUE_MAX 1000
#define TIMER_RING_TRACK_COLOR lv_color_make(0x3D, 0x3D, 0x3D)

/** Visual refresh for ring/water (~60 Hz; water level + bob via cached layout). */
#define TIMER_ANIM_PERIOD_MS 16

/** Ring/water reach full progress this many seconds before the countdown hits zero. */
#define TIMER_ANIM_FINISH_REMAINING_SEC 1

/** Timer Set Duration: minimum and tiered +/- step sizes. */
#define TIMER_DURATION_MIN_SEC 5

static uint32_t timer_duration_step_sec(uint32_t value_sec, void *user_data)
{
    (void)user_data;
    if (value_sec < 30) {
        return 5;
    }
    if (value_sec < 120) {
        return 15;
    }
    if (value_sec < 300) {
        return 30;
    }
    if (value_sec < 1800) {
        return 60;
    }
    return 300;
}

#define TIMER_WATER_COLOR    lv_color_make(0x2E, 0x7A, 0xE8)
#define TIMER_WATER_W        UI_SCREEN_W

/** Water surface bobbing at the top only (pixels, milliseconds). Bottom stays screen-anchored. */
#define TIMER_WATER_BOB_AMP_PX     4.0f
#define TIMER_WATER_BOB_PERIOD_MS 2500.0f

typedef struct {
    lv_obj_t *water_clip;
    lv_obj_t *water_fill;
    lv_obj_t *ring_arc;
    int cached_water_h;
    int cached_arc_value;
    lv_coord_t cached_bob_y;
    uint8_t shown_style;
} timer_countdown_vis_t;

#define TIMER_STYLE_UNSET 0xFFU

static void timer_vis_reset_cache(timer_countdown_vis_t *vis)
{
    if (vis == NULL) {
        return;
    }
    vis->cached_water_h = -1;
    vis->cached_arc_value = -1;
    vis->cached_bob_y = (lv_coord_t)9999;
    vis->shown_style = TIMER_STYLE_UNSET;
}

static lv_obj_t *s_scr_duration;
static lv_obj_t *s_scr_style;
static lv_obj_t *s_scr_bright;
static lv_obj_t *s_scr_dim;
static lv_obj_t *s_scr_triggered;
static lv_obj_t *s_scr_confirm;
static lv_obj_t *lbl_countdown_bright;
static lv_obj_t *lbl_countdown_dim;
static lv_obj_t *s_style_tiles[APP_TIMER_STYLE_COUNT];
static timer_countdown_vis_t s_vis_bright;
static timer_countdown_vis_t s_vis_dim;
static uint32_t s_duration_sec;
static uint8_t s_style_id;
static ui_duration_editor_bundle_t s_duration_bundle;
static lv_timer_t *s_anim_timer;

static void timer_duration_clamp(void)
{
    if (s_duration_sec < TIMER_DURATION_MIN_SEC) {
        s_duration_sec = TIMER_DURATION_MIN_SEC;
    }
}

static uint32_t timer_total_sec(const app_runtime_t *rt)
{
    if (rt->active_timer_total_sec > 0) {
        return rt->active_timer_total_sec;
    }
    const uint32_t configured = app_config_get()->timer_duration_sec;
    if (configured > 0) {
        return configured;
    }
    return rt->active_timer_remaining_sec > 0 ? rt->active_timer_remaining_sec : 1;
}

static uint32_t timer_now_ms(void)
{
    return lv_tick_get();
}

static void timer_sync_anim_start_ms(app_runtime_t *rt)
{
    const uint32_t total = timer_total_sec(rt);
    if (total == 0) {
        rt->active_timer_anim_start_ms = timer_now_ms();
        return;
    }
    const uint32_t elapsed_sec = total > rt->active_timer_remaining_sec
                                     ? total - rt->active_timer_remaining_sec
                                     : 0;
    rt->active_timer_anim_start_ms = timer_now_ms() - elapsed_sec * 1000U;
}

static float timer_smooth_progress(const app_runtime_t *rt)
{
    const uint32_t total_ms = timer_total_sec(rt) * 1000U;
    if (total_ms == 0) {
        return 1.0f;
    }

    const uint32_t finish_early_ms = TIMER_ANIM_FINISH_REMAINING_SEC * 1000U;
    if (total_ms <= finish_early_ms) {
        return 1.0f;
    }
    const uint32_t anim_duration_ms = total_ms - finish_early_ms;

    const uint32_t now = timer_now_ms();
    const uint32_t start = rt->active_timer_anim_start_ms;
    if (now <= start) {
        return 0.0f;
    }
    const uint32_t elapsed_ms = now - start;
    if (elapsed_ms >= anim_duration_ms) {
        return 1.0f;
    }
    return (float)elapsed_ms / (float)anim_duration_ms;
}

static void timer_layout_ring(timer_countdown_vis_t *vis, float progress)
{
    if (vis == NULL || vis->ring_arc == NULL) {
        return;
    }

    const int32_t value = (int32_t)(progress * (float)TIMER_RING_VALUE_MAX);
    if (value != vis->cached_arc_value) {
        lv_arc_set_value(vis->ring_arc, value);
        vis->cached_arc_value = value;
    }
}

static lv_coord_t timer_water_bob_y(void)
{
    if (timer_total_sec(app_runtime_get()) < 60) {
        return 0;
    }
    const uint32_t now = timer_now_ms();
    const float phase = (TIMER_WATER_BOB_PERIOD_MS > 0.0f)
                            ? (2.0f * (float)M_PI * ((float)now / TIMER_WATER_BOB_PERIOD_MS))
                            : 0.0f;
    return (lv_coord_t)lroundf(TIMER_WATER_BOB_AMP_PX * sinf(phase));
}

static void timer_layout_water(timer_countdown_vis_t *vis, float progress)
{
    if (vis == NULL || vis->water_fill == NULL) {
        return;
    }

    lv_obj_t *water = vis->water_fill;
    int fill_h = (int)((float)UI_SCREEN_H * progress);
    if (fill_h < 1) {
        fill_h = 1;
    }
    if (fill_h > (int)UI_SCREEN_H) {
        fill_h = (int)UI_SCREEN_H;
    }

    const lv_coord_t bob_y = timer_water_bob_y();
    int display_h = fill_h + (int)bob_y;
    if (display_h < 1) {
        display_h = 1;
    }
    if (display_h > (int)UI_SCREEN_H) {
        display_h = (int)UI_SCREEN_H;
    }

    if (display_h != vis->cached_water_h || bob_y != vis->cached_bob_y) {
        lv_obj_set_size(water, TIMER_WATER_W, display_h);
        lv_obj_align(water, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_translate_y(water, 0, 0);
        vis->cached_water_h = display_h;
        vis->cached_bob_y = bob_y;
    }
}

static void timer_update_countdown_visuals(timer_countdown_vis_t *vis, lv_obj_t *lbl, uint8_t style_id,
                                         float progress, bool raise_label)
{
    const ui_theme_t *t = ui_theme_get();
    const bool style_changed = vis->shown_style != style_id;

    if (style_id == APP_TIMER_STYLE_RING) {
        if (vis->water_fill != NULL && style_changed) {
            lv_obj_add_flag(vis->water_fill, LV_OBJ_FLAG_HIDDEN);
        }
        if (vis->ring_arc != NULL) {
            if (style_changed) {
                lv_obj_clear_flag(vis->ring_arc, LV_OBJ_FLAG_HIDDEN);
                lv_obj_set_style_arc_color(vis->ring_arc, TIMER_RING_TRACK_COLOR, LV_PART_MAIN);
                lv_obj_set_style_arc_color(vis->ring_arc, t->ring, LV_PART_INDICATOR);
            }
            timer_layout_ring(vis, progress);
        }
    } else {
        if (vis->ring_arc != NULL && style_changed) {
            lv_obj_add_flag(vis->ring_arc, LV_OBJ_FLAG_HIDDEN);
        }
        if (vis->water_fill != NULL) {
            if (style_changed) {
                lv_obj_clear_flag(vis->water_fill, LV_OBJ_FLAG_HIDDEN);
            }
            timer_layout_water(vis, progress);
        }
    }

    vis->shown_style = style_id;

    if (raise_label && lbl != NULL) {
        lv_obj_move_foreground(lbl);
    }
}

static void timer_refresh_active_countdown_visuals(void)
{
    app_runtime_t *rt = app_runtime_get();
    const uint8_t style_id = app_config_get()->timer_style_id;
    const float progress = timer_smooth_progress(rt);
    ui_screen_id_t cur = ui_nav_current();

    if (style_id == APP_TIMER_STYLE_WATER) {
        if (cur == UI_SCREEN_TIMER_BRIGHT) {
            timer_layout_water(&s_vis_bright, progress);
        } else if (cur == UI_SCREEN_TIMER_DIM) {
            timer_layout_water(&s_vis_dim, progress);
        }
        return;
    }

    if (cur == UI_SCREEN_TIMER_BRIGHT) {
        timer_update_countdown_visuals(&s_vis_bright, lbl_countdown_bright, style_id, progress, false);
    } else if (cur == UI_SCREEN_TIMER_DIM) {
        timer_update_countdown_visuals(&s_vis_dim, lbl_countdown_dim, style_id, progress, false);
    }
}

static void timer_refresh_all_countdown_visuals(void)
{
    app_runtime_t *rt = app_runtime_get();
    const uint8_t style_id = app_config_get()->timer_style_id;
    const float progress = timer_smooth_progress(rt);

    timer_update_countdown_visuals(&s_vis_bright, lbl_countdown_bright, style_id, progress, true);
    timer_update_countdown_visuals(&s_vis_dim, lbl_countdown_dim, style_id, progress, true);
}

static void timer_anim_cb(lv_timer_t *t)
{
    (void)t;
    app_runtime_t *rt = app_runtime_get();
    if (!rt->timer_running) {
        return;
    }
    ui_screen_id_t cur = ui_nav_current();
    if (cur != UI_SCREEN_TIMER_BRIGHT && cur != UI_SCREEN_TIMER_DIM) {
        return;
    }
    timer_refresh_active_countdown_visuals();
}

void ui_screen_timer_set_running(bool running)
{
    if (s_anim_timer == NULL) {
        return;
    }
    if (running) {
        lv_timer_resume(s_anim_timer);
    } else {
        lv_timer_pause(s_anim_timer);
    }
}

static void style_refresh_tiles(void)
{
    const ui_theme_t *t = ui_theme_get();
    for (int i = 0; i < APP_TIMER_STYLE_COUNT; i++) {
        if (s_style_tiles[i] == NULL) {
            continue;
        }
        const bool selected = (uint8_t)i == s_style_id;
        lv_obj_set_style_border_width(s_style_tiles[i], selected ? 3 : 0, 0);
        lv_obj_set_style_border_color(s_style_tiles[i], selected ? t->green : t->panel, 0);
    }
}

static void duration_idle_cb(void *user_data)
{
    (void)user_data;
    ui_nav_reset_idle_timer();
}

static void duration_back_cb(lv_event_t *e)
{
    (void)e;
    ui_nav_go(UI_SCREEN_MENU);
}

static void duration_next_cb(lv_event_t *e)
{
    (void)e;
    timer_duration_clamp();
    app_config_get()->timer_duration_sec = s_duration_sec;
    app_config_save_timer();
    ui_nav_go(UI_SCREEN_TIMER_STYLE);
}

static void style_back_cb(lv_event_t *e)
{
    (void)e;
    ui_nav_go(UI_SCREEN_TIMER_DURATION);
}

static void style_next_cb(lv_event_t *e)
{
    (void)e;
    app_config_get()->timer_style_id = s_style_id;
    app_config_save_timer();
    app_runtime_t *rt = app_runtime_get();
    rt->active_timer_total_sec = s_duration_sec;
    rt->active_timer_remaining_sec = s_duration_sec;
    rt->active_timer_anim_start_ms = timer_now_ms();
    rt->timer_running = true;
    ui_screen_timer_set_running(true);
    ui_nav_go(UI_SCREEN_TIMER_BRIGHT);
}

static void style_pick_cb(lv_event_t *e)
{
    s_style_id = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    style_refresh_tiles();
    ui_nav_reset_idle_timer();
}

static void timer_tap_cb(lv_event_t *e)
{
    (void)e;
    if (ui_nav_current() == UI_SCREEN_TIMER_DIM) {
        ui_nav_go(UI_SCREEN_TIMER_BRIGHT);
    }
}

static void timer_end_cb(lv_event_t *e)
{
    (void)e;
    ui_nav_start_aa(UI_SCREEN_TIMER_BRIGHT, UI_SCREEN_CONFIRM_END_TIMER);
}

static void confirm_no_cb(lv_event_t *e)
{
    (void)e;
    ui_nav_go(UI_SCREEN_TIMER_BRIGHT);
}

static void confirm_yes_cb(lv_event_t *e)
{
    (void)e;
    app_runtime_t *rt = app_runtime_get();
    rt->timer_running = false;
    rt->active_timer_remaining_sec = 0;
    rt->active_timer_total_sec = 0;
    rt->active_timer_anim_start_ms = 0;
    ui_screen_timer_set_running(false);
    ui_nav_go(UI_SCREEN_MENU);
}

static void triggered_ok_cb(lv_event_t *e)
{
    (void)e;
    app_runtime_t *rt = app_runtime_get();
    rt->timer_running = false;
    rt->active_timer_remaining_sec = 0;
    rt->active_timer_total_sec = 0;
    rt->active_timer_anim_start_ms = 0;
    ui_screen_timer_set_running(false);
    ui_nav_go(UI_SCREEN_TOD_BRIGHT);
}

static lv_obj_t *style_create_tile(lv_obj_t *parent, const char *name, int x, lv_obj_t **out_lbl)
{
    const ui_theme_t *t = ui_theme_get();
    lv_obj_t *tile = lv_button_create(parent);
    lv_obj_set_size(tile, TIMER_STYLE_TILE_W, TIMER_STYLE_TILE_H);
    lv_obj_set_pos(tile, x, TIMER_STYLE_TILE_Y);
    lv_obj_set_style_radius(tile, 16, 0);
    lv_obj_set_style_bg_color(tile, t->panel, 0);
    lv_obj_set_style_border_width(tile, 0, 0);
    lv_obj_set_style_shadow_width(tile, 0, 0);

    lv_obj_t *lbl = lv_label_create(tile);
    lv_label_set_text(lbl, name);
    lv_obj_set_style_text_color(lbl, t->white, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
    lv_obj_align(lbl, LV_ALIGN_BOTTOM_MID, 0, -10);
    if (out_lbl != NULL) {
        *out_lbl = lbl;
    }

    return tile;
}

static void style_tile_add_ring_preview(lv_obj_t *tile)
{
    const ui_theme_t *t = ui_theme_get();
    lv_obj_t *arc = lv_arc_create(tile);
    lv_obj_set_size(arc, 72, 72);
    lv_obj_align(arc, LV_ALIGN_TOP_MID, 0, 28);
    lv_arc_set_rotation(arc, 270);
    lv_arc_set_bg_angles(arc, 0, 360);
    lv_arc_set_range(arc, 0, TIMER_RING_VALUE_MAX);
    lv_arc_set_value(arc, (int32_t)(0.35f * (float)TIMER_RING_VALUE_MAX));
    lv_obj_set_style_arc_width(arc, 10, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, TIMER_RING_TRACK_COLOR, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, t->ring, LV_PART_INDICATOR);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);
}

static void style_tile_add_water_preview(lv_obj_t *tile)
{
    const int label_band = 36;
    lv_obj_t *water = lv_obj_create(tile);
    lv_obj_set_size(water, TIMER_STYLE_TILE_W, TIMER_STYLE_TILE_H / 2);
    lv_obj_align(water, LV_ALIGN_BOTTOM_MID, 0, -label_band);
    lv_obj_set_style_radius(water, 0, 0);
    lv_obj_set_style_bg_color(water, TIMER_WATER_COLOR, 0);
    lv_obj_set_style_bg_opa(water, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(water, 0, 0);
    lv_obj_remove_flag(water, LV_OBJ_FLAG_CLICKABLE);
}

static void build_duration(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    s_scr_duration = ui_widgets_create_screen();
    screens[UI_SCREEN_TIMER_DURATION] = s_scr_duration;
    s_duration_sec = app_config_get()->timer_duration_sec;
    timer_duration_clamp();

    ui_widgets_create_title(s_scr_duration, "Set Duration");

    s_duration_bundle.cfg = (ui_duration_editor_cfg_t){
        .value_sec = &s_duration_sec,
        .box_y = UI_DURATION_EDITOR_BOX_Y,
        .box_w = UI_DURATION_EDITOR_BOX_W,
        .box_h = UI_DURATION_EDITOR_BOX_H,
        .show_end_time = true,
        .min_sec = TIMER_DURATION_MIN_SEC,
        .display = UI_DURATION_DISPLAY_HUMAN,
        .get_step_sec = timer_duration_step_sec,
        .on_change = duration_idle_cb,
    };
    ui_duration_editor_create(s_scr_duration, &s_duration_bundle);

    lv_obj_t *cancel = ui_wedge_create(s_scr_duration, UI_WEDGE_CANCEL);
    lv_obj_add_event_cb(cancel, duration_back_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *next = ui_wedge_create(s_scr_duration, UI_WEDGE_NEXT);
    lv_obj_add_event_cb(next, duration_next_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_move_foreground(cancel);
    lv_obj_move_foreground(next);
}

static void build_style(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    s_scr_style = ui_widgets_create_screen();
    screens[UI_SCREEN_TIMER_STYLE] = s_scr_style;
    s_style_id = app_config_get()->timer_style_id;
    if (s_style_id >= APP_TIMER_STYLE_COUNT) {
        s_style_id = APP_TIMER_STYLE_RING;
    }

    ui_widgets_create_title(s_scr_style, "Set Style");

    const char *names[] = {"Ring", "Water"};
    const int row_w = APP_TIMER_STYLE_COUNT * TIMER_STYLE_TILE_W
                      + (APP_TIMER_STYLE_COUNT - 1) * TIMER_STYLE_TILE_GAP;
    const int row_x = (UI_SCREEN_W - row_w) / 2;

    for (int i = 0; i < APP_TIMER_STYLE_COUNT; i++) {
        const int x = row_x + i * (TIMER_STYLE_TILE_W + TIMER_STYLE_TILE_GAP);
        lv_obj_t *lbl = NULL;
        lv_obj_t *tile = style_create_tile(s_scr_style, names[i], x, &lbl);
        if (i == APP_TIMER_STYLE_RING) {
            style_tile_add_ring_preview(tile);
        } else {
            style_tile_add_water_preview(tile);
            if (lbl != NULL) {
                lv_obj_move_foreground(lbl);
            }
        }
        lv_obj_add_event_cb(tile, style_pick_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)i);
        s_style_tiles[i] = tile;
    }
    style_refresh_tiles();

    lv_obj_t *back = ui_wedge_create(s_scr_style, UI_WEDGE_CANCEL);
    lv_obj_add_event_cb(back, style_back_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *next = ui_wedge_create(s_scr_style, UI_WEDGE_NEXT);
    lv_obj_add_event_cb(next, style_next_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_move_foreground(back);
    lv_obj_move_foreground(next);
}

static lv_obj_t *timer_create_ring_arc(lv_obj_t *parent)
{
    const ui_theme_t *t = ui_theme_get();
    lv_obj_t *arc = lv_arc_create(parent);
    lv_arc_set_mode(arc, LV_ARC_MODE_NORMAL);
    lv_arc_set_rotation(arc, 270);
    lv_arc_set_bg_angles(arc, 0, 360);
    lv_arc_set_range(arc, 0, TIMER_RING_VALUE_MAX);
    lv_arc_set_value(arc, 0);
    lv_obj_set_style_arc_width(arc, TIMER_RING_STROKE, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, TIMER_RING_STROKE, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(arc, TIMER_RING_TRACK_COLOR, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, t->ring, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_INDICATOR);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(arc, TIMER_RING_DIAMETER, TIMER_RING_DIAMETER);
    lv_obj_align(arc, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_flag(arc, LV_OBJ_FLAG_HIDDEN);
    return arc;
}

static lv_obj_t *timer_create_water_clip(lv_obj_t *parent)
{
    lv_obj_t *clip = lv_obj_create(parent);
    lv_obj_set_size(clip, UI_DISP, UI_DISP);
    lv_obj_set_pos(clip, 0, 0);
    lv_obj_set_style_bg_opa(clip, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(clip, 0, 0);
    lv_obj_set_style_pad_all(clip, 0, 0);
    lv_obj_set_style_radius(clip, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_clip_corner(clip, true, 0);
    lv_obj_remove_flag(clip, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    return clip;
}

static lv_obj_t *timer_create_water_fill(lv_obj_t *parent)
{
    lv_obj_t *water = lv_obj_create(parent);
    lv_obj_set_style_radius(water, 0, 0);
    lv_obj_set_style_bg_color(water, TIMER_WATER_COLOR, 0);
    lv_obj_set_style_bg_opa(water, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(water, 0, 0);
    lv_obj_set_style_pad_all(water, 0, 0);
    lv_obj_set_style_margin_all(water, 0, 0);
    lv_obj_remove_flag(water, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(water, LV_OBJ_FLAG_HIDDEN);
    return water;
}

static void build_countdown(lv_obj_t **scr, lv_obj_t **lbl, timer_countdown_vis_t *vis,
                            ui_screen_id_t id, lv_obj_t *screens[UI_SCREEN_COUNT])
{
    const ui_theme_t *t = ui_theme_get();
    *scr = ui_widgets_create_screen_no_ring();
    screens[id] = *scr;

    timer_vis_reset_cache(vis);
    ui_widgets_attach_screen_edge_fill(*scr);
    vis->water_clip = timer_create_water_clip(*scr);
    vis->water_fill = timer_create_water_fill(vis->water_clip);
    vis->ring_arc = timer_create_ring_arc(*scr);

    *lbl = lv_label_create(*scr);
    lv_obj_set_style_text_color(*lbl, t->white, 0);
    lv_obj_set_style_text_font(*lbl, &lv_font_montserrat_48, 0);
    lv_obj_center(*lbl);

    lv_obj_add_event_cb(*scr, timer_tap_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *end = lv_button_create(*scr);
    lv_obj_set_size(end, 100, 40);
    lv_obj_align(end, LV_ALIGN_BOTTOM_MID, 0, -80);
    lv_obj_set_style_bg_color(end, t->orange, 0);
    lv_obj_t *el = lv_label_create(end);
    lv_label_set_text(el, "End");
    lv_obj_set_style_text_color(el, t->white, 0);
    lv_obj_center(el);
    lv_obj_add_event_cb(end, timer_end_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_move_foreground(end);

    ui_widgets_send_edge_fill_to_back(*scr);
}

static void build_triggered(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    const ui_theme_t *t = ui_theme_get();
    s_scr_triggered = ui_widgets_create_screen();
    screens[UI_SCREEN_TIMER_TRIGGERED] = s_scr_triggered;

    lv_obj_t *lbl = lv_label_create(s_scr_triggered);
    lv_label_set_text(lbl, "Timer Done!");
    lv_obj_set_style_text_color(lbl, t->white, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_26, 0);
    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, -40);

    lv_obj_t *ok = ui_wedge_create(s_scr_triggered, UI_WEDGE_CONFIRM);
    lv_obj_add_event_cb(ok, triggered_ok_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_move_foreground(ok);
}

static void build_confirm(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    const ui_theme_t *t = ui_theme_get();

    s_scr_confirm = ui_widgets_create_screen();
    screens[UI_SCREEN_CONFIRM_END_TIMER] = s_scr_confirm;

    lv_obj_t *prompt = lv_label_create(s_scr_confirm);
    lv_label_set_text(prompt, "Are you sure you want to\ncancel the timer?");
    lv_obj_set_width(prompt, UI_SCREEN_W);
    lv_obj_set_style_text_color(prompt, t->white, 0);
    lv_obj_set_style_text_font(prompt, &lv_font_montserrat_26, 0);
    lv_obj_set_style_text_align(prompt, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(prompt);

    lv_obj_t *cancel = ui_wedge_create(s_scr_confirm, UI_WEDGE_CANCEL);
    lv_obj_add_event_cb(cancel, confirm_no_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *confirm = ui_wedge_create(s_scr_confirm, UI_WEDGE_CONFIRM);
    lv_obj_add_event_cb(confirm, confirm_yes_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_move_foreground(cancel);
    lv_obj_move_foreground(confirm);
}

void ui_screen_timer_build(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    build_duration(screens);
    build_style(screens);
    build_countdown(&s_scr_bright, &lbl_countdown_bright, &s_vis_bright, UI_SCREEN_TIMER_BRIGHT, screens);
    build_countdown(&s_scr_dim, &lbl_countdown_dim, &s_vis_dim, UI_SCREEN_TIMER_DIM, screens);
    build_triggered(screens);
    build_confirm(screens);

    s_anim_timer = lv_timer_create(timer_anim_cb, TIMER_ANIM_PERIOD_MS, NULL);
    lv_timer_pause(s_anim_timer);
}

void ui_screen_timer_on_show(ui_screen_id_t id)
{
    char buf[16];
    app_runtime_t *rt = app_runtime_get();

    if (id == UI_SCREEN_TIMER_DURATION) {
        s_duration_sec = app_config_get()->timer_duration_sec;
        timer_duration_clamp();
        s_duration_bundle.cfg.show_end_time = rt->time_valid;
        ui_duration_editor_refresh(&s_duration_bundle.editor, &s_duration_bundle.cfg);
    }

    if (id == UI_SCREEN_TIMER_STYLE) {
        s_style_id = app_config_get()->timer_style_id;
        if (s_style_id >= APP_TIMER_STYLE_COUNT) {
            s_style_id = APP_TIMER_STYLE_RING;
        }
        style_refresh_tiles();
    }

    ui_format_mm_ss(buf, sizeof(buf), rt->active_timer_remaining_sec);

    if (id == UI_SCREEN_TIMER_BRIGHT || id == UI_SCREEN_TIMER_DIM) {
        if (rt->timer_running) {
            timer_sync_anim_start_ms(rt);
            ui_screen_timer_set_running(true);
        }
    }

    const float progress = timer_smooth_progress(rt);
    const uint8_t style_id = app_config_get()->timer_style_id;

    if (id == UI_SCREEN_TIMER_BRIGHT) {
        timer_vis_reset_cache(&s_vis_bright);
        lv_label_set_text(lbl_countdown_bright, buf);
        timer_update_countdown_visuals(&s_vis_bright, lbl_countdown_bright, style_id, progress, true);
        ui_nav_apply_dim(false);
    } else if (id == UI_SCREEN_TIMER_DIM) {
        timer_vis_reset_cache(&s_vis_dim);
        lv_label_set_text(lbl_countdown_dim, buf);
        timer_update_countdown_visuals(&s_vis_dim, lbl_countdown_dim, style_id, progress, true);
        ui_nav_apply_dim(true);
    }
}

void ui_screen_timer_tick(void)
{
    char buf[16];
    app_runtime_t *rt = app_runtime_get();
    if (!rt->timer_running) {
        return;
    }
    ui_format_mm_ss(buf, sizeof(buf), rt->active_timer_remaining_sec);
    ui_screen_id_t cur = ui_nav_current();
    if (cur == UI_SCREEN_TIMER_BRIGHT) {
        lv_label_set_text(lbl_countdown_bright, buf);
    } else if (cur == UI_SCREEN_TIMER_DIM) {
        lv_label_set_text(lbl_countdown_dim, buf);
    }
}

uint32_t ui_screen_duration_get_sec(void)
{
    return s_duration_sec;
}

uint8_t ui_screen_style_get_id(void)
{
    return s_style_id;
}

void ui_screen_timer_apply_theme(void)
{
    lv_obj_t *scrs[] = {
        s_scr_duration, s_scr_style, s_scr_bright, s_scr_dim, s_scr_triggered, s_scr_confirm,
    };
    for (size_t i = 0; i < sizeof(scrs) / sizeof(scrs[0]); i++) {
        if (scrs[i] == NULL) {
            continue;
        }
        if (scrs[i] == s_scr_bright || scrs[i] == s_scr_dim) {
            ui_widgets_style_circle_panel_no_ring(scrs[i]);
            ui_widgets_attach_screen_edge_fill(scrs[i]);
        } else {
            ui_widgets_style_circle_panel(scrs[i]);
        }
    }
    ui_duration_editor_apply_theme(&s_duration_bundle.editor);
    timer_refresh_all_countdown_visuals();
    style_refresh_tiles();
}
