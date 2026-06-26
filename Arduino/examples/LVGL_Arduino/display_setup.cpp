/*
 * display_setup.cpp
 * TFT + LVGL initialisation for Waveshare ESP32-S3 2.8" (B) 320x240
 * Touch controller: XPT2046 (SPI)  – adjust pins if your board differs
 */

#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include "display_setup.h"

// -------- Screen size --------
#define SCREEN_W 320
#define SCREEN_H 240

// -------- Touch SPI pins (Waveshare ESP32-S3 2.8" B) --------
#define TOUCH_CS   33
#define TOUCH_IRQ  36   // set -1 if not used

// -------- LVGL buffer (1/10th of screen) --------
#define BUF_LINES  (SCREEN_H / 10)
static lv_color_t buf1[SCREEN_W * BUF_LINES];
static lv_color_t buf2[SCREEN_W * BUF_LINES];
static lv_disp_draw_buf_t draw_buf;
static lv_disp_drv_t      disp_drv;
static lv_indev_drv_t     indev_drv;

TFT_eSPI tft = TFT_eSPI();
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

// -------- Flush callback --------
static void disp_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)color_p, w * h, true);
    tft.endWrite();
    lv_disp_flush_ready(drv);
}

// -------- Touch read callback --------
static void touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    if (ts.tirqTouched() && ts.touched()) {
        TS_Point p = ts.getPoint();
        // Map raw touch coords to screen pixels
        // Calibration values – tweak if touch is off
        int16_t x = map(p.x, 300, 3800, 0, SCREEN_W - 1);
        int16_t y = map(p.y, 300, 3800, 0, SCREEN_H - 1);
        x = constrain(x, 0, SCREEN_W - 1);
        y = constrain(y, 0, SCREEN_H - 1);
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void display_init() {
    // TFT
    tft.begin();
    tft.setRotation(1);      // landscape
    tft.fillScreen(TFT_BLACK);

    // Touch (uses separate SPI bus on Waveshare B)
    ts.begin();
    ts.setRotation(1);

    // LVGL
    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, SCREEN_W * BUF_LINES);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res  = SCREEN_W;
    disp_drv.ver_res  = SCREEN_H;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    lv_indev_drv_init(&indev_drv);
    indev_drv.type     = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb  = touch_read;
    lv_indev_drv_register(&indev_drv);
}
