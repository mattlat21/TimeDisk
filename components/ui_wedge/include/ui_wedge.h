/**
 * @file ui_wedge.h
 * @brief Bottom-corner wedge buttons (cancel / confirm) for Wi-Fi wizard.
 *
 * Shapes from docs/wireframes/startup_wizard_ssid.svg; drawn via LV_EVENT_DRAW_MAIN.
 */

#pragma once

#include "lvgl.h"

typedef enum {
    UI_WEDGE_CANCEL = 0,
    UI_WEDGE_CONFIRM,
} ui_wedge_type_t;

#define UI_WEDGE_W_WF 209
#define UI_WEDGE_H_WF 106
#define UI_WEDGE_CANCEL_X_WF 142
#define UI_WEDGE_CANCEL_Y_WF 589
#define UI_WEDGE_CONFIRM_X_WF 376
#define UI_WEDGE_CONFIRM_Y_WF 590

lv_obj_t *ui_wedge_create(lv_obj_t *parent, ui_wedge_type_t type, int x, int y);
