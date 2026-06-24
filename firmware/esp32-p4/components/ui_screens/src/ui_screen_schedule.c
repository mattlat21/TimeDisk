#include "ui_screens_registry.h"
#include "ui_layout.h"
#include "ui_widgets.h"
#include "ui_duration_editor.h"
#include "ui_large_time_picker.h"
#include "ui_wedge.h"
#include "ui_theme.h"
#include "ui_nav.h"
#include "ui_format.h"
#include "app_config.h"

#include <esp_log.h>
#include <stdint.h>

static const char *TAG = "ui_schedule";

#define SCHEDULE_EDITOR_BOX_W      400
#define SCHEDULE_EDITOR_BOX_H      80
#define SCHEDULE_EDITOR_BOX_Y      210
#define SCHEDULE_END_TIME_BOX_Y    UI_LARGE_TIME_PICKER_BOX_Y
#define SCHEDULE_HEADING_Y         40
#define SCHEDULE_DURATION_Y_WF     530

typedef struct {
    lv_obj_t *scr;
    lv_obj_t *lbl_title;
    lv_obj_t *lbl_subtitle;
    lv_obj_t *lbl_duration;
    ui_screen_id_t id;
    ui_duration_editor_bundle_t bundle;
    ui_large_time_picker_bundle_t large_time_picker;
    bool end_time_layout;
    bool wind_down_layout;
} schedule_screen_t;

static schedule_screen_t s_screens[5];
static uint32_t s_wizard_vals[5];

static schedule_screen_t *schedule_screen_at(int idx)
{
    if (idx < 0 || idx >= 5) {
        return NULL;
    }
    return &s_screens[idx];
}

/** Wind-down uses 60 s steps; 1–59 s are step-alignment leftovers and mean zero. */
static void snap_wind_down(int idx)
{
    if (idx != 2 && idx != 4) {
        return;
    }
    uint32_t *val = &s_wizard_vals[idx];
    if (*val > 0 && *val < UI_DURATION_EDITOR_STEP_SEC) {
        *val = 0;
    }
}

static ui_screen_id_t s_sleep_ids[3] = {
    UI_SCREEN_SLEEP_WAKE,
    UI_SCREEN_SLEEP_REST_END,
    UI_SCREEN_SLEEP_WIND_DOWN,
};

static ui_screen_id_t s_rest_ids[2] = {
    UI_SCREEN_REST_REST_END,
    UI_SCREEN_REST_WIND_DOWN,
};

static const char *const s_sleep_title = "Start a Sleep";

static const char *s_sleep_subtitles[3] = {
    "Wake up Time",
    "Rest End Time",
    "Wind Down Time",
};

static const char *s_sleep_subtitles_duration[3] = {
    "Sleep Duration",
    "Rest Duration",
    "Wind Down Duration",
};

static const char *s_rest_titles[2] = {
    "Start a Rest",
    "Start a Rest",
};

static const char *s_rest_subtitles[2] = {
    "End Time",
    "Wind Down Time",
};

static const char *s_rest_subtitles_duration[2] = {
    "Duration",
    "Wind Down Duration",
};

static const char *end_time_heading_for_idx(int idx)
{
    switch (idx) {
    case 0:
        return "Sleep Ends";
    case 1:
    case 3:
        return "Rest Ends";
    default:
        return "";
    }
}

static bool schedule_time_available(void)
{
    return app_runtime_get()->time_valid;
}

static int screen_index(ui_screen_id_t id)
{
    for (int i = 0; i < 3; i++) {
        if (s_sleep_ids[i] == id) {
            return i;
        }
    }
    for (int i = 0; i < 2; i++) {
        if (s_rest_ids[i] == id) {
            return i + 3;
        }
    }
    return 0;
}

static uint32_t wind_down_max_sec(uint32_t gross_next_sec)
{
    if (gross_next_sec == 0) {
        return 0;
    }
    return gross_next_sec - 1;
}

static void clamp_wizard_val(int idx)
{
    schedule_screen_t *ss = schedule_screen_at(idx);
    if (ss == NULL) {
        return;
    }
    ui_duration_editor_cfg_t *cfg = &ss->bundle.cfg;
    uint32_t *val = cfg->value_sec;

    if (val == NULL) {
        return;
    }
    if (cfg->min_sec > 0 && *val < cfg->min_sec) {
        *val = cfg->min_sec;
    }
    if (cfg->max_sec > 0 && *val > cfg->max_sec) {
        *val = cfg->max_sec;
    }
}

static void apply_editor_constraints(int idx)
{
    schedule_screen_t *ss = schedule_screen_at(idx);
    if (ss == NULL) {
        return;
    }
    ui_duration_editor_cfg_t *dcfg = &ss->bundle.cfg;
    ui_large_time_picker_cfg_t *pcfg = ss->end_time_layout ? &ss->large_time_picker.cfg : NULL;

    dcfg->end_time_offset_sec = 0;
    dcfg->max_sec = UI_DURATION_EDITOR_MAX_SEC;
    dcfg->min_sec = 0;

    switch (idx) {
    case 0:
        dcfg->min_sec = UI_DURATION_EDITOR_STEP_SEC;
        dcfg->max_sec = UI_SCHEDULE_REST_MAX_SEC;
        break;
    case 1:
        dcfg->end_time_offset_sec = s_wizard_vals[0];
        dcfg->max_sec = UI_SCHEDULE_REST_MAX_SEC;
        break;
    case 2:
        dcfg->max_sec = wind_down_max_sec(s_wizard_vals[0]);
        break;
    case 3:
        dcfg->min_sec = UI_DURATION_EDITOR_STEP_SEC;
        dcfg->max_sec = UI_SCHEDULE_REST_MAX_SEC;
        break;
    case 4:
        dcfg->max_sec = wind_down_max_sec(s_wizard_vals[3]);
        break;
    default:
        break;
    }

    clamp_wizard_val(idx);
    snap_wind_down(idx);

    if (pcfg != NULL) {
        pcfg->end_time_offset_sec = dcfg->end_time_offset_sec;
        pcfg->max_sec = dcfg->max_sec;
        pcfg->min_sec = dcfg->min_sec;
    }
}

static const char *wind_down_heading(void)
{
    return "Wind Down";
}

static void apply_schedule_labels(int idx)
{
    schedule_screen_t *ss = schedule_screen_at(idx);
    if (ss == NULL) {
        return;
    }
    const bool use_time = schedule_time_available();

    if (ss->wind_down_layout) {
        if (ss->lbl_title != NULL) {
            lv_label_set_text(ss->lbl_title, wind_down_heading());
        }
        return;
    }

    if (ss->end_time_layout) {
        if (ss->lbl_title != NULL) {
            lv_label_set_text(ss->lbl_title, end_time_heading_for_idx(idx));
        }
        if (ss->lbl_subtitle != NULL) {
            lv_obj_add_flag(ss->lbl_subtitle, LV_OBJ_FLAG_HIDDEN);
        }
        return;
    }

    if (ss->lbl_title != NULL) {
        if (idx < 3) {
            lv_label_set_text(ss->lbl_title, s_sleep_title);
        } else {
            lv_label_set_text(ss->lbl_title, s_rest_titles[idx - 3]);
        }
    }
    if (ss->lbl_subtitle != NULL) {
        lv_obj_remove_flag(ss->lbl_subtitle, LV_OBJ_FLAG_HIDDEN);
        if (idx < 3) {
            lv_label_set_text(ss->lbl_subtitle,
                              use_time ? s_sleep_subtitles[idx] : s_sleep_subtitles_duration[idx]);
        } else {
            const int rest_idx = idx - 3;
            lv_label_set_text(ss->lbl_subtitle,
                              use_time ? s_rest_subtitles[rest_idx] : s_rest_subtitles_duration[rest_idx]);
        }
    }
}

static void refresh_finish_time_label(int idx)
{
    schedule_screen_t *ss = schedule_screen_at(idx);
    if (ss == NULL || ss->lbl_duration == NULL || ss->bundle.cfg.value_sec == NULL) {
        return;
    }

    char buf[32];
    ui_format_hh_mm_ampm_after_sec(buf, sizeof(buf),
                                   ss->bundle.cfg.end_time_offset_sec + *ss->bundle.cfg.value_sec);
    lv_label_set_text(ss->lbl_duration, buf);
}

static void refresh_duration_label(int idx)
{
    schedule_screen_t *ss = schedule_screen_at(idx);
    if (ss == NULL || ss->lbl_duration == NULL || ss->bundle.cfg.value_sec == NULL) {
        return;
    }

    char buf[64];
    ui_format_hours_and_minutes(buf, sizeof(buf), *ss->bundle.cfg.value_sec);
    lv_label_set_text(ss->lbl_duration, buf);
}

static void refresh_schedule_editors(int idx)
{
    schedule_screen_t *ss = schedule_screen_at(idx);
    if (ss == NULL) {
        return;
    }
    const bool use_time = schedule_time_available();

    apply_editor_constraints(idx);

    if (ss->end_time_layout) {
        if (use_time) {
            ui_duration_editor_set_visible(&ss->bundle.editor, false);
            ui_large_time_picker_set_visible(&ss->large_time_picker.picker, true);
            ui_large_time_picker_refresh(&ss->large_time_picker.picker, &ss->large_time_picker.cfg);
            refresh_duration_label(idx);
            if (ss->lbl_duration != NULL) {
                lv_obj_remove_flag(ss->lbl_duration, LV_OBJ_FLAG_HIDDEN);
            }
        } else {
            ss->bundle.cfg.show_end_time = false;
            ui_duration_editor_set_visible(&ss->bundle.editor, true);
            ui_duration_editor_refresh(&ss->bundle.editor, &ss->bundle.cfg);
            ui_large_time_picker_set_visible(&ss->large_time_picker.picker, false);
            if (ss->lbl_duration != NULL) {
                lv_obj_add_flag(ss->lbl_duration, LV_OBJ_FLAG_HIDDEN);
            }
        }
    } else if (ss->wind_down_layout) {
        ss->bundle.cfg.show_end_time = false;
        ss->bundle.cfg.show_slider = use_time;
        ss->bundle.cfg.style = UI_DURATION_EDITOR_STYLE_WIND_DOWN;
        ss->bundle.cfg.display = UI_DURATION_DISPLAY_WIND_DOWN;
        ui_duration_editor_set_visible(&ss->bundle.editor, true);
        ui_duration_editor_refresh(&ss->bundle.editor, &ss->bundle.cfg);
        if (use_time) {
            refresh_finish_time_label(idx);
            if (ss->lbl_duration != NULL) {
                lv_obj_remove_flag(ss->lbl_duration, LV_OBJ_FLAG_HIDDEN);
            }
        } else if (ss->lbl_duration != NULL) {
            lv_obj_add_flag(ss->lbl_duration, LV_OBJ_FLAG_HIDDEN);
        }
    }

    apply_schedule_labels(idx);
}

static bool sleep_wake_step_valid(void)
{
    return s_wizard_vals[0] > 0;
}

static bool rest_rest_step_valid(void)
{
    return s_wizard_vals[3] > 0;
}

static bool sleep_wind_down_step_valid(void)
{
    return s_wizard_vals[2] < s_wizard_vals[0];
}

static bool rest_wind_down_step_valid(void)
{
    return s_wizard_vals[4] < s_wizard_vals[3];
}

static void schedule_editor_change_cb(void *user_data)
{
    ui_nav_reset_idle_timer();
    refresh_schedule_editors((int)(intptr_t)user_data);
}

static void schedule_time_change_cb(void *user_data)
{
    ui_nav_reset_idle_timer();
    refresh_schedule_editors((int)(intptr_t)user_data);
}

static void finish_sleep_wizard(void)
{
    if (!sleep_wind_down_step_valid()) {
        return;
    }

    app_config_t *cfg = app_config_get();
    const uint32_t gross_wd = s_wizard_vals[2];

    cfg->wind_down_sec = gross_wd;
    cfg->sleep_sec = s_wizard_vals[0] - gross_wd;
    cfg->rest_sec = s_wizard_vals[1];
    app_config_save_schedule();
    ESP_LOGI(TAG,
             "Sleep cycle: gross_wake=%lus gross_rest_end=%lus gross_wind_down=%lus -> "
             "wind_down=%lus sleep=%lus rest=%lus (time_valid=%d)",
             (unsigned long)s_wizard_vals[0], (unsigned long)s_wizard_vals[1],
             (unsigned long)gross_wd, (unsigned long)cfg->wind_down_sec,
             (unsigned long)cfg->sleep_sec, (unsigned long)cfg->rest_sec,
             schedule_time_available());
    mode_engine_start_cycle();
    ui_nav_go(UI_SCREEN_TOD_BRIGHT);
}

static void finish_rest_wizard(void)
{
    if (!rest_wind_down_step_valid()) {
        return;
    }

    app_config_t *cfg = app_config_get();
    const uint32_t gross_wd = s_wizard_vals[4];

    cfg->wind_down_sec = gross_wd;
    cfg->rest_sec = s_wizard_vals[3] - gross_wd;
    cfg->sleep_sec = 0;
    app_config_save_schedule();
    ESP_LOGI(TAG,
             "Rest cycle: gross_rest_end=%lus gross_wind_down=%lus -> "
             "wind_down=%lus rest=%lus sleep=0 (time_valid=%d)",
             (unsigned long)s_wizard_vals[3], (unsigned long)gross_wd,
             (unsigned long)cfg->wind_down_sec, (unsigned long)cfg->rest_sec,
             schedule_time_available());
    mode_engine_start_cycle();
    ui_nav_go(UI_SCREEN_TOD_BRIGHT);
}

static void next_cb(lv_event_t *e)
{
    ui_screen_id_t id = (ui_screen_id_t)(uintptr_t)lv_event_get_user_data(e);

    if (id == UI_SCREEN_SLEEP_WAKE) {
        if (!sleep_wake_step_valid()) {
            return;
        }
        ui_nav_go(UI_SCREEN_SLEEP_REST_END);
    } else if (id == UI_SCREEN_SLEEP_REST_END) {
        ui_nav_go(UI_SCREEN_SLEEP_WIND_DOWN);
    } else if (id == UI_SCREEN_SLEEP_WIND_DOWN) {
        finish_sleep_wizard();
    } else if (id == UI_SCREEN_REST_REST_END) {
        if (!rest_rest_step_valid()) {
            return;
        }
        ui_nav_go(UI_SCREEN_REST_WIND_DOWN);
    } else if (id == UI_SCREEN_REST_WIND_DOWN) {
        finish_rest_wizard();
    }
}

static void back_cb(lv_event_t *e)
{
    ui_screen_id_t id = (ui_screen_id_t)(uintptr_t)lv_event_get_user_data(e);
    if (id == UI_SCREEN_SLEEP_WAKE) {
        ui_nav_go(UI_SCREEN_MENU);
    } else if (id == UI_SCREEN_SLEEP_REST_END) {
        ui_nav_go(UI_SCREEN_SLEEP_WAKE);
    } else if (id == UI_SCREEN_SLEEP_WIND_DOWN) {
        ui_nav_go(UI_SCREEN_SLEEP_REST_END);
    } else if (id == UI_SCREEN_REST_REST_END) {
        ui_nav_go(UI_SCREEN_MENU);
    } else if (id == UI_SCREEN_REST_WIND_DOWN) {
        ui_nav_go(UI_SCREEN_REST_REST_END);
    }
}

static void attach_wind_down_wedges(lv_obj_t *scr, ui_screen_id_t id)
{
    lv_obj_t *cancel = ui_wedge_create(scr, UI_WEDGE_BACK);
    lv_obj_add_event_cb(cancel, back_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)id);

    lv_obj_t *next = ui_wedge_create(scr, UI_WEDGE_CONFIRM);
    lv_obj_add_event_cb(next, next_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)id);

    lv_obj_move_foreground(cancel);
    lv_obj_move_foreground(next);
}

static void attach_wedges(lv_obj_t *scr, ui_screen_id_t id)
{
    lv_obj_t *cancel = ui_wedge_create(scr, UI_WEDGE_CANCEL);
    lv_obj_add_event_cb(cancel, back_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)id);

    lv_obj_t *next = ui_wedge_create(scr, UI_WEDGE_NEXT);
    lv_obj_add_event_cb(next, next_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)id);

    lv_obj_move_foreground(cancel);
    lv_obj_move_foreground(next);
}

static lv_obj_t *create_end_time_heading(lv_obj_t *scr, const char *text)
{
    const ui_theme_t *t = ui_theme_get();
    lv_obj_t *lbl = lv_label_create(scr);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, t->white, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_42, 0);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, SCHEDULE_HEADING_Y);
    return lbl;
}

static lv_obj_t *create_duration_label(lv_obj_t *scr)
{
    const ui_theme_t *t = ui_theme_get();
    lv_obj_t *lbl = lv_label_create(scr);
    lv_label_set_text(lbl, "");
    lv_obj_set_style_text_color(lbl, t->white, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_34, 0);
    lv_obj_set_width(lbl, UI_DISP - 80);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    const int y = ui_layout_wf_to_content_y(scr, SCHEDULE_DURATION_Y_WF);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, y);
    lv_obj_add_flag(lbl, LV_OBJ_FLAG_HIDDEN);
    return lbl;
}

static lv_obj_t *create_wind_down_finish_label(lv_obj_t *scr)
{
    const ui_theme_t *t = ui_theme_get();
    lv_obj_t *lbl = lv_label_create(scr);
    lv_label_set_text(lbl, "");
    lv_obj_set_style_text_color(lbl, t->white, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_48, 0);
    lv_obj_set_width(lbl, UI_DISP - 80);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    const int y = ui_layout_wf_to_content_y(scr, SCHEDULE_DURATION_Y_WF);
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, y);
    lv_obj_add_flag(lbl, LV_OBJ_FLAG_HIDDEN);
    return lbl;
}

static void build_end_time_screen(lv_obj_t *screens[UI_SCREEN_COUNT], ui_screen_id_t id,
                                  const char *heading, int idx)
{
    schedule_screen_t *ss = &s_screens[idx];

    ss->id = id;
    ss->end_time_layout = true;
    ss->wind_down_layout = false;
    ss->scr = ui_widgets_create_screen();
    screens[id] = ss->scr;

    ss->lbl_title = create_end_time_heading(ss->scr, heading);
    ss->lbl_subtitle = NULL;
    ss->lbl_duration = create_duration_label(ss->scr);

    ss->bundle.cfg = (ui_duration_editor_cfg_t){
        .value_sec = &s_wizard_vals[idx],
        .box_y = SCHEDULE_EDITOR_BOX_Y,
        .box_w = SCHEDULE_EDITOR_BOX_W,
        .box_h = SCHEDULE_EDITOR_BOX_H,
        .show_end_time = false,
        .on_change = schedule_editor_change_cb,
        .user_data = (void *)(intptr_t)idx,
    };
    ui_duration_editor_create(ss->scr, &ss->bundle);

    ss->large_time_picker.cfg = (ui_large_time_picker_cfg_t){
        .value_sec = &s_wizard_vals[idx],
        .box_y = SCHEDULE_END_TIME_BOX_Y,
        .on_change = schedule_time_change_cb,
        .user_data = (void *)(intptr_t)idx,
    };
    ui_large_time_picker_create(ss->scr, &ss->large_time_picker);

    attach_wedges(ss->scr, id);
}

static void build_wind_down_screen(lv_obj_t *screens[UI_SCREEN_COUNT], ui_screen_id_t id, int idx)
{
    schedule_screen_t *ss = &s_screens[idx];

    ss->id = id;
    ss->end_time_layout = false;
    ss->wind_down_layout = true;
    ss->scr = ui_widgets_create_screen();
    screens[id] = ss->scr;

    ss->lbl_title = create_end_time_heading(ss->scr, wind_down_heading());
    ss->lbl_subtitle = NULL;
    ss->lbl_duration = create_wind_down_finish_label(ss->scr);

    ss->bundle.cfg = (ui_duration_editor_cfg_t){
        .value_sec = &s_wizard_vals[idx],
        .box_y = UI_DURATION_EDITOR_WD_BOX_Y_WF,
        .show_end_time = false,
        .show_slider = true,
        .style = UI_DURATION_EDITOR_STYLE_WIND_DOWN,
        .display = UI_DURATION_DISPLAY_WIND_DOWN,
        .on_change = schedule_editor_change_cb,
        .user_data = (void *)(intptr_t)idx,
    };
    ui_duration_editor_create(ss->scr, &ss->bundle);

    attach_wind_down_wedges(ss->scr, id);
}

void ui_screen_schedule_build(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    build_end_time_screen(screens, UI_SCREEN_SLEEP_WAKE, "Sleep Ends", 0);
    build_end_time_screen(screens, UI_SCREEN_SLEEP_REST_END, "Rest Ends", 1);
    build_wind_down_screen(screens, UI_SCREEN_SLEEP_WIND_DOWN, 2);
    build_end_time_screen(screens, UI_SCREEN_REST_REST_END, "Rest Ends", 3);
    build_wind_down_screen(screens, UI_SCREEN_REST_WIND_DOWN, 4);
}

void ui_screen_schedule_on_show(ui_screen_id_t id)
{
    app_config_t *cfg = app_config_get();
    int idx = screen_index(id);

    /* Load NVS only when entering the first step of each wizard; mid-wizard
     * navigation must keep in-progress edits in s_wizard_vals. Gross values
     * reverse the wind-down subtraction applied at save time. */
    if (id == UI_SCREEN_SLEEP_WAKE) {
        s_wizard_vals[0] = cfg->sleep_sec + cfg->wind_down_sec;
        s_wizard_vals[1] = cfg->rest_sec;
        s_wizard_vals[2] = cfg->wind_down_sec;
    } else if (id == UI_SCREEN_REST_REST_END) {
        s_wizard_vals[3] = cfg->rest_sec + cfg->wind_down_sec;
        s_wizard_vals[4] = cfg->wind_down_sec;
    }

    if (idx >= 0 && idx < 5) {
        refresh_schedule_editors(idx);
    }
}

uint32_t ui_screen_schedule_get_sec(void)
{
    return s_wizard_vals[0];
}

void ui_screen_schedule_set_sec(uint32_t sec)
{
    s_wizard_vals[0] = sec;
}

void ui_screen_schedule_apply_theme(void)
{
    for (int i = 0; i < 5; i++) {
        if (s_screens[i].scr != NULL) {
            ui_widgets_style_circle_panel(s_screens[i].scr);
        }
    }
}
