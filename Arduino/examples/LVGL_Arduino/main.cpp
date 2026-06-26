/*
 * ESP32-S3 Game Console
 * Waveshare ESP32-S3 2.8" Touch Display (B variant)
 * Framework: Arduino + LVGL
 */

#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include "menu.h"
#include "display_setup.h"

// ---------- LVGL tick ----------
static uint32_t last_tick = 0;

void lvgl_tick_task(void) {
    uint32_t now = millis();
    lv_tick_inc(now - last_tick);
    last_tick = now;
}

void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println("Booting ESP32-S3 Game Console...");

    display_init();   // Init TFT + LVGL + touch
    menu_init();      // Draw the main menu

    Serial.println("Ready.");
}

void loop() {
    lvgl_tick_task();
    lv_timer_handler();
    delay(5);
}
