/**
 * @file ui_screen_startup_theme_wizard.c
 * @brief Boot-time primary/secondary colour picker (preset swatches).
 */

#include "ui_screens_registry.h"
#include "ui_layout.h"
#include "ui_widgets.h"
#include "ui_wedge.h"
#include "ui_theme.h"
#include "ui_nav.h"
#include "app_config.h"
#include <stddef.h>
#include <stdint.h>

#define THEME_RING_BORDER     UI_RING_BORDER_DEFAULT
#define THEME_SWATCH_SIZE     56
#define THEME_SWATCH_GAP      12
#define THEME_SWATCH_COUNT    8
/** Vertical centre of the two swatch rows on the 720 px display. */
#define THEME_SWATCH_BLOCK_CY     360
#define THEME_ROW_SPACING         80
#define THEME_TITLE_GAP_ABOVE     12
#define THEME_TITLE_LINE_H        32
#define THEME_LABEL_UNDER_GAP     8
#define THEME_PREVIEW_LABEL_GAP_X 100
#define THEME_ROW_PRIMARY_Y       (THEME_SWATCH_BLOCK_CY - THEME_ROW_SPACING / 2 - THEME_SWATCH_SIZE / 2)
#define THEME_ROW_SECONDARY_Y     (THEME_SWATCH_BLOCK_CY + THEME_ROW_SPACING / 2 - THEME_SWATCH_SIZE / 2)
#define THEME_TITLE_Y_WF          (THEME_ROW_PRIMARY_Y - THEME_TITLE_GAP_ABOVE - THEME_TITLE_LINE_H)
#define THEME_PREVIEW_LABELS_Y_WF (THEME_ROW_SECONDARY_Y + THEME_SWATCH_SIZE + THEME_LABEL_UNDER_GAP)

typedef enum {
    THEME_SLOT_PRIMARY = 0,
    THEME_SLOT_SECONDARY,
} theme_slot_t;

static const uint32_t s_palette[THEME_SWATCH_COUNT] = {
    0x7A24BC,
    0x6BCA24,
    0x5A189A,
    0x1D4ED8,
    0xE87A2E,
    0x3CB85C,
    0x0F5BF9,
    0xFFFFFF,
};

static lv_obj_t *s_scr;
static lv_obj_t *s_swatch_btns[2][THEME_SWATCH_COUNT];
static lv_obj_t *s_lbl_preview_primary;
static lv_obj_t *s_lbl_preview_secondary;
static theme_slot_t s_active_slot = THEME_SLOT_PRIMARY;
static uint32_t s_primary_rgb;
static uint32_t s_secondary_rgb;

static int palette_index_for_rgb(uint32_t rgb)
{
    for (int i = 0; i < THEME_SWATCH_COUNT; i++) {
        if (s_palette[i] == rgb) {
            return i;
        }
    }
    return -1;
}

static void apply_colours_to_config(void)
{
    app_config_t *cfg = app_config_get();
    cfg->ui_primary_color = s_primary_rgb;
    cfg->ui_secondary_color = s_secondary_rgb;
    ui_theme_init();
}

static void refresh_preview_labels(void)
{
    const ui_theme_t *t = ui_theme_get();

    if (s_lbl_preview_primary != NULL) {
        lv_obj_set_style_text_color(s_lbl_preview_primary, t->ring, 0);
    }
    if (s_lbl_preview_secondary != NULL) {
        lv_obj_set_style_text_color(s_lbl_preview_secondary, t->secondary, 0);
    }
}

static void refresh_swatch_highlights(void)
{
    int pri_idx = palette_index_for_rgb(s_primary_rgb);
    int sec_idx = palette_index_for_rgb(s_secondary_rgb);

    for (int i = 0; i < THEME_SWATCH_COUNT; i++) {
        lv_obj_t *pri = s_swatch_btns[THEME_SLOT_PRIMARY][i];
        lv_obj_t *sec = s_swatch_btns[THEME_SLOT_SECONDARY][i];
        if (pri != NULL) {
            lv_obj_set_style_border_width(pri, (i == pri_idx) ? 3 : 0, 0);
            lv_obj_set_style_border_color(pri, lv_color_white(), 0);
        }
        if (sec != NULL) {
            lv_obj_set_style_border_width(sec, (i == sec_idx) ? 3 : 0, 0);
            lv_obj_set_style_border_color(sec, lv_color_white(), 0);
        }
    }
}

static void refresh_screen_border(void)
{
    const ui_theme_t *t = ui_theme_get();
    lv_obj_set_style_border_color(s_scr, t->ring, 0);
}

static void theme_refresh_all(void)
{
    apply_colours_to_config();
    refresh_screen_border();
    refresh_preview_labels();
    refresh_swatch_highlights();
}

static void theme_wizard_go_next(void)
{
    if (app_config_wifi_ssid_missing()) {
        ui_nav_go(UI_SCREEN_STARTUP_SSID);
    } else if (app_config_wifi_password_unset()) {
        ui_nav_go(UI_SCREEN_STARTUP_PASSWORD);
    } else {
        ui_nav_go(UI_SCREEN_LOADING);
    }
}

static void next_cb(lv_event_t *e)
{
    (void)e;
    app_config_t *cfg = app_config_get();
    cfg->ui_primary_color = s_primary_rgb;
    cfg->ui_secondary_color = s_secondary_rgb;
    cfg->theme_set = true;
    app_config_save_theme();
    ui_theme_init();
    theme_wizard_go_next();
}

static void swatch_cb(lv_event_t *e)
{
    theme_slot_t slot = (theme_slot_t)(uintptr_t)lv_event_get_user_data(e);
    int idx = (int)(uintptr_t)lv_obj_get_user_data(lv_event_get_target(e));
    if (idx < 0 || idx >= THEME_SWATCH_COUNT) {
        return;
    }
    if (slot == THEME_SLOT_PRIMARY) {
        s_primary_rgb = s_palette[idx];
    } else {
        s_secondary_rgb = s_palette[idx];
    }
    s_active_slot = slot;
    theme_refresh_all();
}

static int32_t swatch_row_x0(void)
{
    int32_t total = THEME_SWATCH_COUNT * THEME_SWATCH_SIZE
                    + (THEME_SWATCH_COUNT - 1) * THEME_SWATCH_GAP;
    return (UI_DISP - total) / 2;
}

static void build_swatch_row(lv_obj_t *parent, theme_slot_t slot, int y_wf)
{
    int32_t x = swatch_row_x0();

    for (int i = 0; i < THEME_SWATCH_COUNT; i++) {
        lv_obj_t *btn = lv_button_create(parent);
        lv_obj_set_size(btn, THEME_SWATCH_SIZE, THEME_SWATCH_SIZE);
        lv_obj_set_pos(btn, UI_WF_X((int)x, THEME_RING_BORDER),
                       UI_WF_Y(y_wf, THEME_RING_BORDER));
        lv_obj_set_style_bg_color(btn, ui_theme_from_rgb(s_palette[i]), 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(btn, 12, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        lv_obj_set_user_data(btn, (void *)(uintptr_t)i);
        lv_obj_add_event_cb(btn, swatch_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)slot);
        s_swatch_btns[slot][i] = btn;
        x += THEME_SWATCH_SIZE + THEME_SWATCH_GAP;
    }
}

static lv_obj_t *create_preview_label_paired(lv_obj_t *parent, const char *text, int x_offset)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
    lv_obj_align(
        lbl, LV_ALIGN_TOP_MID, x_offset,
        UI_WF_Y(THEME_PREVIEW_LABELS_Y_WF, THEME_RING_BORDER));
    return lbl;
}

void ui_screen_startup_theme_wizard_build(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    const ui_theme_t *t = ui_theme_get();
    app_config_t *cfg = app_config_get();

    s_primary_rgb = cfg->ui_primary_color;
    s_secondary_rgb = cfg->ui_secondary_color;

    s_scr = ui_widgets_create_screen();
    screens[UI_SCREEN_STARTUP_THEME] = s_scr;
    lv_obj_set_style_border_width(s_scr, THEME_RING_BORDER, 0);
    lv_obj_set_style_border_color(s_scr, t->ring, 0);

    lv_obj_t *title = ui_widgets_create_title(s_scr, "What's your favourite colours?");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, UI_WF_Y(THEME_TITLE_Y_WF, THEME_RING_BORDER));

    build_swatch_row(s_scr, THEME_SLOT_PRIMARY, THEME_ROW_PRIMARY_Y);
    build_swatch_row(s_scr, THEME_SLOT_SECONDARY, THEME_ROW_SECONDARY_Y);

    s_lbl_preview_primary = create_preview_label_paired(s_scr, "Primary", -THEME_PREVIEW_LABEL_GAP_X);
    s_lbl_preview_secondary = create_preview_label_paired(s_scr, "Secondary", THEME_PREVIEW_LABEL_GAP_X);

    lv_obj_t *next = ui_wedge_create(
        s_scr, UI_WEDGE_CONFIRM,
        UI_WF_X(UI_WEDGE_CONFIRM_X_WF, THEME_RING_BORDER),
        UI_WF_Y(UI_WEDGE_CONFIRM_Y_WF, THEME_RING_BORDER));
    lv_obj_add_event_cb(next, next_cb, LV_EVENT_CLICKED, NULL);

    theme_refresh_all();
}

void ui_screen_startup_theme_wizard_on_show(void)
{
    app_config_t *cfg = app_config_get();
    s_primary_rgb = cfg->ui_primary_color;
    s_secondary_rgb = cfg->ui_secondary_color;
    s_active_slot = THEME_SLOT_PRIMARY;
    theme_refresh_all();
}

void ui_screen_startup_theme_wizard_apply_theme(void)
{
    theme_refresh_all();
}
