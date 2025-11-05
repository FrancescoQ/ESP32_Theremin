/*
 * DisplayManager.h
 *
 * Simple display manager for SSD1306 OLED display.
 * Handles initialization and basic text output.
 */

#pragma once

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Smallest fonts
//#include <Fonts/TomThumb.h>       // 3x5 px - Small but readable
//#include <Fonts/Tiny3x3a2pt7b.h>  // 3x3 px - Almost unreadable but really compact

// Small fonts
//#include <Fonts/Picopixel.h>  // 3x5 px - Minimal
//#include <Fonts/Org_01.h>     // 5x7 px - Retrò

// Normal/Monospaced fonts
//#include <Fonts/FreeMono9pt7b.h>  // Monospace 9pt
//#include <Fonts/FreeSans9pt7b.h>  // Sans-serif 9pt

#include "system/PinConfig.h"

class DisplayManager {
public:
    DisplayManager();

    static constexpr int CHAR_HEGHT = 8;
    static constexpr int CHAR_WIDTH = 5;
    static constexpr int LINE_HEIGHT = 10;
    static constexpr int SCREEN_WIDTH = 128;
    static constexpr int SCREEN_HEIGHT = 64;

    // Initialize the display
    bool begin();

    // Clear the display
    void clear();

    // Display text at position
    void showText(const char* text, int x = 0, int y = 0, int size = 1);

    // Display centered text
    void showCenteredText(const char* text, int size = 1);

    // Update display (call after drawing)
    void update();

    // Check if display is initialized
    bool isInitialized() const { return initialized; }

   private:
    Adafruit_SSD1306 display;
    bool initialized;

    static constexpr int OLED_RESET = -1;  // Reset pin not used
};
