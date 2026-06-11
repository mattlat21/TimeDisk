/**
 * @file ui_wedge.c
 * @brief Corner wedge buttons: recoloured A8 shape + optional icon image overlay.
 *
 * Regenerate assets: ./scripts/embed_ui_assets.sh
 *   (see components/ui_assets/assets/wedge_shape_<side>/ and icon_wedge_<name>/)
 */

#include "ui_wedge.h"
#include "ui_layout.h"
#include "ui_assets.h"
#include "ui_theme.h"
#include <string.h>

static const lv_image_dsc_t *shape_for_side(ui_wedge_side_t side)
{
    switch (side) {
    case UI_WEDGE_SIDE_LEFT:
        return &wedge_shape_left;
    case UI_WEDGE_SIDE_WIDE:
        return &wedge_shape_wide;
    default:
        return &wedge_shape_right;
    }
}

static void wedge_dims_for_side(ui_wedge_side_t side, lv_coord_t *w_out, lv_coord_t *h_out)
{
    if (w_out == NULL || h_out == NULL) {
        return;
    }
    if (side == UI_WEDGE_SIDE_WIDE) {
        *w_out = UI_WEDGE_WIDE_W_WF;
        *h_out = UI_WEDGE_WIDE_H_WF;
    } else {
        *w_out = UI_WEDGE_W_WF;
        *h_out = UI_WEDGE_H_WF;
    }
}

static void wedge_pos_wf_for_side(ui_wedge_side_t side, int *x_wf_out, int *y_wf_out)
{
    if (x_wf_out == NULL || y_wf_out == NULL) {
        return;
    }
    if (side == UI_WEDGE_SIDE_LEFT) {
        *x_wf_out = UI_WEDGE_CANCEL_X_WF;
        *y_wf_out = UI_WEDGE_CANCEL_Y_WF;
    } else if (side == UI_WEDGE_SIDE_WIDE) {
        *x_wf_out = UI_WEDGE_WIDE_X_WF;
        *y_wf_out = UI_WEDGE_WIDE_Y_WF;
    } else {
        *x_wf_out = UI_WEDGE_CONFIRM_X_WF;
        *y_wf_out = UI_WEDGE_CONFIRM_Y_WF;
    }
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
    case UI_WEDGE_ICON_MENU_WIDE_SPANNER:
        return &icon_wedge_menu_wide;
    default:
        return NULL;
    }
}

/** First child: tinted wedge shape (A8). */
static lv_obj_t *wedge_shape_obj(const ui_wedge_button_t *btn)
{
    return lv_obj_get_child(btn, 0);
}

/** Icon overlay image (not the shape mask at child 0, not text labels). */
static lv_obj_t *wedge_icon_obj(const ui_wedge_button_t *btn)
{
    lv_obj_t *shape = wedge_shape_obj(btn);
    const uint32_t n = lv_obj_get_child_count(btn);

    for (uint32_t i = 0; i < n; i++) {
        lv_obj_t *ch = lv_obj_get_child(btn, i);
        if (ch != shape && lv_obj_check_type(ch, &lv_image_class)) {
            return ch;
        }
    }
    return NULL;
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
        lv_coord_t w = 0;
        lv_coord_t h = 0;
        lv_obj_t *shape = wedge_shape_obj(btn);
        if (shape != NULL) {
            w = lv_obj_get_width(shape);
            h = lv_obj_get_height(shape);
        }
        if (w <= 0 || h <= 0) {
            w = UI_WEDGE_W_WF;
            h = UI_WEDGE_H_WF;
        }
        icon_img = lv_image_create(btn);
        lv_obj_remove_flag(icon_img, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_size(icon_img, w, h);
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
    case UI_WEDGE_MENU:
        return (ui_wedge_config_t){
            .side = UI_WEDGE_SIDE_WIDE,
            .color = t->ring,
            .icon = UI_WEDGE_ICON_NONE,
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
    ui_wedge_config_t cfg = ui_wedge_config_from_type(type);
    wedge_pos_wf_for_side(cfg.side, x_wf_out, y_wf_out);
}

static void wedge_apply_geometry(ui_wedge_button_t *btn, const ui_wedge_config_t *cfg)
{
    lv_coord_t w = 0;
    lv_coord_t h = 0;

    wedge_dims_for_side(cfg->side, &w, &h);
    lv_obj_set_size(btn, w, h);

    lv_obj_t *shape = wedge_shape_obj(btn);
    if (shape != NULL) {
        lv_image_set_src(shape, shape_for_side(cfg->side));
        lv_obj_set_size(shape, w, h);
        lv_obj_set_pos(shape, 0, 0);
        apply_shape_color(shape, cfg->color);
    }

    lv_obj_t *icon_img = wedge_icon_obj(btn);
    if (icon_img != NULL) {
        lv_obj_set_size(icon_img, w, h);
        lv_obj_set_pos(icon_img, 0, 0);
    }
}

static ui_wedge_button_t *wedge_button_create_impl(lv_obj_t *parent, const ui_wedge_config_t *cfg,
                                                   int x, int y)
{
    lv_coord_t w = 0;
    lv_coord_t h = 0;

    wedge_dims_for_side(cfg->side, &w, &h);

    lv_obj_t *btn = lv_obj_create(parent);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_pos(btn, x, y);
    lv_obj_remove_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);

    lv_obj_t *shape = lv_image_create(btn);
    lv_image_set_src(shape, shape_for_side(cfg->side));
    lv_obj_set_size(shape, w, h);
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

    int x_wf = 0;
    int y_wf = 0;
    int x = 0;
    int y = 0;
    wedge_pos_wf_for_side(cfg->side, &x_wf, &y_wf);
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

void ui_wedge_button_set_label(ui_wedge_button_t *btn, const char *text)
{
    if (btn == NULL) {
        return;
    }

    icon_apply(btn, UI_WEDGE_ICON_NONE);

    lv_obj_t *lbl = NULL;
    const uint32_t n = lv_obj_get_child_count(btn);
    for (uint32_t i = 1; i < n; i++) {
        lv_obj_t *ch = lv_obj_get_child(btn, i);
        if (lv_obj_check_type(ch, &lv_label_class)) {
            lbl = ch;
            break;
        }
    }

    if (lbl == NULL) {
        const ui_theme_t *t = ui_theme_get();
        lbl = lv_label_create(btn);
        lv_obj_remove_flag(lbl, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_text_color(lbl, t->white, 0);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_26, 0);
    }

    lv_label_set_text(lbl, text != NULL ? text : "");
    lv_obj_center(lbl);
    lv_obj_move_foreground(lbl);
}

struct ui_wedge {
    ui_wedge_button_t *btn;
    ui_wedge_type_t type;
    lv_event_cb_t cb;
    void *user_data;
    char label_text[16];
};

static void apply_config_to_button(ui_wedge_button_t *btn, const ui_wedge_config_t *cfg)
{
    wedge_apply_geometry(btn, cfg);
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
    wedge->label_text[0] = '\0';
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

    if (wedge->label_text[0] != '\0') {
        ui_wedge_button_set_label(wedge->btn, wedge->label_text);
    }
}

void ui_wedge_set_label(ui_wedge_t *wedge, const char *text)
{
    if (wedge == NULL || wedge->btn == NULL) {
        return;
    }
    if (text != NULL) {
        strncpy(wedge->label_text, text, sizeof(wedge->label_text) - 1U);
        wedge->label_text[sizeof(wedge->label_text) - 1U] = '\0';
    } else {
        wedge->label_text[0] = '\0';
    }
    ui_wedge_button_set_label(wedge->btn, text);
}

lv_obj_t *ui_wedge_get_obj(const ui_wedge_t *wedge)
{
    if (wedge == NULL) {
        return NULL;
    }
    return wedge->btn;
}
