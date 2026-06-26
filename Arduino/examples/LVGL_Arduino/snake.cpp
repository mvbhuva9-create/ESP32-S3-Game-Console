/*
 * snake.cpp
 * Classic Snake – LVGL canvas, touch-swipe or on-screen d-pad controls
 * Grid: 26×18 cells at 11px each (fits 320×240 with HUD)
 */

#include <Arduino.h>
#include <lvgl.h>
#include "snake.h"
#include "menu.h"

// -------- Grid config --------
#define CELL      11
#define COLS      26
#define ROWS      18
#define CANVAS_W  (COLS * CELL)   // 286
#define CANVAS_H  (ROWS * CELL)   // 198
#define HUD_H     (240 - CANVAS_H - 6)  // top bar

// -------- Colours (RGB565) --------
#define C_BG      lv_color_hex(0x0A0A0A)
#define C_GRID    lv_color_hex(0x1A1A2E)
#define C_SNAKE_H lv_color_hex(0x4ECDC4)
#define C_SNAKE_B lv_color_hex(0x2FA89E)
#define C_FOOD    lv_color_hex(0xFF6B6B)
#define C_TEXT    lv_color_white()

// -------- Direction --------
enum Dir { UP, DOWN, LEFT, RIGHT };

// -------- Snake state --------
struct Pt { int x, y; };

static Pt    snake[COLS * ROWS];
static int   snake_len;
static Dir   dir, next_dir;
static Pt    food;
static int   score;
static bool  game_over;
static bool  started;

// -------- LVGL objects --------
static lv_obj_t   *canvas    = nullptr;
static lv_color_t  cbuf[CANVAS_W * CANVAS_H];
static lv_obj_t   *lbl_score = nullptr;
static lv_timer_t *game_timer = nullptr;

// touch tracking
static lv_point_t touch_start;
static bool       touch_active = false;

// -------- Helpers --------
static void place_food() {
    bool ok;
    do {
        food.x = random(0, COLS);
        food.y = random(0, ROWS);
        ok = true;
        for (int i = 0; i < snake_len; i++)
            if (snake[i].x == food.x && snake[i].y == food.y) { ok = false; break; }
    } while (!ok);
}

static void draw_cell(int gx, int gy, lv_color_t col) {
    lv_canvas_draw_rect(canvas, gx * CELL, gy * CELL, CELL - 1, CELL - 1,
                        // use a simple draw descriptor
                        [&]() -> lv_draw_rect_dsc_t {
                            lv_draw_rect_dsc_t d;
                            lv_draw_rect_dsc_init(&d);
                            d.bg_color = col;
                            d.bg_opa   = LV_OPA_COVER;
                            d.radius   = (col.full == C_FOOD.full) ? CELL / 2 : 2;
                            return d;
                        }());
}

static void render() {
    // Clear canvas
    lv_draw_rect_dsc_t bg;
    lv_draw_rect_dsc_init(&bg);
    bg.bg_color = C_BG;
    bg.bg_opa   = LV_OPA_COVER;
    lv_canvas_draw_rect(canvas, 0, 0, CANVAS_W, CANVAS_H, &bg);

    // Light grid lines
    lv_draw_line_dsc_t ld;
    lv_draw_line_dsc_init(&ld);
    ld.color = C_GRID;
    ld.width = 1;
    ld.opa   = LV_OPA_50;
    for (int c = 0; c <= COLS; c++) {
        lv_point_t a = {(lv_coord_t)(c * CELL), 0};
        lv_point_t b = {(lv_coord_t)(c * CELL), CANVAS_H};
        lv_canvas_draw_line(canvas, &a, &b, 2, &ld);
    }
    for (int r = 0; r <= ROWS; r++) {
        lv_point_t a = {0, (lv_coord_t)(r * CELL)};
        lv_point_t b = {CANVAS_W, (lv_coord_t)(r * CELL)};
        lv_canvas_draw_line(canvas, &a, &b, 2, &ld);
    }

    // Food
    lv_draw_rect_dsc_t fd;
    lv_draw_rect_dsc_init(&fd);
    fd.bg_color = C_FOOD;
    fd.bg_opa   = LV_OPA_COVER;
    fd.radius   = CELL / 2;
    lv_canvas_draw_rect(canvas, food.x * CELL + 1, food.y * CELL + 1,
                        CELL - 2, CELL - 2, &fd);

    // Snake body
    lv_draw_rect_dsc_t sd;
    lv_draw_rect_dsc_init(&sd);
    sd.bg_opa = LV_OPA_COVER;
    sd.radius = 2;
    for (int i = 1; i < snake_len; i++) {
        sd.bg_color = C_SNAKE_B;
        lv_canvas_draw_rect(canvas, snake[i].x * CELL + 1, snake[i].y * CELL + 1,
                            CELL - 2, CELL - 2, &sd);
    }
    // Head (brighter)
    sd.bg_color = C_SNAKE_H;
    sd.radius   = 3;
    lv_canvas_draw_rect(canvas, snake[0].x * CELL + 1, snake[0].y * CELL + 1,
                        CELL - 2, CELL - 2, &sd);

    // Score
    char buf[32];
    snprintf(buf, sizeof(buf), "Score: %d", score);
    lv_label_set_text(lbl_score, buf);
}

// -------- Game step --------
static void game_step(lv_timer_t *) {
    if (game_over || !started) return;

    dir = next_dir;

    Pt head = snake[0];
    switch (dir) {
        case UP:    head.y--; break;
        case DOWN:  head.y++; break;
        case LEFT:  head.x--; break;
        case RIGHT: head.x++; break;
    }

    // Wall collision
    if (head.x < 0 || head.x >= COLS || head.y < 0 || head.y >= ROWS) {
        game_over = true;
    }

    // Self collision
    for (int i = 0; i < snake_len; i++) {
        if (snake[i].x == head.x && snake[i].y == head.y) {
            game_over = true;
            break;
        }
    }

    if (!game_over) {
        bool ate = (head.x == food.x && head.y == food.y);
        // Shift body
        if (!ate) snake_len--; else { score += 10; }
        memmove(&snake[1], &snake[0], snake_len * sizeof(Pt));
        snake[0] = head;
        snake_len++;
        if (ate) place_food();
    }

    render();

    if (game_over) {
        // Show game-over overlay
        lv_obj_t *scr = lv_scr_act();
        lv_obj_t *over = lv_obj_create(scr);
        lv_obj_set_size(over, 200, 100);
        lv_obj_center(over);
        lv_obj_set_style_bg_color(over, lv_color_hex(0x1A1A2E), 0);
        lv_obj_set_style_bg_opa(over, LV_OPA_90, 0);
        lv_obj_set_style_border_color(over, lv_color_hex(0xFF6B6B), 0);
        lv_obj_set_style_border_width(over, 2, 0);
        lv_obj_set_style_radius(over, 10, 0);

        lv_obj_t *go_lbl = lv_label_create(over);
        char buf[64];
        snprintf(buf, sizeof(buf), "Game Over!\nScore: %d\n\nTap to Menu", score);
        lv_label_set_text(go_lbl, buf);
        lv_obj_set_style_text_align(go_lbl, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(go_lbl, lv_color_white(), 0);
        lv_obj_center(go_lbl);

        lv_obj_add_event_cb(over, [](lv_event_t *e) {
            if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
                if (game_timer) { lv_timer_del(game_timer); game_timer = nullptr; }
                menu_show();
            }
        }, LV_EVENT_CLICKED, nullptr);
    }
}

// -------- Touch swipe handler --------
static void canvas_touch_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t *indev = lv_indev_get_act();
    if (!indev) return;

    lv_point_t pt;
    lv_indev_get_point(indev, &pt);

    if (code == LV_EVENT_PRESSED) {
        touch_start  = pt;
        touch_active = true;
        if (!started) started = true;
    } else if (code == LV_EVENT_RELEASED && touch_active) {
        touch_active = false;
        int dx = pt.x - touch_start.x;
        int dy = pt.y - touch_start.y;
        if (abs(dx) > abs(dy)) {
            if (dx > 10 && dir != LEFT)  next_dir = RIGHT;
            if (dx < -10 && dir != RIGHT) next_dir = LEFT;
        } else {
            if (dy > 10 && dir != UP)   next_dir = DOWN;
            if (dy < -10 && dir != DOWN) next_dir = UP;
        }
    }
}

// -------- Init --------
void snake_init() {
    game_over  = false;
    started    = false;
    score      = 0;
    snake_len  = 4;
    dir = next_dir = RIGHT;
    touch_active   = false;

    int mx = COLS / 2, my = ROWS / 2;
    for (int i = 0; i < snake_len; i++) {
        snake[i].x = mx - i;
        snake[i].y = my;
    }
    place_food();

    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x050D1A), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    // HUD bar
    lv_obj_t *hud = lv_obj_create(scr);
    lv_obj_set_size(hud, 320, 36);
    lv_obj_align(hud, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_color(hud, lv_color_hex(0x0A1A30), 0);
    lv_obj_set_style_bg_opa(hud, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(hud, 0, 0);
    lv_obj_set_style_pad_all(hud, 4, 0);

    lv_obj_t *title = lv_label_create(hud);
    lv_label_set_text(title, "SNAKE");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x4ECDC4), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 4, 0);

    lbl_score = lv_label_create(hud);
    lv_label_set_text(lbl_score, "Score: 0");
    lv_obj_set_style_text_font(lbl_score, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_score, lv_color_white(), 0);
    lv_obj_align(lbl_score, LV_ALIGN_RIGHT_MID, -4, 0);

    // Back button
    lv_obj_t *back = lv_btn_create(hud);
    lv_obj_set_size(back, 10, 20);  // minimal size label only
    lv_obj_set_style_bg_opa(back, LV_OPA_TRANSP, 0);
    lv_obj_set_style_shadow_width(back, 0, 0);
    lv_obj_set_style_border_width(back, 0, 0);
    lv_obj_set_style_pad_all(back, 0, 0);
    lv_obj_align(back, LV_ALIGN_RIGHT_MID, -64, 0);
    lv_obj_add_event_cb(back, [](lv_event_t *e) {
        if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
            if (game_timer) { lv_timer_del(game_timer); game_timer = nullptr; }
            menu_show();
        }
    }, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *blbl = lv_label_create(back);
    lv_label_set_text(blbl, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(blbl, lv_color_hex(0x4ECDC4), 0);
    lv_obj_center(blbl);

    // Canvas
    canvas = lv_canvas_create(scr);
    lv_canvas_set_buffer(canvas, cbuf, CANVAS_W, CANVAS_H, LV_IMG_CF_TRUE_COLOR);
    lv_obj_align(canvas, LV_ALIGN_BOTTOM_MID, 0, -3);
    lv_obj_add_event_cb(canvas, canvas_touch_cb, LV_EVENT_PRESSED,  nullptr);
    lv_obj_add_event_cb(canvas, canvas_touch_cb, LV_EVENT_RELEASED, nullptr);
    lv_obj_clear_flag(canvas, LV_OBJ_FLAG_SCROLLABLE);

    // Start hint
    lv_obj_t *hint = lv_label_create(scr);
    lv_label_set_text(hint, "Swipe to start!");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x4ECDC4), 0);
    lv_obj_center(hint);

    render();
    game_timer = lv_timer_create(game_step, 130, nullptr);
}
