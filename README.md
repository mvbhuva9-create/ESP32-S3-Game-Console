# ESP32-S3 Handheld Game Console
A portable retro gaming console built on the **ESP32-S3** microcontroller using the **LVGL** graphics library. This project features a custom game selector menu and three classic games (Snake, Pong, and Flappy Bird) designed to run smoothly on an embedded display.

---

## 🚀 Project Features
* **Custom UI Launcher:** A clean, modern game selection menu built entirely using LVGL objects, event callbacks, and custom design styles.
* **Responsive Touch Controls:** Features real-time touchscreen tracking mapped directly to screen coordinates for precise paddle dragging and swiping.
* **Optimized Video Memory:** Uses display double-buffering to load and push pixel data quickly over SPI, ensuring steady frame rates during gameplay.

---

## 📁 Repository Structure
```text

├── Arduino/
├── examples/
│   └── LVGL_Arduino/                 # Main project directory
│       ├── LVGL_Arduino.ino          # Main entry point and FreeRTOS task configuration
│       ├── display_setup.cpp         # Display initialization and touchscreen calibration
│       ├── display_setup.h           # Display initialization headers
│       ├── menu.cpp                  # Game launcher menu screen and touch button events
│       ├── menu.h                    # Menu interface headers
│       ├── snake.cpp                 # Snake game logic (grid movement and fruit spawning)
│       ├── snake.h                   # Snake game headers
│       ├── pong.cpp                  # Pong game logic (ball physics and single-player CPU AI)
│       ├── pong.h                    # Pong game headers
│       ├── flappy.cpp                # Flappy bird game logic (gravity simulation and jump physics)
│       └── flappy.h                  # Flappy bird game headers
└── 3D Prints/                        # 3D printed components for the handheld console enclosure
    ├── buttons.stl                   # Physical buttons design file
    ├── case final7.stl               # Main outer protective shell design file
    └── Component271.stl              # Internal brackets/hardware mounts design file

```


---

## 🎮 Game Implementations

### 1. Pong
* **Physics:** Tracks the ball's X/Y velocity vector and calculates precise bounces when hitting screen walls or paddles
* **Opponent:** Includes an automated opponent paddle that tracks the ball's position vertically to play against the user.

### 2. Snake
* **Grid Movement:** Uses a 2D coordinate grid array to manage the snake's body segments.
* **Memory Management:** Utilizes low-level memory shifting to smoothly move the body segments forward each frame.

### 3. Flappy Bird
* **Gravity Engine:** Simulates real-time physics by constantly applying downward gravity acceleration to the bird.
* **Collision Routine:** Runs continuous bounding-box checks to instantly detect if the player crashes into a pipe obstacle or out of bounds.

---

## 🛠️ Hardware & Tech Stack
* **Language:** C++ (Arduino Framework)
* **Graphics Library:** LVGL (Light and Versatile Graphics Library) v8
* **Target Device:** Waveshare ESP32-S3 2.8" (B) Touch Screen
* **Battery** Li-Ion 3.7 V, 500mAh Battery
---

## ⚡ How to Build and Run
1. Install the **LVGL** and **TFT_eSPI** libraries in your Arduino IDE or PlatformIO project setup.
2. Open `LVGL_Arduino.ino`.
3. Ensure the `TOUCH_CS` pin inside `display_setup.cpp` matches your board layout (Default is **GPIO 4**).
4. Select **ESP32S3 Dev Module** as your board target, compile, and upload!

## ⚡ How to Make it Portable
1. Connect the battery to the BAT port on your ESP32 Device 
2. In our case, we had a USB-C port on our integrated device so we could charge it through that port,
otherwise it will be necessary to buy a USB-C charging module and connect that to the battery.
3. Assemble it in the case and place buttons in the slots or you can use it without buttons as well.
