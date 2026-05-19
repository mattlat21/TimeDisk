/**
 * @file ui_keyboard.c
 * @brief On-screen keyboard for Wi-Fi wizard (wireframe: startup_wizard_ssid.svg).
 */

#include "ui_keyboard.h"
#include "ui_layout.h"
#include "ui_theme.h"
#include <string.h>

// Button size and spacing
#define UI_KB_KEY_DIAM        58
#define UI_KB_KEY_RADIUS      (UI_KB_KEY_DIAM / 2)
#define UI_KB_KEY_X_SPACING   6
#define UI_KB_ROW_PITCH       (UI_KB_KEY_DIAM + UI_KB_KEY_X_SPACING)
#define UI_KB_ROW_PITCH_HALF  (UI_KB_ROW_PITCH / 2)
#define UI_KB_KEY_Y_SPACING   8
#define UI_KB_COL_PITCH       (UI_KB_KEY_DIAM + UI_KB_KEY_Y_SPACING)

// Row Y Coordinates
#define UI_KB_ROW1_CY_WF      340
#define UI_KB_ROW2_CY_WF      (UI_KB_ROW1_CY_WF + UI_KB_COL_PITCH)
#define UI_KB_ROW3_CY_WF      (UI_KB_ROW2_CY_WF + UI_KB_COL_PITCH)
#define UI_KB_ROW4_CY_WF      (UI_KB_ROW3_CY_WF + UI_KB_COL_PITCH)

#define UI_KB_MAX_ROW_KEYS    10
#define UI_KB_SPACE_SLOTS     3
#define UI_KB_PILL_W          (UI_KB_SPACE_SLOTS * UI_KB_KEY_DIAM + (UI_KB_SPACE_SLOTS - 1) * UI_KB_KEY_X_SPACING)
/** Bottom row: mode | 1-unit gap | space (3 units) | 1-unit gap | backspace. */
#define UI_KB_BOTTOM_W        (UI_KB_KEY_DIAM + UI_KB_ROW_PITCH + UI_KB_PILL_W + UI_KB_ROW_PITCH + UI_KB_KEY_DIAM)
#define UI_KB_PILL_H          UI_KB_KEY_DIAM
#define UI_KB_PILL_RADIUS     (UI_KB_PILL_H / 2)

struct ui_keyboard {
    ui_keyboard_config_t config;
    lv_obj_t *layers[UI_KEYBOARD_MODE_COUNT];
    lv_obj_t *mode_btn;
    ui_keyboard_mode_t mode;
};

static char s_key_chars[96][2];
static int s_key_char_idx;
static bool s_module_inited;

static int keyboard_row_cy(lv_obj_t *screen, int cy_wf)
{
    return ui_layout_wf_to_content_y(screen, cy_wf);
}

static void keyboard_key_cx(lv_obj_t *screen, int *cx, int count, int x_offset)
{
    if (screen == NULL || cx == NULL || count <= 0) {
        return;
    }
    if (count > UI_KB_MAX_ROW_KEYS) {
        count = UI_KB_MAX_ROW_KEYS;
    }

    int32_t center_x;
    ui_layout_get_content_center(screen, &center_x, NULL);
    const int span = (count - 1) * UI_KB_ROW_PITCH;
    const int first = (int)center_x - span / 2 + x_offset;
    for (int i = 0; i < count; i++) {
        cx[i] = first + i * UI_KB_ROW_PITCH;
    }
}

static void keyboard_bottom_centers(lv_obj_t *screen, int *mode_cx, int *space_cx, int *bs_cx)
{
    if (screen == NULL || mode_cx == NULL || space_cx == NULL || bs_cx == NULL) {
        return;
    }

    int32_t center_x;
    ui_layout_get_content_center(screen, &center_x, NULL);
    const int left = (int)center_x - UI_KB_BOTTOM_W / 2;

    *mode_cx = left + UI_KB_KEY_RADIUS;
    const int space_left = left + UI_KB_KEY_DIAM + UI_KB_ROW_PITCH;
    *space_cx = space_left + UI_KB_PILL_W / 2;
    *bs_cx = space_left + UI_KB_PILL_W + UI_KB_ROW_PITCH + UI_KB_KEY_RADIUS;
}

static void init_key_chars(void)
{
    if (s_key_char_idx > 0) {
        return;
    }
    static const char *sets[] = {
        "abcdefghijklmnopqrstuvwxyz",
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
        "0123456789-_.@#$%+&/*=()[]:;'\" <>{}\\|^~,`",
        " ",
    };
    for (size_t s = 0; s < sizeof(sets) / sizeof(sets[0]); s++) {
        for (const char *p = sets[s]; *p != '\0'; p++) {
            if (s_key_char_idx >= (int)(sizeof(s_key_chars) / sizeof(s_key_chars[0]))) {
                return;
            }
            s_key_chars[s_key_char_idx][0] = *p;
            s_key_chars[s_key_char_idx][1] = '\0';
            s_key_char_idx++;
        }
    }
}

void ui_keyboard_module_init(void)
{
    if (s_module_inited) {
        return;
    }
    init_key_chars();
    s_module_inited = true;
}

static const char *key_char_ptr(char c)
{
    for (int i = 0; i < s_key_char_idx; i++) {
        if (s_key_chars[i][0] == c) {
            return s_key_chars[i];
        }
    }
    return "?";
}

static const char *mode_btn_label(ui_keyboard_mode_t mode)
{
    switch (mode) {
    case UI_KEYBOARD_MODE_LOWER:
        return "ABC";
    case UI_KEYBOARD_MODE_UPPER:
        return "123";
    case UI_KEYBOARD_MODE_NUMBER:
        return "#+=";
    case UI_KEYBOARD_MODE_SYMBOL:
        return "abc";
    default:
        return "?";
    }
}

static void notify_activity(ui_keyboard_t *kb)
{
    if (kb != NULL && kb->config.on_activity != NULL) {
        kb->config.on_activity(kb->config.user_data);
    }
}

static void show_layer(ui_keyboard_t *kb)
{
    for (int i = 0; i < UI_KEYBOARD_MODE_COUNT; i++) {
        if (kb->layers[i] == NULL) {
            continue;
        }
        if (i == (int)kb->mode) {
            lv_obj_remove_flag(kb->layers[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(kb->layers[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (kb->mode_btn != NULL) {
        lv_obj_t *lab = lv_obj_get_child(kb->mode_btn, 0);
        if (lab != NULL) {
            lv_label_set_text(lab, mode_btn_label(kb->mode));
        }
    }
}

static lv_obj_t *create_key_btn(lv_obj_t *parent, const char *txt, int cx, int cy,
                                lv_event_cb_t cb, ui_keyboard_t *kb, const char *ch_ptr)
{
    const ui_theme_t *t = ui_theme_get();
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, UI_KB_KEY_DIAM, UI_KB_KEY_DIAM);
    lv_obj_set_pos(btn, cx - UI_KB_KEY_RADIUS, cy - UI_KB_KEY_RADIUS);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(btn, t->menu_petal, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);

    lv_obj_t *lab = lv_label_create(btn);
    lv_label_set_text(lab, txt);
    lv_obj_set_style_text_color(lab, t->white, 0);
    const lv_font_t *font =
        (txt != NULL && txt[0] != '\0' && txt[1] == '\0') ? &lv_font_montserrat_26 : &lv_font_montserrat_20;
    lv_obj_set_style_text_font(lab, font, 0);
    lv_obj_center(lab);

    if (cb != NULL) {
        lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, kb);
        lv_obj_set_user_data(btn, (void *)ch_ptr);
    }
    return btn;
}

/** Wide pill key (e.g. space spanning two normal key widths). */
static lv_obj_t *create_key_pill_btn(lv_obj_t *parent, const char *txt, int cx, int cy,
                                     lv_event_cb_t cb, ui_keyboard_t *kb, const char *ch_ptr)
{
    const ui_theme_t *t = ui_theme_get();
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, UI_KB_PILL_W, UI_KB_PILL_H);
    lv_obj_set_pos(btn, cx - UI_KB_PILL_W / 2, cy - UI_KB_PILL_RADIUS);
    lv_obj_set_style_radius(btn, UI_KB_PILL_RADIUS, 0);
    lv_obj_set_style_bg_color(btn, t->menu_petal, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_pad_all(btn, 0, 0);

    if (txt != NULL && txt[0] != '\0') {
        lv_obj_t *lab = lv_label_create(btn);
        lv_label_set_text(lab, txt);
        lv_obj_set_style_text_color(lab, t->white, 0);
        lv_obj_set_style_text_font(lab, &lv_font_montserrat_20, 0);
        lv_obj_center(lab);
    }

    if (cb != NULL) {
        lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, kb);
        lv_obj_set_user_data(btn, (void *)ch_ptr);
    }
    return btn;
}

static void key_cb(lv_event_t *e)
{
    ui_keyboard_t *kb = lv_event_get_user_data(e);
    lv_obj_t *btn = lv_event_get_target(e);
    const char *ch = lv_obj_get_user_data(btn);
    if (kb == NULL || ch == NULL || ch[0] == '\0' || kb->config.buf == NULL ||
        kb->config.label == NULL) {
        return;
    }
    size_t len = strlen(kb->config.buf);
    if (len + 1 >= kb->config.buf_len) {
        return;
    }
    kb->config.buf[len] = ch[0];
    kb->config.buf[len + 1] = '\0';
    lv_label_set_text(kb->config.label, kb->config.buf);
    notify_activity(kb);
}

static void backspace_cb(lv_event_t *e)
{
    ui_keyboard_t *kb = lv_event_get_user_data(e);
    if (kb == NULL || kb->config.buf == NULL || kb->config.label == NULL) {
        return;
    }
    size_t len = strlen(kb->config.buf);
    if (len == 0) {
        return;
    }
    kb->config.buf[len - 1] = '\0';
    lv_label_set_text(kb->config.label, kb->config.buf);
    notify_activity(kb);
}

static void mode_cb(lv_event_t *e)
{
    ui_keyboard_t *kb = lv_event_get_user_data(e);
    if (kb == NULL) {
        return;
    }
    kb->mode = (ui_keyboard_mode_t)((kb->mode + 1) % UI_KEYBOARD_MODE_COUNT);
    show_layer(kb);
    notify_activity(kb);
}

static void place_chars(lv_obj_t *layer, ui_keyboard_t *kb, const char *keys,
                        const int *cx, int count, int cy)
{
    for (int i = 0; i < count && keys[i] != '\0'; i++) {
        char label[2] = {keys[i], '\0'};
        create_key_btn(layer, label, cx[i], cy, key_cb, kb, key_char_ptr(keys[i]));
    }
}

/** Row 4: mode (parent) | gap | 3-unit space pill | gap | backspace. */
static void build_bottom_row(lv_obj_t *layer, ui_keyboard_t *kb)
{
    lv_obj_t *screen = lv_obj_get_parent(layer);
    int mode_cx = 0;
    int space_cx = 0;
    int bs_cx = 0;
    keyboard_bottom_centers(screen, &mode_cx, &space_cx, &bs_cx);
    const int cy = keyboard_row_cy(screen, UI_KB_ROW4_CY_WF);

    create_key_pill_btn(layer, "", space_cx, cy, key_cb, kb, key_char_ptr(' '));
    lv_obj_t *bs_btn = create_key_btn(layer, LV_SYMBOL_BACKSPACE, bs_cx, cy, backspace_cb, kb, NULL);
    lv_obj_t *bs_lab = lv_obj_get_child(bs_btn, 0);
    if (bs_lab != NULL) {
        lv_obj_set_style_text_font(bs_lab, &lv_font_montserrat_26, 0);
    }
}

static lv_obj_t *create_layer(lv_obj_t *screen)
{
    int32_t cw;
    int32_t ch;
    ui_layout_get_content_size(screen, &cw, &ch);

    lv_obj_t *layer = lv_obj_create(screen);
    lv_obj_set_size(layer, cw, ch);
    lv_obj_set_pos(layer, 0, 0);
    lv_obj_set_style_bg_opa(layer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(layer, 0, 0);
    lv_obj_set_style_pad_all(layer, 0, 0);
    lv_obj_remove_flag(layer, LV_OBJ_FLAG_SCROLLABLE);
    return layer;
}

static void build_number_layer(lv_obj_t *layer, ui_keyboard_t *kb)
{
    lv_obj_t *screen = lv_obj_get_parent(layer);
    int cx10[UI_KB_MAX_ROW_KEYS];
    keyboard_key_cx(screen, cx10, 10, 0);
    place_chars(layer, kb, "!@#$%^&*()", cx10, 10, keyboard_row_cy(screen, UI_KB_ROW1_CY_WF));
    place_chars(layer, kb, "1234567890", cx10, 10, keyboard_row_cy(screen, UI_KB_ROW2_CY_WF));
    build_bottom_row(layer, kb);
}

static void build_symbol_layer(lv_obj_t *layer, ui_keyboard_t *kb)
{
    static const char row1[] = "=+-_.{}[]/";
    static const char row2[] = "<>:;'\"|\\";

    lv_obj_t *screen = lv_obj_get_parent(layer);
    int cx10[UI_KB_MAX_ROW_KEYS];
    int cx9[UI_KB_MAX_ROW_KEYS];
    int cx5[UI_KB_MAX_ROW_KEYS];
    keyboard_key_cx(screen, cx10, 10, 0);
    keyboard_key_cx(screen, cx9, 9, 0);
    keyboard_key_cx(screen, cx5, 6, 0);

    place_chars(layer, kb, row1, cx10, 10, keyboard_row_cy(screen, UI_KB_ROW1_CY_WF));
    place_chars(layer, kb, row2, cx9, 9, keyboard_row_cy(screen, UI_KB_ROW2_CY_WF));

    create_key_btn(layer, "~", cx5[1], keyboard_row_cy(screen, UI_KB_ROW3_CY_WF),
                   key_cb, kb, key_char_ptr('~'));
    create_key_btn(layer, ",", cx5[2], keyboard_row_cy(screen, UI_KB_ROW3_CY_WF),
                   key_cb, kb, key_char_ptr(','));
    create_key_btn(layer, "`", cx5[3], keyboard_row_cy(screen, UI_KB_ROW3_CY_WF),
                   key_cb, kb, key_char_ptr('`'));
    build_bottom_row(layer, kb);
}

static void build_letter_layer(lv_obj_t *layer, ui_keyboard_t *kb, const char *r1,
                               const char *r2, const char *r3)
{
    lv_obj_t *screen = lv_obj_get_parent(layer);
    int cx10[UI_KB_MAX_ROW_KEYS];
    int cx9[UI_KB_MAX_ROW_KEYS];
    int cx7[UI_KB_MAX_ROW_KEYS];
    keyboard_key_cx(screen, cx10, 10, 0);
    keyboard_key_cx(screen, cx9, 9, 0);
    keyboard_key_cx(screen, cx7, 7, UI_KB_ROW_PITCH_HALF);

    place_chars(layer, kb, r1, cx10, 10, keyboard_row_cy(screen, UI_KB_ROW1_CY_WF));
    place_chars(layer, kb, r2, cx9, 9, keyboard_row_cy(screen, UI_KB_ROW2_CY_WF));
    place_chars(layer, kb, r3, cx7, 7, keyboard_row_cy(screen, UI_KB_ROW3_CY_WF));
    build_bottom_row(layer, kb);
}

ui_keyboard_t *ui_keyboard_create(lv_obj_t *parent, const ui_keyboard_config_t *config)
{
    if (parent == NULL || config == NULL || config->buf == NULL || config->label == NULL ||
        config->buf_len == 0) {
        return NULL;
    }

    ui_keyboard_module_init();

    ui_keyboard_t *kb = lv_malloc(sizeof(ui_keyboard_t));
    if (kb == NULL) {
        return NULL;
    }
    memset(kb, 0, sizeof(*kb));
    kb->config = *config;
    kb->mode = config->initial_mode;
    if (kb->mode >= UI_KEYBOARD_MODE_COUNT) {
        kb->mode = UI_KEYBOARD_MODE_LOWER;
    }

    kb->layers[UI_KEYBOARD_MODE_LOWER] = create_layer(parent);
    build_letter_layer(kb->layers[UI_KEYBOARD_MODE_LOWER], kb, "qwertyuiop", "asdfghjkl", "zxcvbnm");

    kb->layers[UI_KEYBOARD_MODE_UPPER] = create_layer(parent);
    build_letter_layer(kb->layers[UI_KEYBOARD_MODE_UPPER], kb, "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM");

    kb->layers[UI_KEYBOARD_MODE_NUMBER] = create_layer(parent);
    build_number_layer(kb->layers[UI_KEYBOARD_MODE_NUMBER], kb);

    kb->layers[UI_KEYBOARD_MODE_SYMBOL] = create_layer(parent);
    build_symbol_layer(kb->layers[UI_KEYBOARD_MODE_SYMBOL], kb);

    int mode_cx = 0;
    int space_cx = 0;
    int bs_cx = 0;
    keyboard_bottom_centers(parent, &mode_cx, &space_cx, &bs_cx);
    kb->mode_btn = create_key_btn(parent, mode_btn_label(kb->mode), mode_cx,
                                  keyboard_row_cy(parent, UI_KB_ROW4_CY_WF),
                                  mode_cb, kb, NULL);
    (void)space_cx;
    (void)bs_cx;
    lv_obj_t *mode_lab = lv_obj_get_child(kb->mode_btn, 0);
    if (mode_lab != NULL) {
        lv_obj_set_style_text_font(mode_lab, &lv_font_montserrat_20, 0);
    }

    show_layer(kb);
    lv_obj_move_foreground(kb->mode_btn);

    return kb;
}

ui_keyboard_mode_t ui_keyboard_get_mode(const ui_keyboard_t *kb)
{
    if (kb == NULL) {
        return UI_KEYBOARD_MODE_LOWER;
    }
    return kb->mode;
}

void ui_keyboard_set_mode(ui_keyboard_t *kb, ui_keyboard_mode_t mode)
{
    if (kb == NULL || mode >= UI_KEYBOARD_MODE_COUNT) {
        return;
    }
    kb->mode = mode;
    show_layer(kb);
}
