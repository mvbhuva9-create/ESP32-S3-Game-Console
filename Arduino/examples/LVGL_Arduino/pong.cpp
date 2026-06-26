/*
 * pong.cpp
 * Classic Pong – Player (left) vs CPU (right)
 * Touch: drag left half of screen to move player paddle
 */

#include <Arduino.h>
#include <lvgl.h>
#include "pong.h"
#include "menu.h"

// -------- Dimensions --------
#define PW 320
#define PH 240
#define PAD_W  10
#define PAD_H  52
#define BALL_S 10
#define MAX_SCORE 7

// -------- Game state --------
static float bx, by, vx, vy;
static float p1y, p2y;   // top-Y of each paddle
static int   score1, score2;
static bool  paused;

// touch
static lv_coord_t  touch_y     = PH / 2;
static bool        touching    = false;

// -------- LVGL objects --------
static lv_obj_t   *canvas       = nullptr;
static lv_color_t  cbuf[PW * PH];
static lv_obj_t   *lbl_s1       = nullptr;
static lv_obj_t   *lbl_s2       = nullptr;
static lv_timer_t *game_timer   = nullptr;

// -------- Reset ball --------
static void reset_ball(bool to_right) {
    bx = PW / 2.0f;
    by = PH / 2.0f;
    float spd = 3.2f;
    vx = to_right ? spd : -spd;
    float angle = ((random(0, 60) - 30) * M_PI) / 180.0f;
    vy = spd * sinf(angle);
}

// -------- Render --------
static void render() {
    lv_draw_rect_dsc_t bg;
    lv_draw_rect_dsc_init(&bg);
    bg.bg_color = lv_color_hex(0x050D1A);
    bg.bg_opa   = LV_OPA_COVER;
    lv_canvas_draw_rect(canvas, 0, 0, PW, PH, &bg);

    // Centre dashes
    lv_draw_rect_dsc_t dash;
    lv_draw_rect_dsc_init(&dash);
    dash.bg_color = lv_color_hex(0x334466);
    dash.bg_opa   = LV_OPA_COVER;
    for (int y = 0; y < PH; y += 18)
        lv_canvas_draw_rect(canvas, PW / 2 - 1, y, 3, 10, &dash);

    // Paddles
    lv_draw_rect_dsc_t pad;
    lv_draw_rect_dsc_init(&pad);
    pad.bg_opa   = LV_OPA_COVER;
    pad.radius   = 4;

    pad.bg_color = lv_color_hex(0x4ECDC4);
    lv_canvas_draw_rect(canvas, 8, (int)p1y, PAD_W, PAD_H, &pad);

    pad.bg_color = lv_color_hex(0xFF6B6B);
    lv_canvas_draw_rect(canvas, PW - 8 - PAD_W, (int)p2y, PAD_W, PAD_H, &pad);

    // Ball
    lv_draw_rect_dsc_t bd;
    lv_draw_rect_dsc_init(&bd);
    bd.bg_color = lv_color_white();
    bd.bg_opa   = LV_OPA_COVER;
    bd.radius   = BALL_S / 2;
    lv_canvas_draw_rect(canvas, (int)bx - BALL_S / 2, (int)by - BALL_S / 2,
                        BALL_S, BALL_S, &bd);

    // Scores
    char s[8];
    snprintf(s, sizeof(s), "%d", score1);
    lv_label_set_text(lbl_s1, s);
    snprintf(s, sizeof(s), "%d", score2);
    lv_label_set_text(lbl_s2, s);
}

// -------- Game step --------
static void game_step(lv_timer_t *) {
    if (paused) return;

    // Player paddle follows touch
    if (touching) {
        p1y = touch_y - PAD_H / 2.0f;
    }
    p1y = constrain(p1y, 0, PH - PAD_H);

    // CPU paddle (simple AI – tracks ball)
    float cpu_center = p2y + PAD_H / 2.0f;
    float diff = by - cpu_center;
    float cpu_spd = 2.6f;
    if (fabsf(diff) > 2) p2y += (diff > 0 ? 1 : -1) * cpu_spd;
    p2y = constrain(p2y, 0, PH - PAD_H);

    // Move ball
    bx += vx;
    by += vy;

    // Top/bottom bounce
    if (by <= BALL_S / 2)         { by = BALL_S / 2;      vy = fabsf(vy); }
    if (by >= PH - BALL_S / 2)    { by = PH - BALL_S / 2; vy = -fabsf(vy); }

    // Player paddle collision
    if (bx - BALL_S / 2 <= 8 + PAD_W &&
        by >= p1y && by <= p1y + PAD_H && vx < 0) {
        float rel = (by - (p1y + PAD_H / 2.0f)) / (PAD_H / 2.0f);
        float spd = sqrtf(vx * vx + vy * vy) * 1.05f;
        if (spd > 7) spd = 7;
        float angle = rel * 60.0f * M_PI / 180.0f;
        vx =  spd * cosf(angle);
        vy =  spd * sinf(angle);
        bx = 8 + PAD_W + BALL_S / 2;
    }

    // CPU paddle collision
    if (bx + BALL_S / 2 >= PW - 8 - PAD_W &&
        by >= p2y && by <= p2y + PAD_H && vx > 0) {
        float rel = (by - (p2y + PAD_H / 2.0f)) / (PAD_H / 2.0f);
        float spd = sqrtf(vx * vx + vy * vy) * 1.05f;
        if (spd > 7) spd = 7;
        float angle = rel * 60.0f * M_PI / 180.0f;
        vx = -spd * cosf(angle);
        vy =  spd * sinf(angle);
        bx = PW - 8 - PAD_W - BALL_S / 2;
    }

    // Score
    bool scored = false;
    if (bx < 0)   { score2++; reset_ball(false); scored = true; }
    if (bx > PW)  { score1++; reset_ball(true);  scored = true; }

    render();

    // Win check
    if (scored && (score1 >= MAX_SCORE || score2 >= MAX_SCORE)) {
        paused = true;
        lv_obj_t *scr = lv_scr_act();
        lv_obj_t *over = lv_obj_create(scr);
        lv_obj_set_size(over, 220, 110);
        lv_obj_center(over);
        lv_obj_set_style_bg_color(over, lv_color_hex(0x0A1A30), 0);
        lv_obj_set_style_bg_opa(over, LV_OPA_95, 0);
        lv_obj_set_style_border_color(over, lv_color_hex(0x4ECDC4), 0);
        lv_obj_set_style_border_width(over, 2, 0);
        lv_obj_set_style_radius(over, 10, 0);

        const char *winner = (score1 >= MAX_SCORE) ? "You Win!" : "CPU Wins!";
        lv_obj_t *wl = lv_label_create(over);
        char buf[80];
        snprintf(buf, sizeof(buf), "%s\n%d – %d\n\nTap for Menu", winner, score1, score2);
        lv_label_set_text(wl, buf);
        lv_obj_set_style_text_align(wl, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(wl, lv_color_white(), 0);
        lv_obj_set_style_text_font(wl, &lv_font_montserrat_16, 0);
        lv_obj_center(wl);

        lv_obj_add_event_cb(over, [](lv_event_t *e) {
            if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
                if (game_timer) { lv_timer_del(game_timer); game_timer = nullptr; }
                menu_show();
            }
        }, LV_EVENT_CLICKED, nullptr);
    }
}

// -------- Touch --------
static void screen_touch_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev = lv_indev_get_act();
    if (!indev) return;
    lv_point_t pt;
    lv_indev_get_point(indev, &pt);

    if (code == LV_EVENT_PRESSING && pt.x < PW / 2) {
        touching = true;
        touch_y  = pt.y;
    } else if (code == LV_EVENT_RELEASED) {
        touching = false;
    }
}

// -------- Init --------
void pong_init() {
    score1 = score2 = 0;
    p1y = p2y = (PH - PAD_H) / 2.0f;
    paused = false;
    touching = false;
    reset_ball(true);

    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x050D1A), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // Canvas covers full screen (scores drawn on canvas)
    canvas = lv_canvas_create(scr);
    lv_canvas_set_buffer(canvas, cbuf, PW, PH, LV_IMG_CF_TRUE_COLOR);
    lv_obj_align(canvas, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_add_event_cb(canvas, screen_touch_cb, LV_EVENT_PRESSING,  nullptr);
    lv_obj_add_event_cb(canvas, screen_touch_cb, LV_EVENT_RELEASED,  nullptr);

    // Floating score labels
    lbl_s1 = lv_label_create(scr);
    lv_label_set_text(lbl_s1, "0");
    lv_obj_set_style_text_font(lbl_s1, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_s1, lv_color_hex(0x4ECDC4), 0);
    lv_obj_align(lbl_s1, LV_ALIGN_TOP_LEFT, PW / 4, 6);

    lbl_s2 = lv_label_create(scr);
    lv_label_set_text(lbl_s2, "0");
    lv_obj_set_style_text_font(lbl_s2, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_s2, lv_color_hex(0xFF6B6B), 0);
    lv_obj_align(lbl_s2, LV_ALIGN_TOP_RIGHT, -(PW / 4), 6);

    // Back button (top-center)
    lv_obj_t *back = lv_btn_create(scr);
    lv_obj_set_size(back, 40, 30);
    lv_obj_align(back, LV_ALIGN_TOP_MID, 0, 4);
    lv_obj_set_style_bg_color(back, lv_color_hex(0x0A1A30), 0);
    lv_obj_set_style_bg_opa(back, LV_OPA_80, 0);
    lv_obj_set_style_border_width(back, 0, 0);
    lv_obj_set_style_radius(back, 6, 0);
    lv_obj_add_event_cb(back, [](lv_event_t *e) {
        if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
            if (game_timer) { lv_timer_del(game_timer); game_timer = nullptr; }
            menu_show();
        }
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *blbl = lv_label_create(back);
    lv_label_set_text(blbl, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(blbl, lv_color_white(), 0);
    lv_obj_center(blbl);

    render();
    game_timer = lv_timer_create(game_step, 16, nullptr);  // ~60fps
}
