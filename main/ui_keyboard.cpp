#include "ui_keyboard.h"
#include "ui_common.h"
#include "ui_theme.h"
#include <string.h>

/* Wireframe: docs/wireframes/startup_wizard_ssid.svg — 58 px keys, three rows */

// Size of the keyboard keys
#define UI_KB_KEY_DIAM     58
#define UI_KB_KEY_RADIUS   (UI_KB_KEY_DIAM / 2)

#define UI_KB_ROW1_CY      362
#define UI_KB_ROW2_CY      430
#define UI_KB_ROW3_CY      497
#define UI_KB_MODE_KEY_CX  73
#define UI_KB_MODE_KEY_CY  UI_KB_ROW3_CY
#define UI_KB_SPACE_KEY_CX 612
#define UI_KB_SPACE_KEY_CY UI_KB_ROW3_CY

static const int s_row10_cx[] = {20, 137, 201, 264, 328, 392, 456, 520, 583, 647};
static const int s_row5a_cx[] = {73, 137, 201, 264, 328};
static const int s_row5b_cx[] = {392, 456, 520, 583, 647};
static const int s_row9_cx[] = {102, 166, 230, 293, 357, 421, 485, 549, 612};
static const int s_row7_cx[] = {201, 264, 328, 392, 456, 520, 584};

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
    ui_keyboard_place_chars(layer, kb, "12345", s_row5a_cx, 5, UI_KB_ROW1_CY);
    ui_keyboard_place_chars(layer, kb, "67890", s_row5b_cx, 5, UI_KB_ROW2_CY);
    ui_keyboard_create_key_btn(layer, "spc", UI_KB_SPACE_KEY_CX, UI_KB_SPACE_KEY_CY, ui_keyboard_key_cb,
                               kb, ui_keyboard_key_char_ptr(' '));
}

static void ui_keyboard_build_symbol_layer(lv_obj_t *layer, ui_keyboard_t *kb)
{
    static const char row1[] = "-_.@#$%{}&";
    static const char row2[] = "=<>[]()+/";
    static const char row3[] = ":;'\"|\\^";

    ui_keyboard_place_chars(layer, kb, row1, s_row10_cx, 10, UI_KB_ROW1_CY);
    ui_keyboard_place_chars(layer, kb, row2, s_row10_cx, 10, UI_KB_ROW2_CY);
    ui_keyboard_place_chars(layer, kb, row3, s_row9_cx, 9, UI_KB_ROW3_CY);
    ui_keyboard_create_key_btn(layer, "~", 647, UI_KB_ROW1_CY, ui_keyboard_key_cb, kb,
                               ui_keyboard_key_char_ptr('~'));
    ui_keyboard_create_key_btn(layer, ",", 647, UI_KB_ROW2_CY, ui_keyboard_key_cb, kb,
                               ui_keyboard_key_char_ptr(','));
    ui_keyboard_create_key_btn(layer, "`", 647, UI_KB_ROW3_CY, ui_keyboard_key_cb, kb,
                               ui_keyboard_key_char_ptr('`'));
    ui_keyboard_create_key_btn(layer, "spc", UI_KB_SPACE_KEY_CX, UI_KB_SPACE_KEY_CY, ui_keyboard_key_cb,
                               kb, ui_keyboard_key_char_ptr(' '));
}

static void ui_keyboard_build_letter_layer(lv_obj_t *layer, ui_keyboard_t *kb, const char *r1,
                                           const char *r2, const char *r3)
{
    ui_keyboard_place_chars(layer, kb, r1, s_row10_cx, 10, UI_KB_ROW1_CY);
    ui_keyboard_place_chars(layer, kb, r2, s_row9_cx, 9, UI_KB_ROW2_CY);
    ui_keyboard_place_chars(layer, kb, r3, s_row7_cx, 7, UI_KB_ROW3_CY);
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

    kb->mode_btn = ui_keyboard_create_key_btn(parent, ui_keyboard_mode_btn_label(kb->mode),
                                              UI_KB_MODE_KEY_CX, UI_KB_MODE_KEY_CY, ui_keyboard_mode_cb,
                                              kb, NULL);
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
