/*
 * DisplayManager.cpp
 *
 * Implementation of DisplayManager for SSD1306 OLED display.
 */

#include "system/DisplayManager.h"
#include "system/Debug.h"

const GFXfont* DisplayManager::SMALL_FONT = &TomThumb;

DisplayManager::DisplayManager()
    : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET),
      initialized(false) {
}

bool DisplayManager::begin() {
    DEBUG_PRINTLN("DisplayManager: Initializing SSD1306 display...");

    // Initialize display with I2C address from PinConfig
    if (!display.begin(SSD1306_SWITCHCAPVCC, PIN_DISPLAY_I2C_ADDR)) {
        DEBUG_PRINTLN("DisplayManager: ERROR - SSD1306 allocation failed!");
        DEBUG_PRINTF("DisplayManager: Check I2C address (trying 0x%02X)\n", PIN_DISPLAY_I2C_ADDR);
        initialized = false;
        return false;
    }

    DEBUG_PRINTLN("DisplayManager: Display initialized successfully");

    // Clear display
    display.clearDisplay();
    display.display();
    //display.setFont(&TomThumb);

    initialized = true;
    return true;
}

void DisplayManager::clear() {
    if (!initialized) {
        return;
    }
    display.clearDisplay();
}

void DisplayManager::showText(const char* text, int x, int y, int size, uint16_t color, const GFXfont* font) {
  if (!initialized) {
    return;
  }

  if (font) {
    display.setFont(font);
  }
  display.setTextSize(size);
  display.setTextColor(color);
  display.setCursor(x, y);
  display.print(text);
}

void DisplayManager::showCenteredText(const char* text, int size) {
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  int x = (SCREEN_WIDTH - w) / 2;
  int y = (SCREEN_HEIGHT - h) / 2;
  showText(text, x, y, size);
}

void DisplayManager::update() {
    if (!initialized) {
        return;
    }
    display.display();
}
