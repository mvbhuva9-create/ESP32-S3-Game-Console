/*
 * menu.cpp  –  Main game selector
 * Three big touch buttons: Snake | Pong | Flappy Bird
 */

#include <lvgl.h>
#include "menu.h"
#include "snake.h"
#include "pong.h"
#include "flappy.h"

static lv_obj_t *menu_screen = nullptr;

// -------- Button callbacks --------
static void btn_snake_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_obj_clean(lv_scr_act());
        snake_init();
    }
}

static void btn_pong_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_obj_clean(lv_scr_act());
        pong_init();
    }
}

static void btn_flappy_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        lv_obj_clean(lv_scr_act());
        flappy_init();
    }
}

// -------- Style helpers --------
static lv_style_t style_btn;
static lv_style_t style_btn_pressed;
static bool styles_inited = false;

static void init_styles() {
    if (styles_inited) return;
    styles_inited = true;

    lv_style_init(&style_btn);
    lv_style_set_radius(&style_btn, 12);
    lv_style_set_bg_color(&style_btn, lv_color_hex(0x1E3A5F));
    lv_style_set_bg_grad_color(&style_btn, lv_color_hex(0x0A1A30));
    lv_style_set_bg_grad_dir(&style_btn, LV_GRAD_DIR_VER);
    lv_style_set_bg_opa(&style_btn, LV_OPA_COVER);
    lv_style_set_border_color(&style_btn, lv_color_hex(0x4ECDC4));
    lv_style_set_border_width(&style_btn, 2);
    lv_style_set_shadow_color(&style_btn, lv_color_hex(0x4ECDC4));
    lv_style_set_shadow_width(&style_btn, 8);
    lv_style_set_shadow_ofs_y(&style_btn, 3);
    lv_style_set_text_color(&style_btn, lv_color_white());
    lv_style_set_text_font(&style_btn, &lv_font_montserrat_16);

    lv_style_init(&style_btn_pressed);
    lv_style_set_bg_color(&style_btn_pressed, lv_color_hex(0x4ECDC4));
    lv_style_set_bg_grad_color(&style_btn_pressed, lv_color_hex(0x2FA89E));
    lv_style_set_text_color(&style_btn_pressed, lv_color_hex(0x0A1A30));
}

static lv_obj_t *make_game_btn(lv_obj_t *parent, const char *label,
                                lv_event_cb_t cb, int y_pos) {
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_size(btn, 260, 52);
    lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, y_pos);
    lv_obj_add_style(btn, &style_btn, 0);
    lv_obj_add_style(btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, label);
    lv_obj_center(lbl);
    return btn;
}

// -------- Public API --------
void menu_init() {
    init_styles();
    menu_show();
}

void menu_show() {
    lv_obj_t *scr = lv_scr_act();
    lv_obj_clean(scr);

    // Background
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x050D1A), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // Title
    lv_obj_t *title = lv_label_create(scr);
    lv_label_set_text(title, LV_SYMBOL_PLAY " GAME CONSOLE");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x4ECDC4), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 14);

    // Divider
    lv_obj_t *line = lv_obj_create(scr);
    lv_obj_set_size(line, 280, 2);
    lv_obj_align(line, LV_ALIGN_TOP_MID, 0, 46);
    lv_obj_set_style_bg_color(line, lv_color_hex(0x4ECDC4), 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_40, 0);
    lv_obj_set_style_border_width(line, 0, 0);
    lv_obj_set_style_pad_all(line, 0, 0);

    // Buttons
    make_game_btn(scr, LV_SYMBOL_RIGHT "  Snake",       btn_snake_cb,  58);
    make_game_btn(scr, LV_SYMBOL_RIGHT "  Pong",        btn_pong_cb,  120);
    make_game_btn(scr, LV_SYMBOL_RIGHT "  Flappy Bird", btn_flappy_cb, 182);

    // Footer
    lv_obj_t *footer = lv_label_create(scr);
    lv_label_set_text(footer, "Tap a game to play");
    lv_obj_set_style_text_font(footer, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(footer, lv_color_hex(0x556677), 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -6);
}
