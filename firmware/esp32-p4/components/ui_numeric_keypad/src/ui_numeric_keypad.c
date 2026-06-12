/**
 * @file ui_numeric_keypad.c
 * @brief Standard numeric keypad widget (1-9, 0, backspace).
 */

#include "ui_numeric_keypad.h"

#include "ui_layout.h"
#include "ui_widgets.h"

#include <stdio.h>

struct ui_numeric_keypad {
    lv_obj_t *container;
    ui_numeric_keypad_cfg_t cfg;
};

#define UI_NKP_BTN 76
#define UI_NKP_GAP 8
#define UI_NKP_COLS 3
#define UI_NKP_GRID_W (UI_NKP_COLS * UI_NKP_BTN + (UI_NKP_COLS - 1) * UI_NKP_GAP)

/* Standard absolute placement (wireframe LCD coords). */
#define UI_NKP_Y_WF 252
#define UI_NKP_X_WF (UI_SCREEN_CX - (UI_NKP_GRID_W / 2))

static void add_digit_btn_cfg(ui_numeric_keypad_t *kp, const char *digit, int x, int y)
{
    lv_obj_t *b = ui_widgets_create_keypad_btn(kp->container, digit, x, y, UI_NKP_BTN);
    if (kp->cfg.digit_cb != NULL) {
        lv_obj_add_event_cb(b, kp->cfg.digit_cb, LV_EVENT_CLICKED, kp->cfg.user_ctx);
    }
    /* Store digit string on the target object. */
    lv_obj_set_user_data(b, (void *)digit);
}

static void build_keys(ui_numeric_keypad_t *kp)
{
    if (kp == NULL || kp->container == NULL) {
        return;
    }

    static const char *keys[10] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"};
    for (int i = 0; i < 9; i++) {
        const int row = i / 3;
        const int col = i % 3;
        add_digit_btn_cfg(kp, keys[i],
                          col * (UI_NKP_BTN + UI_NKP_GAP),
                          row * (UI_NKP_BTN + UI_NKP_GAP));
    }

    /* Bottom row: [empty] [0] [backspace] */
    add_digit_btn_cfg(kp, "0",
                      1 * (UI_NKP_BTN + UI_NKP_GAP),
                      3 * (UI_NKP_BTN + UI_NKP_GAP));

    lv_obj_t *bs = ui_widgets_create_keypad_btn(kp->container, LV_SYMBOL_BACKSPACE,
                                                2 * (UI_NKP_BTN + UI_NKP_GAP),
                                                3 * (UI_NKP_BTN + UI_NKP_GAP),
                                                UI_NKP_BTN);
    if (kp->cfg.backspace_cb != NULL) {
        lv_obj_add_event_cb(bs, kp->cfg.backspace_cb, LV_EVENT_CLICKED, kp->cfg.user_ctx);
    }
}

ui_numeric_keypad_t *ui_numeric_keypad_create(lv_obj_t *parent, const ui_numeric_keypad_cfg_t *cfg)
{
    if (parent == NULL || cfg == NULL) {
        return NULL;
    }

    ui_numeric_keypad_t *kp = lv_malloc(sizeof(*kp));
    if (kp == NULL) {
        return NULL;
    }
    kp->cfg = *cfg;

    /* Child of @p parent so keypad hides when panel is hidden (not a screen overlay). */
    kp->container = lv_obj_create(parent);
    lv_obj_set_style_bg_opa(kp->container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(kp->container, 0, 0);
    lv_obj_set_style_pad_all(kp->container, 0, 0);
    lv_obj_remove_flag(kp->container, LV_OBJ_FLAG_SCROLLABLE);

    int start_x = 0;
    int start_y = 0;
    ui_layout_parent_pos_from_wf(parent, UI_NKP_X_WF, UI_NKP_Y_WF, &start_x, &start_y);

    lv_obj_set_size(kp->container, UI_NKP_GRID_W, 4 * UI_NKP_BTN + 3 * UI_NKP_GAP);
    lv_obj_set_pos(kp->container, start_x, start_y);

    build_keys(kp);

    return kp;
}

ui_numeric_keypad_t *ui_numeric_keypad_create_overlay(lv_obj_t *screen, const ui_numeric_keypad_cfg_t *cfg)
{
    if (screen == NULL || lv_obj_get_parent(screen) != NULL) {
        return NULL;
    }
    ui_numeric_keypad_t *kp = ui_numeric_keypad_create(screen, cfg);
    if (kp == NULL) {
        return NULL;
    }
    int x = 0;
    int y = 0;
    ui_layout_screen_pos_from_wf(screen, UI_NKP_X_WF, UI_NKP_Y_WF, &x, &y);
    lv_obj_set_pos(kp->container, x, y);
    lv_obj_move_foreground(kp->container);
    return kp;
}

void ui_numeric_keypad_set_cfg(ui_numeric_keypad_t *kp, const ui_numeric_keypad_cfg_t *cfg)
{
    if (kp == NULL || cfg == NULL) {
        return;
    }
    kp->cfg = *cfg;
    if (kp->container != NULL) {
        /* Rebuild key objects to rebind callbacks/user_ctx. */
        lv_obj_clean(kp->container);
        build_keys(kp);
    }
}

void ui_numeric_keypad_set_visible(ui_numeric_keypad_t *kp, bool visible)
{
    if (kp == NULL || kp->container == NULL) {
        return;
    }
    if (visible) {
        lv_obj_clear_flag(kp->container, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(kp->container);
    } else {
        lv_obj_add_flag(kp->container, LV_OBJ_FLAG_HIDDEN);
    }
}

lv_obj_t *ui_numeric_keypad_get_container(ui_numeric_keypad_t *kp)
{
    if (kp == NULL) {
        return NULL;
    }
    return kp->container;
}

void ui_numeric_keypad_destroy(ui_numeric_keypad_t *kp)
{
    if (kp == NULL) {
        return;
    }
    if (kp->container != NULL) {
        lv_obj_del(kp->container);
    }
    lv_free(kp);
}

