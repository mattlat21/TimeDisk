#include "ui_keyboard.h"
#include "ui_common.h"
#include "ui_theme.h"
#include <string.h>

/* Wireframe: docs/wireframes/startup_wizard_ssid.svg — 58 px keys, three rows */

// Size of the keyboard keys
#define UI_KB_KEY_DIAM     58
#define UI_KB_KEY_RADIUS   (UI_KB_KEY_DIAM / 2)

// Spacing between the keyboard keys
#define UI_KB_KEY_SPACING  7
#define UI_KB_ROW_PITCH    (UI_KB_KEY_DIAM + UI_KB_KEY_SPACING)

#define UI_KB_ROW1_CY      362
#define UI_KB_ROW2_CY      430
#define UI_KB_ROW3_CY      497
#define UI_KB_MODE_KEY_CY  UI_KB_ROW3_CY
#define UI_KB_SPACE_KEY_CY UI_KB_ROW3_CY

/* Max keys in any keyboard row */
#define UI_KB_MAX_ROW_KEYS 10

static void ui_keyboard_key_cx(int *cx, int count, int x_offset = 0)
{
    if (cx == NULL || count <= 0) {
        return;
    }
    if (count > UI_KB_MAX_ROW_KEYS) {
        count = UI_KB_MAX_ROW_KEYS;
    }

    const int center = UI_DISP / 2;
    const int span = (count - 1) * UI_KB_ROW_PITCH;
    const int first = center - span / 2 + x_offset;
    for (int i = 0; i < count; i++) {
        cx[i] = first + i * UI_KB_ROW_PITCH;
    }
}

struct ui_keyboard {
    ui_keyboard_config_t config;
    lv_obj_t *layers[UI_KEYBOARD_MODE_COUNT];
    lv_obj_t *mode_btn;
    ui_keyboard_mode_t mode;
};

static char s_key_chars[96][2];
static int s_key_char_idx;
static bool s_module_inited;

static void ui_keyboard_init_key_chars(void)
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
    ui_keyboard_init_key_chars();
    s_module_inited = true;
}

static const char *ui_keyboard_key_char_ptr(char c)
{
    for (int i = 0; i < s_key_char_idx; i++) {
        if (s_key_chars[i][0] == c) {
            return s_key_chars[i];
        }
    }
    return "?";
}

static const char *ui_keyboard_mode_btn_label(ui_keyboard_mode_t mode)
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

static void ui_keyboard_notify_activity(ui_keyboard_t *kb)
{
    if (kb != NULL && kb->config.on_activity != NULL) {
        kb->config.on_activity(kb->config.user_data);
    }
}

static void ui_keyboard_show_layer(ui_keyboard_t *kb)
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
            lv_label_set_text(lab, ui_keyboard_mode_btn_label(kb->mode));
        }
    }
}

static lv_obj_t *ui_keyboard_create_key_btn(lv_obj_t *parent, const char *txt, int cx, int cy,
                                              lv_event_cb_t cb, ui_keyboard_t *kb,
                                              const char *ch_ptr)
{
    const ui_theme_t *t = ui_theme_get();
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, UI_KB_KEY_DIAM, UI_KB_KEY_DIAM);
    lv_obj_set_pos(btn, cx - UI_KB_KEY_RADIUS, cy - UI_KB_KEY_RADIUS);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(btn, t->menu_petal, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_border_width(btn, 0, 0);

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

static void ui_keyboard_key_cb(lv_event_t *e)
{
    ui_keyboard_t *kb = (ui_keyboard_t *)lv_event_get_user_data(e);
    lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
    const char *ch = (const char *)lv_obj_get_user_data(btn);
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
    ui_keyboard_notify_activity(kb);
}

static void ui_keyboard_mode_cb(lv_event_t *e)
{
    ui_keyboard_t *kb = (ui_keyboard_t *)lv_event_get_user_data(e);
    if (kb == NULL) {
        return;
    }
    kb->mode = (ui_keyboard_mode_t)((kb->mode + 1) % UI_KEYBOARD_MODE_COUNT);
    ui_keyboard_show_layer(kb);
    ui_keyboard_notify_activity(kb);
}

static void ui_keyboard_place_chars(lv_obj_t *layer, ui_keyboard_t *kb, const char *keys,
                                    const int *cx, int count, int cy)
{
    for (int i = 0; i < count && keys[i] != '\0'; i++) {
        char label[2] = {keys[i], '\0'};
        ui_keyboard_create_key_btn(layer, label, cx[i], cy, ui_keyboard_key_cb, kb,
                                   ui_keyboard_key_char_ptr(keys[i]));
    }
}

static lv_obj_t *ui_keyboard_create_layer(lv_obj_t *parent)
{
    lv_obj_t *layer = lv_obj_create(parent);
    lv_obj_set_size(layer, UI_DISP, UI_DISP);
    lv_obj_set_pos(layer, 0, 0);
    lv_obj_set_style_bg_opa(layer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(layer, 0, 0);
    lv_obj_remove_flag(layer, LV_OBJ_FLAG_SCROLLABLE);
    return layer;
}

static void ui_keyboard_build_number_layer(lv_obj_t *layer, ui_keyboard_t *kb)
{
    int cx10[UI_KB_MAX_ROW_KEYS];
    int cx9[UI_KB_MAX_ROW_KEYS];
    ui_keyboard_key_cx(cx10, 10);
    ui_keyboard_key_cx(cx9, 9, UI_KB_ROW_PITCH / 2);
    ui_keyboard_place_chars(layer, kb, "12345", cx10, 5, UI_KB_ROW1_CY);
    ui_keyboard_place_chars(layer, kb, "67890", cx10 + 5, 5, UI_KB_ROW2_CY);
    ui_keyboard_create_key_btn(layer, "spc", cx9[8], UI_KB_SPACE_KEY_CY, ui_keyboard_key_cb, kb,
                               ui_keyboard_key_char_ptr(' '));
}

static void ui_keyboard_build_symbol_layer(lv_obj_t *layer, ui_keyboard_t *kb)
{
    static const char row1[] = "-_.@#$%{}&";
    static const char row2[] = "=<>[]()+/";
    static const char row3[] = ":;'\"|\\^";

    int cx10[UI_KB_MAX_ROW_KEYS];
    int cx9[UI_KB_MAX_ROW_KEYS];
    ui_keyboard_key_cx(cx10, 10);
    ui_keyboard_key_cx(cx9, 9, UI_KB_ROW_PITCH / 2);

    ui_keyboard_place_chars(layer, kb, row1, cx10, 10, UI_KB_ROW1_CY);
    ui_keyboard_place_chars(layer, kb, row2, cx10, 10, UI_KB_ROW2_CY);
    ui_keyboard_place_chars(layer, kb, row3, cx9, 9, UI_KB_ROW3_CY);
    ui_keyboard_create_key_btn(layer, "~", cx10[9], UI_KB_ROW1_CY, ui_keyboard_key_cb, kb,
                               ui_keyboard_key_char_ptr('~'));
    ui_keyboard_create_key_btn(layer, ",", cx10[9], UI_KB_ROW2_CY, ui_keyboard_key_cb, kb,
                               ui_keyboard_key_char_ptr(','));
    ui_keyboard_create_key_btn(layer, "`", cx10[9], UI_KB_ROW3_CY, ui_keyboard_key_cb, kb,
                               ui_keyboard_key_char_ptr('`'));
    ui_keyboard_create_key_btn(layer, "spc", cx9[8], UI_KB_SPACE_KEY_CY, ui_keyboard_key_cb, kb,
                               ui_keyboard_key_char_ptr(' '));
}

static void ui_keyboard_build_letter_layer(lv_obj_t *layer, ui_keyboard_t *kb, const char *r1,
                                           const char *r2, const char *r3)
{
    int cx10[UI_KB_MAX_ROW_KEYS];
    int cx9[UI_KB_MAX_ROW_KEYS];
    int cx7[UI_KB_MAX_ROW_KEYS];
    ui_keyboard_key_cx(cx10, 10);
    // ui_keyboard_key_cx(cx9, 9, UI_KB_ROW_PITCH / 2);
    ui_keyboard_key_cx(cx9, 9);
    ui_keyboard_key_cx(cx7, 7, UI_KB_ROW_PITCH / 2);

    ui_keyboard_place_chars(layer, kb, r1, cx10, 10, UI_KB_ROW1_CY);
    ui_keyboard_place_chars(layer, kb, r2, cx9, 9, UI_KB_ROW2_CY);
    ui_keyboard_place_chars(layer, kb, r3, cx7, 7, UI_KB_ROW3_CY);
}

ui_keyboard_t *ui_keyboard_create(lv_obj_t *parent, const ui_keyboard_config_t *config)
{
    if (parent == NULL || config == NULL || config->buf == NULL || config->label == NULL ||
        config->buf_len == 0) {
        return NULL;
    }

    ui_keyboard_module_init();

    ui_keyboard_t *kb = (ui_keyboard_t *)lv_malloc(sizeof(ui_keyboard_t));
    if (kb == NULL) {
        return NULL;
    }
    memset(kb, 0, sizeof(*kb));
    kb->config = *config;
    kb->mode = config->initial_mode;
    if (kb->mode >= UI_KEYBOARD_MODE_COUNT) {
        kb->mode = UI_KEYBOARD_MODE_LOWER;
    }

    kb->layers[UI_KEYBOARD_MODE_LOWER] = ui_keyboard_create_layer(parent);
    ui_keyboard_build_letter_layer(kb->layers[UI_KEYBOARD_MODE_LOWER], kb, "qwertyuiop", "asdfghjkl",
                                   "zxcvbnm");

    kb->layers[UI_KEYBOARD_MODE_UPPER] = ui_keyboard_create_layer(parent);
    ui_keyboard_build_letter_layer(kb->layers[UI_KEYBOARD_MODE_UPPER], kb, "QWERTYUIOP", "ASDFGHJKL",
                                   "ZXCVBNM");

    kb->layers[UI_KEYBOARD_MODE_NUMBER] = ui_keyboard_create_layer(parent);
    ui_keyboard_build_number_layer(kb->layers[UI_KEYBOARD_MODE_NUMBER], kb);

    kb->layers[UI_KEYBOARD_MODE_SYMBOL] = ui_keyboard_create_layer(parent);
    ui_keyboard_build_symbol_layer(kb->layers[UI_KEYBOARD_MODE_SYMBOL], kb);

    int mode_cx[UI_KB_MAX_ROW_KEYS];
    ui_keyboard_key_cx(mode_cx, 10);
    kb->mode_btn = ui_keyboard_create_key_btn(parent, ui_keyboard_mode_btn_label(kb->mode), mode_cx[0],
                                              UI_KB_MODE_KEY_CY, ui_keyboard_mode_cb, kb, NULL);
    lv_obj_t *mode_lab = lv_obj_get_child(kb->mode_btn, 0);
    if (mode_lab != NULL) {
        lv_obj_set_style_text_font(mode_lab, &lv_font_montserrat_20, 0);
    }

    ui_keyboard_show_layer(kb);
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
    ui_keyboard_show_layer(kb);
}
