/*
 * ui_screen_startup_wifi_wizard.cpp — Boot-time WiFi setup (SSID + password).
 *
 * Wireframe: docs/wireframes/startup_wizard_ssid.svg
 *   - 720×720 circular panel, 14 px purple ring (#7D23BE)
 *   - Title, purple rounded SSID field (563×78 @ x=78 y=215, radius 30)
 *   - Three rows of blue keys (#0F5BF9, Ø≈58) — only three rows fit above corner buttons
 *   - Bottom-left orange (backspace / back), bottom-right green (confirm)
 *
 * Screens built here (registered in ui_screens_registry.cpp):
 *   UI_SCREEN_STARTUP_SSID      — "Wi-Fi Setup", enter network name
 *   UI_SCREEN_STARTUP_PASSWORD  — "Wi-Fi Password", optional passphrase
 *
 * Boot flow (ui_nav.cpp, stubbed until NVS/WiFi stack):
 *   splash → [SSID if missing] → [password if unset] → loading → TOD
 *   See docs/screen_flow.md and docs/data_model.md.
 *
 * Keyboard — four layouts on three rows; only one layout visible at a time.
 * Cycled via the fixed mode key at bottom-left of row 3 (where 'z' would be on QWERTY):
 *   lower → upper → numbers → symbols → lower …
 *   Mode key labels indicate the *next* layout: ABC → 123 → #+= → abc
 *
 * Character set targets WPA2 printable ASCII (0x20–0x7E): letters on ABC layouts,
 * digits on 123, punctuation on #+=. Space appears on number and symbol layouts.
 *
 * Exported getters (tests / future esp_wifi_set_config):
 *   ui_screen_startup_wifi_wizard_ssid_get_text()
 *   ui_screen_startup_wifi_wizard_password_get_text()
 */

#include "ui_screens_registry.h"
#include "ui_common.h"
#include "ui_theme.h"
#include "ui_nav.h"
#include "app_config.h"
#include <stdio.h>
#include <string.h>

/* --- Key geometry (from startup_wizard_ssid.svg circle centers) --- */

#define WIFI_KEY_DIAM       58   /* 2 × r≈29.133 in wireframe */
#define WIFI_KEY_RADIUS     (WIFI_KEY_DIAM / 2)

/* Purple text field — wireframe rect x=78 y=215 w=563 h=78 rx=30 */
#define WIFI_FIELD_X        78
#define WIFI_FIELD_Y        215
#define WIFI_FIELD_W        563
#define WIFI_FIELD_H        78
#define WIFI_FIELD_RADIUS   30

#define WIFI_TITLE_Y_OFFSET 28

/* Corner wedge buttons — lower than y≈330 used on other screens (wireframe ~y 589) */
#define WIFI_BTN_BACK_X     108
#define WIFI_BTN_NEXT_X     (UI_DISP - 108 - 64)
#define WIFI_BTN_Y          600

/* Row vertical centers (cy) for key circles in the wireframe */
#define WIFI_ROW1_CY        362
#define WIFI_ROW2_CY        430
#define WIFI_ROW3_CY        497

/* Mode-cycle key: always on screen, not inside a hidden layer */
#define WIFI_MODE_KEY_CX    73
#define WIFI_MODE_KEY_CY    WIFI_ROW3_CY

/* Space key position on number/symbol layouts (letter row uses 9-wide grid) */
#define WIFI_SPACE_KEY_CX   612
#define WIFI_SPACE_KEY_CY   WIFI_ROW3_CY

/*
 * Horizontal centers (cx) for each row width — copied from SVG <circle cx="…">.
 * Keys are positioned at (cx - WIFI_KEY_RADIUS, cy - WIFI_KEY_RADIUS).
 */
static const int s_row10_cx[] = {20, 137, 201, 264, 328, 392, 456, 520, 583, 647};
static const int s_row5a_cx[] = {73, 137, 201, 264, 328};   /* digits 1–5 */
static const int s_row5b_cx[] = {392, 456, 520, 583, 647};  /* digits 6–0 */
static const int s_row9_cx[] = {102, 166, 230, 293, 357, 421, 485, 549, 612};
static const int s_row7_cx[] = {201, 264, 328, 392, 456, 520, 584}; /* z–m row */

typedef enum {
    WIFI_KB_LOWER = 0,
    WIFI_KB_UPPER,
    WIFI_KB_NUMBER,
    WIFI_KB_SYMBOL,
    WIFI_KB_MODE_COUNT,
} wifi_kb_mode_t;

/*
 * Per-screen input state: text buffer, LVGL label, four keyboard layers + mode button.
 * SSID and password each have their own ctx so keyboard mode is remembered per screen.
 */
typedef struct {
    char *buf;
    size_t buf_len;
    lv_obj_t *label;
    lv_obj_t *layers[WIFI_KB_MODE_COUNT];
    lv_obj_t *mode_btn;
    wifi_kb_mode_t mode;
} wifi_input_ctx_t;

static lv_obj_t *s_scr_ssid;
static lv_obj_t *s_scr_pw;
static lv_obj_t *lbl_ssid;
static lv_obj_t *lbl_pw;
static char s_ssid_buf[APP_WIFI_SSID_MAX];
static char s_pw_buf[APP_WIFI_PASSWORD_MAX];
static wifi_input_ctx_t s_ssid_ctx;
static wifi_input_ctx_t s_pw_ctx;

/*
 * LVGL event callbacks need stable pointers for one-character strings.
 * Each key stores a pointer into this pool via lv_obj_set_user_data().
 */
static char s_key_chars[96][2];
static int s_key_char_idx;

static void wifi_init_key_chars(void)
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

static const char *wifi_key_char_ptr(char c)
{
    for (int i = 0; i < s_key_char_idx; i++) {
        if (s_key_chars[i][0] == c) {
            return s_key_chars[i];
        }
    }
    return "?";
}

/* Mode key label = name of the layout we switch to on next press. */
static const char *wifi_mode_btn_label(wifi_kb_mode_t mode)
{
    switch (mode) {
    case WIFI_KB_LOWER:
        return "ABC";
    case WIFI_KB_UPPER:
        return "123";
    case WIFI_KB_NUMBER:
        return "#+=";
    case WIFI_KB_SYMBOL:
        return "abc";
    default:
        return "?";
    }
}

/* Show one keyboard layer; hide the other three; refresh mode key caption. */
static void wifi_kb_show_layer(wifi_input_ctx_t *ctx)
{
    for (int i = 0; i < WIFI_KB_MODE_COUNT; i++) {
        if (ctx->layers[i] == NULL) {
            continue;
        }
        if (i == (int)ctx->mode) {
            lv_obj_remove_flag(ctx->layers[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(ctx->layers[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    if (ctx->mode_btn != NULL) {
        lv_obj_t *lab = lv_obj_get_child(ctx->mode_btn, 0);
        if (lab != NULL) {
            lv_label_set_text(lab, wifi_mode_btn_label(ctx->mode));
        }
    }
}

/*
 * Create one circular blue key (theme menu_petal ≈ wireframe #0F5BF9).
 * cx/cy = circle center. ch_ptr = stable char for wifi_key_cb (via btn user_data).
 * ctx passed as event user_data on CLICKED.
 */
static lv_obj_t *wifi_create_key_btn(lv_obj_t *parent, const char *txt, int cx, int cy,
                                     lv_event_cb_t cb, wifi_input_ctx_t *ctx, const char *ch_ptr)
{
    const ui_theme_t *t = ui_theme_get();
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, WIFI_KEY_DIAM, WIFI_KEY_DIAM);
    lv_obj_set_pos(btn, cx - WIFI_KEY_RADIUS, cy - WIFI_KEY_RADIUS);
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
        lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, ctx);
        lv_obj_set_user_data(btn, (void *)ch_ptr);
    }
    return btn;
}

/* Append one character to the active field buffer and refresh the label. */
static void wifi_key_cb(lv_event_t *e)
{
    wifi_input_ctx_t *ctx = (wifi_input_ctx_t *)lv_event_get_user_data(e);
    lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);
    const char *ch = (const char *)lv_obj_get_user_data(btn);
    if (ctx == NULL || ch == NULL || ch[0] == '\0') {
        return;
    }
    size_t len = strlen(ctx->buf);
    if (len + 1 >= ctx->buf_len) {
        return;
    }
    ctx->buf[len] = ch[0];
    ctx->buf[len + 1] = '\0';
    lv_label_set_text(ctx->label, ctx->buf);
    ui_nav_reset_idle_timer();
}

/* Advance keyboard mode: lower → upper → number → symbol → lower. */
static void wifi_mode_cb(lv_event_t *e)
{
    wifi_input_ctx_t *ctx = (wifi_input_ctx_t *)lv_event_get_user_data(e);
    if (ctx == NULL) {
        return;
    }
    ctx->mode = (wifi_kb_mode_t)((ctx->mode + 1) % WIFI_KB_MODE_COUNT);
    wifi_kb_show_layer(ctx);
    ui_nav_reset_idle_timer();
}

/* Place a horizontal run of single-character keys at fixed cy. */
static void wifi_place_chars(lv_obj_t *layer, wifi_input_ctx_t *ctx, const char *keys,
                             const int *cx, int count, int cy)
{
    for (int i = 0; i < count && keys[i] != '\0'; i++) {
        char label[2] = {keys[i], '\0'};
        wifi_create_key_btn(layer, label, cx[i], cy, wifi_key_cb, ctx, wifi_key_char_ptr(keys[i]));
    }
}

/* Transparent full-screen container; one per keyboard layout. */
static lv_obj_t *wifi_create_kb_layer(lv_obj_t *scr)
{
    lv_obj_t *layer = lv_obj_create(scr);
    lv_obj_set_size(layer, UI_DISP, UI_DISP);
    lv_obj_set_pos(layer, 0, 0);
    lv_obj_set_style_bg_opa(layer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(layer, 0, 0);
    lv_obj_remove_flag(layer, LV_OBJ_FLAG_SCROLLABLE);
    return layer;
}

/*
 * Number layout — two rows of five digits + space on row 3.
 * Does not use column x=73 (reserved for mode key on parent screen).
 */
static void wifi_build_number_layer(lv_obj_t *layer, wifi_input_ctx_t *ctx)
{
    wifi_place_chars(layer, ctx, "12345", s_row5a_cx, 5, WIFI_ROW1_CY);
    wifi_place_chars(layer, ctx, "67890", s_row5b_cx, 5, WIFI_ROW2_CY);
    wifi_create_key_btn(layer, "spc", WIFI_SPACE_KEY_CX, WIFI_SPACE_KEY_CY, wifi_key_cb, ctx,
                        wifi_key_char_ptr(' '));
}

/*
 * Symbol layout — punctuation only (no letters or digits).
 * Row 1–2: ten keys each on s_row10_cx. Row 3: nine on s_row9_cx.
 * Column x=647 holds extra symbols; x=612 is space ("spc").
 */
static void wifi_build_symbol_layer(lv_obj_t *layer, wifi_input_ctx_t *ctx)
{
    static const char row1[] = "-_.@#$%{}&";
    static const char row2[] = "=<>[]()+/";
    static const char row3[] = ":;'\"|\\^";

    wifi_place_chars(layer, ctx, row1, s_row10_cx, 10, WIFI_ROW1_CY);
    wifi_place_chars(layer, ctx, row2, s_row10_cx, 10, WIFI_ROW2_CY);
    wifi_place_chars(layer, ctx, row3, s_row9_cx, 9, WIFI_ROW3_CY);
    wifi_create_key_btn(layer, "~", 647, WIFI_ROW1_CY, wifi_key_cb, ctx, wifi_key_char_ptr('~'));
    wifi_create_key_btn(layer, ",", 647, WIFI_ROW2_CY, wifi_key_cb, ctx, wifi_key_char_ptr(','));
    wifi_create_key_btn(layer, "`", 647, WIFI_ROW3_CY, wifi_key_cb, ctx, wifi_key_char_ptr('`'));
    wifi_create_key_btn(layer, "spc", WIFI_SPACE_KEY_CX, WIFI_SPACE_KEY_CY, wifi_key_cb, ctx,
                        wifi_key_char_ptr(' '));
}

/* Shared QWERTY geometry for lower and upper layers (different character strings). */
static void wifi_build_letter_layer(lv_obj_t *layer, wifi_input_ctx_t *ctx, const char *r1,
                                    const char *r2, const char *r3)
{
    wifi_place_chars(layer, ctx, r1, s_row10_cx, 10, WIFI_ROW1_CY);
    wifi_place_chars(layer, ctx, r2, s_row9_cx, 9, WIFI_ROW2_CY);
    wifi_place_chars(layer, ctx, r3, s_row7_cx, 7, WIFI_ROW3_CY);
}

/*
 * Build all four keyboard layers, mode key, and show the default (lowercase).
 * Mode button is a child of scr (not a layer) so it stays visible and on top.
 */
static void wifi_add_keypad(lv_obj_t *scr, wifi_input_ctx_t *ctx)
{
    ctx->mode = WIFI_KB_LOWER;

    ctx->layers[WIFI_KB_LOWER] = wifi_create_kb_layer(scr);
    wifi_build_letter_layer(ctx->layers[WIFI_KB_LOWER], ctx, "qwertyuiop", "asdfghjkl", "zxcvbnm");

    ctx->layers[WIFI_KB_UPPER] = wifi_create_kb_layer(scr);
    wifi_build_letter_layer(ctx->layers[WIFI_KB_UPPER], ctx, "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM");

    ctx->layers[WIFI_KB_NUMBER] = wifi_create_kb_layer(scr);
    wifi_build_number_layer(ctx->layers[WIFI_KB_NUMBER], ctx);

    ctx->layers[WIFI_KB_SYMBOL] = wifi_create_kb_layer(scr);
    wifi_build_symbol_layer(ctx->layers[WIFI_KB_SYMBOL], ctx);

    ctx->mode_btn =
        wifi_create_key_btn(scr, wifi_mode_btn_label(ctx->mode), WIFI_MODE_KEY_CX, WIFI_MODE_KEY_CY,
                            wifi_mode_cb, ctx, NULL);
    lv_obj_t *mode_lab = lv_obj_get_child(ctx->mode_btn, 0);
    if (mode_lab != NULL) {
        lv_obj_set_style_text_font(mode_lab, &lv_font_montserrat_20, 0);
    }

    wifi_kb_show_layer(ctx);
    lv_obj_move_foreground(ctx->mode_btn);
}

/* Purple rounded input bar; typed text shown in lbl (left-aligned). */
static lv_obj_t *wifi_create_ssid_field(lv_obj_t *parent, lv_obj_t **label_out)
{
    const ui_theme_t *t = ui_theme_get();
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_set_size(box, WIFI_FIELD_W, WIFI_FIELD_H);
    lv_obj_set_pos(box, WIFI_FIELD_X, WIFI_FIELD_Y);
    lv_obj_set_style_bg_color(box, t->ring, 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(box, 0, 0);
    lv_obj_set_style_radius(box, WIFI_FIELD_RADIUS, 0);
    lv_obj_set_style_pad_left(box, 24, 0);
    lv_obj_remove_flag(box, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(box);
    lv_label_set_text(lbl, "");
    lv_obj_set_style_text_color(lbl, t->white, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_26, 0);
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 0, 0);
    *label_out = lbl;
    return box;
}

/* Layout guides — coordinates relative to screen content area (inside 14 px ring). */
static void wifi_add_layout_guides(lv_obj_t *scr)
{
    int32_t cw;
    int32_t ch;
    ui_common_get_content_size(scr, &cw, &ch);

    ui_common_add_vertical_line(scr, cw / 2);
    ui_common_add_vertical_line(scr, cw - 20);
    ui_common_add_vertical_line(scr, 20);

    ui_common_add_horizontal_line(scr, ch / 2);
    ui_common_add_horizontal_line(scr, ch - 20);
    ui_common_add_horizontal_line(scr, 20);
}

/* Black circular screen + 14 px ring (thicker than ui_common_create_screen's 6 px). */
static lv_obj_t *wifi_create_screen(void)
{
    const ui_theme_t *t = ui_theme_get();
    lv_obj_t *scr = ui_common_create_screen();
    lv_obj_set_style_border_width(scr, 14, 0);
    lv_obj_set_style_border_color(scr, t->ring, 0);
    ui_common_line_points_reset();
    wifi_add_layout_guides(scr);
    return scr;
}

/* Orange corner on SSID screen: delete last character (not navigate back). */
static void ssid_back_cb(lv_event_t *e)
{
    (void)e;
    size_t len = strlen(s_ssid_buf);
    if (len > 0) {
        s_ssid_buf[len - 1] = '\0';
        lv_label_set_text(lbl_ssid, s_ssid_buf);
    }
    ui_nav_reset_idle_timer();
}

/* Green corner: require non-empty SSID, save to app_config, advance boot flow. */
static void ssid_next_cb(lv_event_t *e)
{
    (void)e;
    if (s_ssid_buf[0] == '\0') {
        return;
    }
    app_config_t *cfg = app_config_get();
    snprintf(cfg->wifi_ssid, sizeof(cfg->wifi_ssid), "%s", s_ssid_buf);
    if (app_config_wifi_password_unset()) {
        ui_nav_go(UI_SCREEN_STARTUP_PASSWORD);
    } else {
        ui_nav_go(UI_SCREEN_LOADING);
    }
}

/* Green corner: save password (may be empty for open network), go to loading. */
static void pw_next_cb(lv_event_t *e)
{
    (void)e;
    app_config_t *cfg = app_config_get();
    snprintf(cfg->wifi_password, sizeof(cfg->wifi_password), "%s", s_pw_buf);
    cfg->wifi_password_set = true;
    ui_nav_go(UI_SCREEN_LOADING);
}

/* Orange corner on password screen: return to SSID entry (does not delete chars). */
static void pw_back_cb(lv_event_t *e)
{
    (void)e;
    ui_nav_go(UI_SCREEN_STARTUP_SSID);
}

static void build_ssid(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    wifi_init_key_chars();

    s_scr_ssid = wifi_create_screen();
    screens[UI_SCREEN_STARTUP_SSID] = s_scr_ssid;
    s_ssid_buf[0] = '\0';

    s_ssid_ctx.buf = s_ssid_buf;
    s_ssid_ctx.buf_len = sizeof(s_ssid_buf);
    s_ssid_ctx.label = NULL;
    s_ssid_ctx.mode_btn = NULL;
    memset(s_ssid_ctx.layers, 0, sizeof(s_ssid_ctx.layers));

    lv_obj_t *title = ui_common_create_title(s_scr_ssid, "Wi-Fi Setup");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, WIFI_TITLE_Y_OFFSET);

    wifi_create_ssid_field(s_scr_ssid, &lbl_ssid);
    s_ssid_ctx.label = lbl_ssid;
    wifi_add_keypad(s_scr_ssid, &s_ssid_ctx);

    lv_obj_t *back = ui_common_create_side_btn(s_scr_ssid, true, WIFI_BTN_BACK_X, WIFI_BTN_Y, NULL);
    lv_obj_add_event_cb(back, ssid_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *next = ui_common_create_side_btn(s_scr_ssid, false, WIFI_BTN_NEXT_X, WIFI_BTN_Y, NULL);
    lv_obj_add_event_cb(next, ssid_next_cb, LV_EVENT_CLICKED, NULL);
}

static void build_password(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    wifi_init_key_chars();

    s_scr_pw = wifi_create_screen();
    screens[UI_SCREEN_STARTUP_PASSWORD] = s_scr_pw;
    s_pw_buf[0] = '\0';

    s_pw_ctx.buf = s_pw_buf;
    s_pw_ctx.buf_len = sizeof(s_pw_buf);
    s_pw_ctx.label = NULL;
    s_pw_ctx.mode_btn = NULL;
    memset(s_pw_ctx.layers, 0, sizeof(s_pw_ctx.layers));

    lv_obj_t *title = ui_common_create_title(s_scr_pw, "Wi-Fi Password");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, WIFI_TITLE_Y_OFFSET);

    wifi_create_ssid_field(s_scr_pw, &lbl_pw);
    s_pw_ctx.label = lbl_pw;
    lv_obj_set_style_text_font(lbl_pw, &lv_font_montserrat_20, 0);
    wifi_add_keypad(s_scr_pw, &s_pw_ctx);

    lv_obj_t *back = ui_common_create_side_btn(s_scr_pw, true, WIFI_BTN_BACK_X, WIFI_BTN_Y, NULL);
    lv_obj_add_event_cb(back, pw_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *next = ui_common_create_side_btn(s_scr_pw, false, WIFI_BTN_NEXT_X, WIFI_BTN_Y, NULL);
    lv_obj_add_event_cb(next, pw_next_cb, LV_EVENT_CLICKED, NULL);
}

extern "C" void ui_screen_startup_wifi_wizard_build(lv_obj_t *screens[UI_SCREEN_COUNT])
{
    build_ssid(screens);
    build_password(screens);
}

extern "C" void ui_screen_startup_wifi_wizard_ssid_get_text(char *out, size_t len)
{
    snprintf(out, len, "%s", s_ssid_buf);
}

extern "C" void ui_screen_startup_wifi_wizard_password_get_text(char *out, size_t len)
{
    snprintf(out, len, "%s", s_pw_buf);
}
