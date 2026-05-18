#include "ui_screens.h"
#include "bsp/display.h"
#include <stdio.h>
#include <string.h>

/* Vibrant theme */
static const lv_color_t COL_BG        = LV_COLOR_MAKE(0, 0, 0);
static const lv_color_t COL_RING      = LV_COLOR_MAKE(155, 48, 255);
static const lv_color_t COL_PURPLE    = LV_COLOR_MAKE(90, 24, 154);
static const lv_color_t COL_KEYPAD    = LV_COLOR_MAKE(29, 78, 216);
static const lv_color_t COL_ORANGE    = LV_COLOR_MAKE(232, 122, 46);
static const lv_color_t COL_GREEN     = LV_COLOR_MAKE(60, 184, 92);
static const lv_color_t COL_WHITE     = LV_COLOR_MAKE(255, 255, 255);

static const int DISP = BSP_LCD_H_RES;

static lv_obj_t *scr_password;
static lv_obj_t *scr_maths;
static lv_obj_t *lbl_pin;
static lv_obj_t *lbl_answer;

static char pin_buf[5];
static char answer_buf[8];

static void style_circle_panel(lv_obj_t *obj)
{
    lv_obj_set_size(obj, DISP, DISP);
    lv_obj_set_style_bg_color(obj, COL_BG, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(obj, COL_RING, 0);
    lv_obj_set_style_border_width(obj, 6, 0);
    lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_clip_corner(obj, true, 0);
}

static lv_obj_t *create_keypad_btn(lv_obj_t *parent, const char *txt, int x, int y, int size)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, size, size);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(btn, COL_KEYPAD, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_border_width(btn, 0, 0);

    lv_obj_t *lab = lv_label_create(btn);
    lv_label_set_text(lab, txt);
    lv_obj_set_style_text_color(lab, COL_WHITE, 0);
    lv_obj_set_style_text_font(lab, &lv_font_montserrat_26, 0);
    lv_obj_center(lab);
    return btn;
}

static void keypad_digit_cb(lv_event_t *e)
{
    const char *digit = (const char *)lv_event_get_user_data(e);
    lv_obj_t *scr = lv_obj_get_screen((lv_obj_t *)lv_event_get_target(e));
    bool is_password = (scr == scr_password);

    if (is_password) {
        size_t len = strlen(pin_buf);
        if (len >= 4) {
            return;
        }
        pin_buf[len] = digit[0];
        pin_buf[len + 1] = '\0';
        char dots[8];
        memset(dots, 0, sizeof(dots));
        for (size_t i = 0; i < len + 1; i++) {
            dots[i * 2] = '*';
            if (i < len) {
                dots[i * 2 + 1] = ' ';
            }
        }
        lv_label_set_text(lbl_pin, dots);
    } else {
        size_t len = strlen(answer_buf);
        if (len >= 3) {
            return;
        }
        answer_buf[len] = digit[0];
        answer_buf[len + 1] = '\0';
        lv_label_set_text(lbl_answer, answer_buf);
    }
}

static void add_keypad(lv_obj_t *parent, bool password_screen)
{
    const int btn = 72;
    const int gap = 14;
    const int cols = 3;
    const int grid_w = cols * btn + (cols - 1) * gap;
    const int start_x = (DISP - grid_w) / 2;
    const int start_y = password_screen ? 268 : 300;

    static const char *keys[10] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"};
    for (int i = 0; i < 9; i++) {
        int row = i / 3;
        int col = i % 3;
        lv_obj_t *b = create_keypad_btn(parent, keys[i],
                                        start_x + col * (btn + gap),
                                        start_y + row * (btn + gap), btn);
        lv_obj_add_event_cb(b, keypad_digit_cb, LV_EVENT_CLICKED, (void *)keys[i]);
    }
    lv_obj_t *b0 = create_keypad_btn(parent, "0",
                                     start_x + btn + gap,
                                     start_y + 3 * (btn + gap), btn);
    lv_obj_add_event_cb(b0, keypad_digit_cb, LV_EVENT_CLICKED, (void *)"0");
}

static lv_obj_t *create_side_btn(lv_obj_t *parent, bool is_back, int x, int y)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 64, 110);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_radius(btn, 32, 0);
    lv_obj_set_style_bg_color(btn, is_back ? COL_ORANGE : COL_GREEN, 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_set_style_border_width(btn, 0, 0);

    lv_obj_t *lab = lv_label_create(btn);
    lv_label_set_text(lab, is_back ? LV_SYMBOL_LEFT : LV_SYMBOL_OK);
    lv_obj_set_style_text_color(lab, COL_WHITE, 0);
    lv_obj_set_style_text_font(lab, &lv_font_montserrat_26, 0);
    lv_obj_center(lab);
    return btn;
}

static void show_maths_screen(void)
{
    pin_buf[0] = '\0';
    answer_buf[0] = '\0';
    lv_label_set_text(lbl_answer, "");
    lv_screen_load(scr_maths);
}

static void password_back_cb(lv_event_t *e)
{
    (void)e;
    size_t len = strlen(pin_buf);
    if (len > 0) {
        pin_buf[len - 1] = '\0';
        char dots[8] = "";
        for (size_t i = 0; i < len - 1; i++) {
            dots[i * 2] = '*';
            dots[i * 2 + 1] = ' ';
        }
        lv_label_set_text(lbl_pin, dots);
    }
}

static void password_ok_cb(lv_event_t *e)
{
    (void)e;
    if (strlen(pin_buf) == 4) {
        show_maths_screen();
    }
}

static void maths_back_cb(lv_event_t *e)
{
    (void)e;
    size_t len = strlen(answer_buf);
    if (len > 0) {
        answer_buf[len - 1] = '\0';
        lv_label_set_text(lbl_answer, answer_buf);
    } else {
        pin_buf[0] = '\0';
        lv_label_set_text(lbl_pin, "");
        lv_screen_load(scr_password);
    }
}

static void maths_ok_cb(lv_event_t *e)
{
    (void)e;
    if (strcmp(answer_buf, "8") == 0) {
        lv_label_set_text(lbl_answer, "8");
    }
}

static lv_obj_t *create_purple_box(lv_obj_t *parent, int w, int h, int x, int y, bool outline_only)
{
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_set_size(box, w, h);
    lv_obj_set_pos(box, x, y);
    lv_obj_set_style_bg_color(box, outline_only ? COL_BG : COL_PURPLE, 0);
    lv_obj_set_style_bg_opa(box, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(box, COL_PURPLE, 0);
    lv_obj_set_style_border_width(box, outline_only ? 3 : 0, 0);
    lv_obj_set_style_radius(box, 12, 0);
    lv_obj_set_style_pad_all(box, 0, 0);
    lv_obj_remove_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    return box;
}

static void build_password_screen(void)
{
    scr_password = lv_obj_create(NULL);
    style_circle_panel(scr_password);

    lv_obj_t *pin_bar = create_purple_box(scr_password, 340, 56, (DISP - 340) / 2, 72, false);
    lbl_pin = lv_label_create(pin_bar);
    lv_label_set_text(lbl_pin, "");
    lv_obj_set_style_text_color(lbl_pin, COL_WHITE, 0);
    lv_obj_set_style_text_font(lbl_pin, &lv_font_montserrat_26, 0);
    lv_obj_center(lbl_pin);

    add_keypad(scr_password, true);

    lv_obj_t *back = create_side_btn(scr_password, true, 36, 330);
    lv_obj_add_event_cb(back, password_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ok = create_side_btn(scr_password, false, DISP - 36 - 64, 330);
    lv_obj_add_event_cb(ok, password_ok_cb, LV_EVENT_CLICKED, NULL);
}

static void build_maths_screen(void)
{
    scr_maths = lv_obj_create(NULL);
    style_circle_panel(scr_maths);

    const int box = 64;
    const int gap = 12;
    const int eq_w = box * 3 + gap * 2 + 24 + 28 + 24;
    int x = (DISP - eq_w) / 2;
    const int y = 88;

    lv_obj_t *b5 = create_purple_box(scr_maths, box, box, x, y, false);
    lv_obj_t *l5 = lv_label_create(b5);
    lv_label_set_text(l5, "5");
    lv_obj_set_style_text_color(l5, COL_WHITE, 0);
    lv_obj_set_style_text_font(l5, &lv_font_montserrat_26, 0);
    lv_obj_center(l5);
    x += box + gap;

    lv_obj_t *plus = lv_label_create(scr_maths);
    lv_label_set_text(plus, "+");
    lv_obj_set_style_text_color(plus, COL_WHITE, 0);
    lv_obj_set_style_text_font(plus, &lv_font_montserrat_26, 0);
    lv_obj_set_pos(plus, x, y + 16);
    x += 28 + gap;

    lv_obj_t *b3 = create_purple_box(scr_maths, box, box, x, y, false);
    lv_obj_t *l3 = lv_label_create(b3);
    lv_label_set_text(l3, "3");
    lv_obj_set_style_text_color(l3, COL_WHITE, 0);
    lv_obj_set_style_text_font(l3, &lv_font_montserrat_26, 0);
    lv_obj_center(l3);
    x += box + gap;

    lv_obj_t *eq = lv_label_create(scr_maths);
    lv_label_set_text(eq, "=");
    lv_obj_set_style_text_color(eq, COL_WHITE, 0);
    lv_obj_set_style_text_font(eq, &lv_font_montserrat_26, 0);
    lv_obj_set_pos(eq, x, y + 16);
    x += 28 + gap;

    lv_obj_t *ans_box = create_purple_box(scr_maths, box, box, x, y, true);
    lbl_answer = lv_label_create(ans_box);
    lv_label_set_text(lbl_answer, "");
    lv_obj_set_style_text_color(lbl_answer, COL_WHITE, 0);
    lv_obj_set_style_text_font(lbl_answer, &lv_font_montserrat_26, 0);
    lv_obj_center(lbl_answer);

    add_keypad(scr_maths, false);

    lv_obj_t *back = create_side_btn(scr_maths, true, 36, 330);
    lv_obj_add_event_cb(back, maths_back_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ok = create_side_btn(scr_maths, false, DISP - 36 - 64, 330);
    lv_obj_add_event_cb(ok, maths_ok_cb, LV_EVENT_CLICKED, NULL);
}

void ui_screens_init(void)
{
    pin_buf[0] = '\0';
    answer_buf[0] = '\0';
    build_password_screen();
    build_maths_screen();
    lv_screen_load(scr_password);
}
