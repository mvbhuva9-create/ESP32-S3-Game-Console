/*
 * flappy.cpp
 * Flappy Bird clone – tap anywhere to flap
 * Procedural pipe generation, gravity + velocity physics
 */

#include <Arduino.h>
#include <lvgl.h>
#include "flappy.h"
#include "menu.h"

// -------- Screen --------
#define FW 320
#define FH 240

// -------- Bird --------
#define BIRD_X   60
#define BIRD_R   9
#define GRAVITY  0.38f
#define FLAP_V  -5.8f

// -------- Pipes --------
#define PIPE_W    28
#define GAP_H     70
#define PIPE_SPD  2.2f
#define NUM_PIPES 3
#define PIPE_SPACE 110

// -------- Colors --------
#define C_SKY_T  lv_color_hex(0x0A1A50)
#define C_SKY_B  lv_color_hex(0x050D1A)
#define C_BIRD   lv_color_hex(0xFFD93D)
#define C_PIPE   lv_color_hex(0x2ECC71)
#define C_PIPEDARK lv_color_hex(0x27AE60)
#define C_GROUND lv_color_hex(0xC8A96E)

// -------- State --------
struct Pipe { float x; int gap_y; bool scored; };

static float bird_y, bird_vy;
static Pipe  pipes[NUM_PIPES];
static int   score, best;
static bool  alive, started;

// -------- LVGL --------
static lv_obj_t   *canvas     = nullptr;
static lv_color_t  cbuf[FW * FH];
static lv_obj_t   *lbl_score  = nullptr;
static lv_obj_t   *lbl_best   = nullptr;
static lv_timer_t *game_timer = nullptr;

// -------- Helpers --------
static void spawn_pipe(Pipe &p, float x) {
    p.x      = x;
    p.gap_y  = random(GAP_H / 2 + 10, FH - 30 - GAP_H / 2);
    p.scored = false;
}

static void init_pipes() {
    for (int i = 0; i < NUM_PIPES; i++)
        spawn_pipe(pipes[i], FW + 60 + i * PIPE_SPACE);
}

// -------- Render --------
static void render() {
    // Sky gradient (two rects)
    lv_draw_rect_dsc_t r;
    lv_draw_rect_dsc_init(&r);
    r.bg_opa = LV_OPA_COVER;

    r.bg_color = C_SKY_T;
    lv_canvas_draw_rect(canvas, 0, 0, FW, FH / 2, &r);
    r.bg_color = C_SKY_B;
    lv_canvas_draw_rect(canvas, 0, FH / 2, FW, FH / 2, &r);

    // Pipes
    lv_draw_rect_dsc_t pipe_d;
    lv_draw_rect_dsc_init(&pipe_d);
    pipe_d.bg_opa   = LV_OPA_COVER;
    pipe_d.radius   = 3;

    for (int i = 0; i < NUM_PIPES; i++) {
        int px  = (int)pipes[i].x;
        int gy  = pipes[i].gap_y;

        // Top pipe
        pipe_d.bg_color = C_PIPE;
        lv_canvas_draw_rect(canvas, px, 0, PIPE_W, gy - GAP_H / 2, &pipe_d);
        // Cap top
        pipe_d.bg_color = C_PIPEDARK;
        lv_canvas_draw_rect(canvas, px - 3, gy - GAP_H / 2 - 12, PIPE_W + 6, 12, &pipe_d);

        // Bottom pipe
        pipe_d.bg_color = C_PIPE;
        int bot_start = gy + GAP_H / 2;
        lv_canvas_draw_rect(canvas, px, bot_start, PIPE_W, FH - bot_start, &pipe_d);
        // Cap bottom
        pipe_d.bg_color = C_PIPEDARK;
        lv_canvas_draw_rect(canvas, px - 3, bot_start, PIPE_W + 6, 12, &pipe_d);
    }

    // Ground
    lv_draw_rect_dsc_t gd;
    lv_draw_rect_dsc_init(&gd);
    gd.bg_color = C_GROUND;
    gd.bg_opa   = LV_OPA_COVER;
    lv_canvas_draw_rect(canvas, 0, FH - 10, FW, 10, &gd);

    // Bird (circle)
    lv_draw_rect_dsc_t bd;
    lv_draw_rect_dsc_init(&bd);
    bd.bg_color = C_BIRD;
    bd.bg_opa   = LV_OPA_COVER;
    bd.radius   = BIRD_R;
    int bx = BIRD_X - BIRD_R;
    int by = (int)bird_y - BIRD_R;
    lv_canvas_draw_rect(canvas, bx, by, BIRD_R * 2, BIRD_R * 2, &bd);

    // Eye
    lv_draw_rect_dsc_t eye;
    lv_draw_rect_dsc_init(&eye);
    eye.bg_color = lv_color_hex(0x1A1A2E);
    eye.bg_opa   = LV_OPA_COVER;
    eye.radius   = 3;
    lv_canvas_draw_rect(canvas, BIRD_X + 3, (int)bird_y - 4, 5, 5, &eye);

    // Score
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", score);
    lv_label_set_text(lbl_score, buf);
    snprintf(buf, sizeof(buf), "Best:%d", best);
    lv_label_set_text(lbl_best, buf);
}

// -------- Collision --------
static bool check_collision() {
    // Ground / ceiling
    if (bird_y + BIRD_R >= FH - 10) return true;
    if (bird_y - BIRD_R <= 0)        return true;

    for (int i = 0; i < NUM_PIPES; i++) {
        int px = (int)pipes[i].x;
        int gy = pipes[i].gap_y;
        // Horizontal overlap?
        if (BIRD_X + BIRD_R > px && BIRD_X - BIRD_R < px + PIPE_W) {
            // Vertical: in gap?
            if (bird_y - BIRD_R < gy - GAP_H / 2 ||
                bird_y + BIRD_R > gy + GAP_H / 2) {
                return true;
            }
        }
    }
    return false;
}

// -------- Game step --------
static void game_step(lv_timer_t *) {
    if (!alive) return;
    if (!started) { render(); return; }

    // Physics
    bird_vy += GRAVITY;
    bird_y  += bird_vy;

    // Move pipes
    for (int i = 0; i < NUM_PIPES; i++) {
        pipes[i].x -= PIPE_SPD;
        if (pipes[i].x + PIPE_W < 0)
            spawn_pipe(pipes[i], FW + 10);

        // Score when bird passes pipe center
        if (!pipes[i].scored && BIRD_X > pipes[i].x + PIPE_W / 2) {
            pipes[i].scored = true;
            score++;
        }
    }

    if (check_collision()) {
        alive = false;
        if (score > best) best = score;

        // Game-over overlay
        lv_obj_t *scr = lv_scr_act();
        lv_obj_t *over = lv_obj_create(scr);
        lv_obj_set_size(over, 220, 120);
        lv_obj_center(over);
        lv_obj_set_style_bg_color(over, lv_color_hex(0x0A1A30), 0);
        lv_obj_set_style_bg_opa(over, LV_OPA_95, 0);
        lv_obj_set_style_border_color(over, lv_color_hex(0xFFD93D), 0);
        lv_obj_set_style_border_width(over, 2, 0);
        lv_obj_set_style_radius(over, 10, 0);

        lv_obj_t *gl = lv_label_create(over);
        char buf[80];
        snprintf(buf, sizeof(buf), "Game Over!\nScore: %d  Best: %d\n\nTap for Menu", score, best);
        lv_label_set_text(gl, buf);
        lv_obj_set_style_text_align(gl, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(gl, lv_color_white(), 0);
        lv_obj_set_style_text_font(gl, &lv_font_montserrat_14, 0);
        lv_obj_center(gl);

        lv_obj_add_event_cb(over, [](lv_event_t *e) {
            if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
                if (game_timer) { lv_timer_del(game_timer); game_timer = nullptr; }
                menu_show();
            }
        }, LV_EVENT_CLICKED, nullptr);
        return;
    }

    render();
}

// -------- Touch: flap --------
static void flap_cb(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (!alive) return;
    started   = true;
    bird_vy   = FLAP_V;
}

// -------- Init --------
void flappy_init() {
    bird_y   = FH / 2.0f;
    bird_vy  = 0;
    score    = 0;
    alive    = true;
    started  = false;
    init_pipes();

    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, C_SKY_T, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    canvas = lv_canvas_create(scr);
    lv_canvas_set_buffer(canvas, cbuf, FW, FH, LV_IMG_CF_TRUE_COLOR);
    lv_obj_align(canvas, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_clear_flag(canvas, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(canvas, flap_cb, LV_EVENT_CLICKED, nullptr);

    // Score overlay
    lbl_score = lv_label_create(scr);
    lv_label_set_text(lbl_score, "0");
    lv_obj_set_style_text_font(lbl_score, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(lbl_score, lv_color_white(), 0);
    lv_obj_align(lbl_score, LV_ALIGN_TOP_MID, 0, 8);

    lbl_best = lv_label_create(scr);
    lv_label_set_text(lbl_best, "Best:0");
    lv_obj_set_style_text_font(lbl_best, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(lbl_best, lv_color_hex(0xFFD93D), 0);
    lv_obj_align(lbl_best, LV_ALIGN_TOP_RIGHT, -6, 8);

    // Back button
    lv_obj_t *back = lv_btn_create(scr);
    lv_obj_set_size(back, 40, 30);
    lv_obj_align(back, LV_ALIGN_TOP_LEFT, 4, 4);
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

    // Start hint
    lv_obj_t *hint = lv_label_create(scr);
    lv_label_set_text(hint, "Tap to flap!");
    lv_obj_set_style_text_color(hint, lv_color_hex(0xFFD93D), 0);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
    lv_obj_align(hint, LV_ALIGN_CENTER, 0, 30);

    render();
    game_timer = lv_timer_create(game_step, 16, nullptr);  // ~60fps
}
