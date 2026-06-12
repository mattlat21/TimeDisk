/**
 * @file ui_settings_colours.c
 * @brief Settings -> Colours sub-panel.
 */

#include "ui_screen_settings_internal.h"

#include "ui_screens_registry.h"
#include "ui_theme.h"
#include "ui_widgets.h"
#include "ui_wedge.h"

/* --- Colours (theme swatches) --- */
#define THEME_SWATCH_SIZE          56
#define THEME_SWATCH_GAP           12
#define THEME_SWATCH_COUNT         8
#define THEME_SWATCH_BLOCK_CY_WF   360
#define THEME_ROW_SPACING          80
#define THEME_TITLE_GAP_ABOVE      24
#define THEME_TITLE_LINE_H         36
#define THEME_LABEL_UNDER_GAP      20
#define THEME_ROW_W                  (THEME_SWATCH_COUNT * THEME_SWATCH_SIZE \
                                      + (THEME_SWATCH_COUNT - 1) * THEME_SWATCH_GAP)
#define THEME_ROW_PRIMARY_Y_WF     (THEME_SWATCH_BLOCK_CY_WF - THEME_ROW_SPACING / 2 - THEME_SWATCH_SIZE / 2)
#define THEME_ROW_SECONDARY_Y_WF   (THEME_SWATCH_BLOCK_CY_WF + THEME_ROW_SPACING / 2 - THEME_SWATCH_SIZE / 2)
#define THEME_PRIMARY_TITLE_Y_WF   (THEME_ROW_PRIMARY_Y_WF - THEME_TITLE_GAP_ABOVE - THEME_TITLE_LINE_H)
#define THEME_SECONDARY_LABEL_Y_WF (THEME_ROW_SECONDARY_Y_WF + THEME_SWATCH_SIZE + THEME_LABEL_UNDER_GAP)

static const uint32_t s_palette[THEME_SWATCH_COUNT] = {
    0x7A24BC, 0x6BCA24, 0x5A189A, 0x1D4ED8,
    0xE87A2E, 0x3CB85C, 0x0F5BF9, 0xFFFFFF,
};

typedef enum {
    THEME_SLOT_PRIMARY = 0,
    THEME_SLOT_SECONDARY,
} theme_slot_t;

static lv_obj_t *s_panel;
static lv_obj_t *s_swatch_btns[2][THEME_SWATCH_COUNT];
static lv_obj_t *s_lbl_primary_title;
static lv_obj_t *s_lbl_secondary_title;
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

static void colours_refresh_slot_labels(void)
{
    if (s_lbl_primary_title != NULL) {
        lv_obj_set_style_text_color(s_lbl_primary_title, lv_color_hex(s_primary_rgb), 0);
    }
    if (s_lbl_secondary_title != NULL) {
        lv_obj_set_style_text_color(s_lbl_secondary_title, lv_color_hex(s_secondary_rgb), 0);
    }
}

static void colours_refresh_swatch_highlights(void)
{
    int pri_idx = palette_index_for_rgb(s_primary_rgb);
    int sec_idx = palette_index_for_rgb(s_secondary_rgb);

    colours_refresh_slot_labels();

    for (int i = 0; i < THEME_SWATCH_COUNT; i++) {
        if (s_swatch_btns[THEME_SLOT_PRIMARY][i] != NULL) {
            lv_obj_set_style_border_width(s_swatch_btns[THEME_SLOT_PRIMARY][i],
                                          (i == pri_idx) ? 3 : 0, 0);
            lv_obj_set_style_border_color(s_swatch_btns[THEME_SLOT_PRIMARY][i],
                                          lv_color_white(), 0);
        }
        if (s_swatch_btns[THEME_SLOT_SECONDARY][i] != NULL) {
            lv_obj_set_style_border_width(s_swatch_btns[THEME_SLOT_SECONDARY][i],
                                          (i == sec_idx) ? 3 : 0, 0);
            lv_obj_set_style_border_color(s_swatch_btns[THEME_SLOT_SECONDARY][i],
                                          lv_color_white(), 0);
        }
    }
}

static void colours_apply_preview(void)
{
    app_config_t *cfg = app_config_get();
    cfg->ui_primary_color = s_primary_rgb;
    cfg->ui_secondary_color = s_secondary_rgb;
    ui_theme_apply();
}

static void colours_swatch_cb(lv_event_t *e)
{
    int slot = (int)(uintptr_t)lv_event_get_user_data(e);
    lv_obj_t *btn = lv_event_get_target(e);
    uint32_t rgb = (uint32_t)(uintptr_t)lv_obj_get_user_data(btn);

    s_active_slot = (theme_slot_t)slot;
    if (slot == THEME_SLOT_PRIMARY) {
        s_primary_rgb = rgb;
    } else {
        s_secondary_rgb = rgb;
    }
    colours_refresh_swatch_highlights();
    colours_apply_preview();
    ui_settings_idle_cb(NULL);
}

static void colours_build_swatch_row(lv_obj_t *parent, theme_slot_t slot, int row_y_wf)
{
    lv_obj_t *row = lv_obj_create(parent);
    int x = 0;

    lv_obj_set_size(row, THEME_ROW_W, THEME_SWATCH_SIZE);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(row, LV_ALIGN_TOP_MID, 0, ui_settings_wf_y(parent, row_y_wf));

    for (int i = 0; i < THEME_SWATCH_COUNT; i++) {
        lv_obj_t *btn = lv_button_create(row);
        lv_obj_set_size(btn, THEME_SWATCH_SIZE, THEME_SWATCH_SIZE);
        lv_obj_set_pos(btn, x, 0);
        lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(s_palette[i]), 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_user_data(btn, (void *)(uintptr_t)s_palette[i]);
        lv_obj_add_event_cb(btn, colours_swatch_cb, LV_EVENT_CLICKED, (void *)(uintptr_t)slot);
        s_swatch_btns[slot][i] = btn;
        x += THEME_SWATCH_SIZE + THEME_SWATCH_GAP;
    }
}

void ui_settings_colours_sync_from_draft(void)
{
    app_config_t *draft = ui_settings_draft();
    s_primary_rgb = draft->ui_primary_color;
    s_secondary_rgb = draft->ui_secondary_color;
    colours_refresh_swatch_highlights();
}

static void colours_save_cb(lv_event_t *e)
{
    (void)e;
    app_config_t *cfg = app_config_get();
    app_config_t *draft = ui_settings_draft();
    app_config_t *saved = ui_settings_saved();

    draft->ui_primary_color = s_primary_rgb;
    draft->ui_secondary_color = s_secondary_rgb;
    draft->theme_set = true;
    cfg->ui_primary_color = draft->ui_primary_color;
    cfg->ui_secondary_color = draft->ui_secondary_color;
    cfg->theme_set = true;

    saved->ui_primary_color = draft->ui_primary_color;
    saved->ui_secondary_color = draft->ui_secondary_color;
    saved->theme_set = true;

    app_config_save_theme();
    ui_theme_apply();
    ui_settings_show_panel(PANEL_HUB);
}

lv_obj_t *ui_settings_colours_build(void)
{
    s_panel = ui_settings_create_sub_panel("Colours");

    s_lbl_primary_title = lv_label_create(s_panel);
    lv_label_set_text(s_lbl_primary_title, "Primary");
    lv_obj_set_style_text_font(s_lbl_primary_title, &lv_font_montserrat_26, 0);
    lv_obj_align(s_lbl_primary_title, LV_ALIGN_TOP_MID, 0,
                 ui_settings_wf_y(s_panel, THEME_PRIMARY_TITLE_Y_WF));

    colours_build_swatch_row(s_panel, THEME_SLOT_PRIMARY, THEME_ROW_PRIMARY_Y_WF);
    colours_build_swatch_row(s_panel, THEME_SLOT_SECONDARY, THEME_ROW_SECONDARY_Y_WF);

    s_lbl_secondary_title = lv_label_create(s_panel);
    lv_label_set_text(s_lbl_secondary_title, "Secondary");
    lv_obj_set_style_text_font(s_lbl_secondary_title, &lv_font_montserrat_26, 0);
    lv_obj_align(s_lbl_secondary_title, LV_ALIGN_TOP_MID, 0,
                 ui_settings_wf_y(s_panel, THEME_SECONDARY_LABEL_Y_WF));

    colours_refresh_slot_labels();
    ui_settings_attach_panel_wedges(s_panel, PANEL_COLOURS, colours_save_cb);

    return s_panel;
}

void ui_settings_colours_apply_theme(void)
{
    (void)s_active_slot;
    colours_refresh_swatch_highlights();
}

