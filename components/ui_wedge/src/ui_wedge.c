/**
 * @file ui_wedge.c
 * @brief Corner wedge buttons: recoloured A8 shape + optional icon image overlay.
 *
 * Regenerate assets:
 *   rsvg-convert → PNG; managed_components/lvgl__lvgl/scripts/LVGLImage.py
 *   (see components/ui_assets/assets/wedge_shape_*.svg, icon_wedge_*.svg)
 */

#include "ui_wedge.h"
#include "ui_layout.h"
#include "ui_assets.h"
#include "ui_theme.h"

static const lv_image_dsc_t *shape_for_side(ui_wedge_side_t side)
{
    return (side == UI_WEDGE_SIDE_LEFT) ? &wedge_shape_left : &wedge_shape_right;
}

static const lv_image_dsc_t *icon_for_id(ui_wedge_icon_t icon)
{
    switch (icon) {
    case UI_WEDGE_ICON_CANCEL_X:
        return &icon_wedge_cancel;
    case UI_WEDGE_ICON_CONFIRM_CHECK:
        return &icon_wedge_confirm;
    case UI_WEDGE_ICON_NEXT_ARROW:
        return &icon_wedge_next;
    case UI_WEDGE_ICON_SETTINGS_SPANNER:
        return &icon_wedge_settings;
    default:
        return NULL;
    }
}

/** First child: tinted wedge shape (A8). */
static lv_obj_t *wedge_shape_obj(const ui_wedge_button_t *btn)
{
    return lv_obj_get_child(btn, 0);
}

/** Second child when present: icon overlay. */
static lv_obj_t *wedge_icon_obj(const ui_wedge_button_t *btn)
{
    return (lv_obj_get_child_count(btn) > 1) ? lv_obj_get_child(btn, 1) : NULL;
}

static void apply_shape_color(lv_obj_t *shape, lv_color_t color)
{
    lv_obj_set_style_image_recolor(shape, color, 0);
    lv_obj_set_style_image_recolor_opa(shape, LV_OPA_COVER, 0);
}

static void icon_apply(ui_wedge_button_t *btn, ui_wedge_icon_t icon)
{
    lv_obj_t *icon_img = wedge_icon_obj(btn);
    const lv_image_dsc_t *src = icon_for_id(icon);

    if (src == NULL) {
        if (icon_img != NULL) {
            lv_obj_delete(icon_img);
        }
        return;
    }

    if (icon_img == NULL) {
        icon_img = lv_image_create(btn);
        lv_obj_remove_flag(icon_img, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_size(icon_img, UI_WEDGE_W_WF, UI_WEDGE_H_WF);
        lv_obj_set_pos(icon_img, 0, 0);
    }

    lv_image_set_src(icon_img, src);
    lv_obj_move_foreground(icon_img);
}

ui_wedge_config_t ui_wedge_config_default(ui_wedge_side_t side, ui_wedge_icon_t icon)
{
    const ui_theme_t *t = ui_theme_get();
    ui_wedge_config_t cfg = {
        .side = side,
        .icon = icon,
        .color = t->menu_petal,
    };
    return cfg;
}

ui_wedge_config_t ui_wedge_config_from_type(ui_wedge_type_t type)
{
    const ui_theme_t *t = ui_theme_get();

    switch (type) {
    case UI_WEDGE_CANCEL:
        return (ui_wedge_config_t){
            .side = UI_WEDGE_SIDE_LEFT,
            .color = t->orange,
            .icon = UI_WEDGE_ICON_CANCEL_X,
        };
    case UI_WEDGE_NEXT:
        return (ui_wedge_config_t){
            .side = UI_WEDGE_SIDE_RIGHT,
            .color = t->green,
            .icon = UI_WEDGE_ICON_NEXT_ARROW,
        };
    case UI_WEDGE_SETTINGS:
        return (ui_wedge_config_t){
            .side = UI_WEDGE_SIDE_RIGHT,
            .color = t->menu_petal,
            .icon = UI_WEDGE_ICON_SETTINGS_SPANNER,
        };
    default:
        return (ui_wedge_config_t){
            .side = UI_WEDGE_SIDE_RIGHT,
            .color = t->green,
            .icon = UI_WEDGE_ICON_CONFIRM_CHECK,
        };
    }
}

void ui_wedge_default_pos_for_type(ui_wedge_type_t type, int *x_wf_out, int *y_wf_out)
{
    if (x_wf_out == NULL || y_wf_out == NULL) {
        return;
    }
    if (type == UI_WEDGE_CANCEL) {
        *x_wf_out = UI_WEDGE_CANCEL_X_WF;
        *y_wf_out = UI_WEDGE_CANCEL_Y_WF;
    } else {
        *x_wf_out = UI_WEDGE_CONFIRM_X_WF;
        *y_wf_out = UI_WEDGE_CONFIRM_Y_WF;
    }
}

static ui_wedge_button_t *wedge_button_create_impl(lv_obj_t *parent, const ui_wedge_config_t *cfg,
                                                   int x, int y)
{
    lv_obj_t *btn = lv_obj_create(parent);
    lv_obj_set_size(btn, UI_WEDGE_W_WF, UI_WEDGE_H_WF);
    lv_obj_set_pos(btn, x, y);
    lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);

    lv_obj_t *shape = lv_image_create(btn);
    lv_image_set_src(shape, shape_for_side(cfg->side));
    lv_obj_set_size(shape, UI_WEDGE_W_WF, UI_WEDGE_H_WF);
    lv_obj_set_pos(shape, 0, 0);
    lv_obj_remove_flag(shape, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    apply_shape_color(shape, cfg->color);

    icon_apply(btn, cfg->icon);

    return btn;
}

ui_wedge_button_t *ui_wedge_button_create(lv_obj_t *parent, const ui_wedge_config_t *cfg)
{
    if (parent == NULL || cfg == NULL) {
        return NULL;
    }

    lv_obj_t *screen = ui_layout_find_screen(parent);
    if (screen == NULL) {
        return NULL;
    }

    int x_wf = UI_WEDGE_CONFIRM_X_WF;
    int y_wf = UI_WEDGE_CONFIRM_Y_WF;
    if (cfg->side == UI_WEDGE_SIDE_LEFT) {
        x_wf = UI_WEDGE_CANCEL_X_WF;
        y_wf = UI_WEDGE_CANCEL_Y_WF;
    }
    int x = 0;
    int y = 0;
    ui_layout_screen_pos_from_wf(screen, x_wf, y_wf, &x, &y);
    return wedge_button_create_impl(screen, cfg, x, y);
}

ui_wedge_button_t *ui_wedge_button_create_at(lv_obj_t *parent, const ui_wedge_config_t *cfg, int x,
                                             int y)
{
    if (parent == NULL || cfg == NULL) {
        return NULL;
    }
    return wedge_button_create_impl(parent, cfg, x, y);
}

void ui_wedge_button_set_color(ui_wedge_button_t *btn, lv_color_t color)
{
    lv_obj_t *shape = wedge_shape_obj(btn);
    if (shape != NULL) {
        apply_shape_color(shape, color);
    }
}

void ui_wedge_button_set_icon(ui_wedge_button_t *btn, ui_wedge_icon_t icon)
{
    icon_apply(btn, icon);
}

struct ui_wedge {
    ui_wedge_button_t *btn;
    ui_wedge_type_t type;
    lv_event_cb_t cb;
    void *user_data;
};

static void apply_config_to_button(ui_wedge_button_t *btn, const ui_wedge_config_t *cfg)
{
    lv_obj_t *shape = wedge_shape_obj(btn);
    if (shape != NULL) {
        lv_image_set_src(shape, shape_for_side(cfg->side));
        apply_shape_color(shape, cfg->color);
    }
    ui_wedge_button_set_icon(btn, cfg->icon);
}

lv_obj_t *ui_wedge_create(lv_obj_t *parent, ui_wedge_type_t type)
{
    ui_wedge_config_t cfg = ui_wedge_config_from_type(type);
    return ui_wedge_button_create(parent, &cfg);
}

ui_wedge_t *ui_wedge_create_overlay(lv_obj_t *screen, ui_wedge_type_t type)
{
    if (screen == NULL || lv_obj_get_parent(screen) != NULL) {
        return NULL;
    }

    ui_wedge_t *wedge = lv_malloc(sizeof(*wedge));
    if (wedge == NULL) {
        return NULL;
    }

    ui_wedge_config_t cfg = ui_wedge_config_from_type(type);
    wedge->btn = ui_wedge_button_create(screen, &cfg);
    if (wedge->btn == NULL) {
        lv_free(wedge);
        return NULL;
    }

    wedge->type = type;
    wedge->cb = NULL;
    wedge->user_data = NULL;
    lv_obj_move_foreground(wedge->btn);
    return wedge;
}

void ui_wedge_destroy(ui_wedge_t *wedge)
{
    if (wedge == NULL) {
        return;
    }
    if (wedge->btn != NULL) {
        lv_obj_delete(wedge->btn);
    }
    lv_free(wedge);
}

void ui_wedge_set_visible(ui_wedge_t *wedge, bool visible)
{
    if (wedge == NULL || wedge->btn == NULL) {
        return;
    }
    if (visible) {
        lv_obj_clear_flag(wedge->btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(wedge->btn);
    } else {
        lv_obj_add_flag(wedge->btn, LV_OBJ_FLAG_HIDDEN);
    }
}

void ui_wedge_bind(ui_wedge_t *wedge, ui_wedge_type_t type, lv_event_cb_t cb, void *user_data)
{
    if (wedge == NULL || wedge->btn == NULL) {
        return;
    }

    if (wedge->cb != NULL) {
        lv_obj_remove_event_cb(wedge->btn, wedge->cb);
        wedge->cb = NULL;
    }

    wedge->type = type;
    wedge->user_data = user_data;
    wedge->cb = cb;

    ui_wedge_config_t cfg = ui_wedge_config_from_type(type);
    apply_config_to_button(wedge->btn, &cfg);

    if (cb != NULL) {
        lv_obj_add_event_cb(wedge->btn, cb, LV_EVENT_CLICKED, user_data);
    }
}

void ui_wedge_refresh_theme(ui_wedge_t *wedge)
{
    if (wedge == NULL || wedge->btn == NULL) {
        return;
    }
    ui_wedge_config_t cfg = ui_wedge_config_from_type(wedge->type);
    apply_config_to_button(wedge->btn, &cfg);
}

lv_obj_t *ui_wedge_get_obj(const ui_wedge_t *wedge)
{
    if (wedge == NULL) {
        return NULL;
    }
    return wedge->btn;
}
