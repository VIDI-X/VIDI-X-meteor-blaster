# 🚀 VIDI X Game - Meteor Blaster

A fast-paced arcade-style shooter game for the VIDI X microcontroller, featuring smooth graphics powered by LovyanGFX, reactive LED feedback with FastLED, and immersive audio effects via PWM speaker control. The game challenges players to survive a meteor storm, collect energy cells, and rack up points with precision shooting.

## 🎮 Gameplay Overview

You control a spaceship navigating through waves of falling meteors. Avoid collisions, collect energy cells, and blast meteors into smaller fragments to earn points. Once the player's health reaches zero, it's game over — but you can always restart and try again!

## 🧠 Features

* Custom graphics rendering with `LovyanGFX` on ILI9341 display.
* Dynamic meteor physics with randomized shapes and real-time collision detection.
* Analog joystick support using GPIO34 (horizontal) and GPIO35 (vertical).
* Five RGB LEDs for immersive visual feedback (via FastLED).
* PWM-controlled sound effects using a piezo speaker.
* Energy cell collection system to boost score and energy.
* Bullet shooting mechanics with sprite-based cannon flash animation.

## 📦 Hardware Requirements

* **VIDI X microcomputer** (ESP32-based microcontroller)
* **ILI9341 display** (240x320)
* **Buttons** (UP, DOWN, LEFT, RIGHT, buttons connected to GPIO34 & GPIO35, Button A - GPIO32, Restart Button - GPIO0)
* **RGB LEDs** (WS2812B x5 on GPIO26)
* **Speaker** (PWM audio on GPIO25)

## ⚙️ Libraries Used

* `LovyanGFX` for display control
* `FastLED` for addressable LEDs

## 🕹️ Controls

* **Joystick left/right**: Move ship horizontally
* **Joystick up/down**: Move ship vertically
* **Button A (GPIO32)**: Fire cannon
* **Restart Button (GPIO0)**: Reset game after game over

## 💡 Code Highlights

* `LGFX` class sets up the screen with custom SPI bus.
* `drawPlayerShip()` renders the ship with shading, canopy highlights, engine trails, and flashing cannons.
* `splitMeteor()` handles meteor fragmentation upon bullet hit.
* `updateGame()` performs all real-time physics, input handling, collision detection, and spawning logic.
* `drawGame()` paints all game elements onto a sprite canvas and displays it on screen.

## 🛠️ Setup Instructions

1. Clone or copy the code into Arduino IDE.
2. Install dependencies: `LovyanGFX`, `FastLED`.
3. Select the correct board: **ESP32 Dev Module**.
4. Flash the sketch to VIDI X.
5. Play and enjoy!

## 📽️ Demo & Screenshots
<img width="4096" height="3072" alt="IMG_20260319_160142" src="https://github.com/user-attachments/assets/259176af-ce60-4017-a035-f5085bff74c3" />


## 📋 License

This project is open-source and free to use under the MIT license. Contributions and feedback are welcome!

---

Made with ❤️ for the VIDI X community.
