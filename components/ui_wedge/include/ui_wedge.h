/**
 * @file ui_wedge.h
 * @brief Bottom-corner wedge buttons with runtime colour and icon overlay.
 *
 * Shape masks: components/ui_assets/assets/wedge_shape_{left,right}.svg (A8)
 * Icons:       components/ui_assets/assets/icon_wedge_*.svg (RGB565A8)
 */

#pragma once

#include "lvgl.h"

/** Which corner the wedge sits in (mirrored shape). */
typedef enum {
    UI_WEDGE_SIDE_LEFT = 0,
    UI_WEDGE_SIDE_RIGHT,
} ui_wedge_side_t;

/** Built-in white icon overlays (same 209×106 canvas as the wedge). */
typedef enum {
    UI_WEDGE_ICON_NONE = 0,
    UI_WEDGE_ICON_CANCEL_X,
    UI_WEDGE_ICON_CONFIRM_CHECK,
    UI_WEDGE_ICON_NEXT_ARROW,
    UI_WEDGE_ICON_SETTINGS_SPANNER,
} ui_wedge_icon_t;

/** Legacy presets — map to side + default theme colour + icon. */
typedef enum {
    UI_WEDGE_CANCEL = 0,
    UI_WEDGE_CONFIRM,
    UI_WEDGE_NEXT,
    UI_WEDGE_SETTINGS,
} ui_wedge_type_t;

typedef struct {
    ui_wedge_side_t side;
    lv_color_t color;
    ui_wedge_icon_t icon;
} ui_wedge_config_t;

/** Wireframe placement and size (startup_wizard_ssid.svg). */
#define UI_WEDGE_W_WF 209
#define UI_WEDGE_H_WF 106
#define UI_WEDGE_CANCEL_X_WF 142
#define UI_WEDGE_CANCEL_Y_WF 589
#define UI_WEDGE_CONFIRM_X_WF 376
#define UI_WEDGE_CONFIRM_Y_WF 590

/** Opaque handle returned by ui_wedge_button_create (clickable container). */
typedef lv_obj_t ui_wedge_button_t;

ui_wedge_config_t ui_wedge_config_default(ui_wedge_side_t side, ui_wedge_icon_t icon);
ui_wedge_config_t ui_wedge_config_from_type(ui_wedge_type_t type);

ui_wedge_button_t *ui_wedge_button_create(lv_obj_t *parent, const ui_wedge_config_t *cfg, int x, int y);
void ui_wedge_button_set_color(ui_wedge_button_t *btn, lv_color_t color);
void ui_wedge_button_set_icon(ui_wedge_button_t *btn, ui_wedge_icon_t icon);

/** @deprecated Use ui_wedge_button_create() with ui_wedge_config_from_type(). */
lv_obj_t *ui_wedge_create(lv_obj_t *parent, ui_wedge_type_t type, int x, int y);
