/*
 * NotificationManager.cpp
 *
 * Implementation of time-limited notification overlay system.
 */

#include "system/NotificationManager.h"
#include <Fonts/TomThumb.h>

// Static instance pointer for callback
NotificationManager* NotificationManager::instance = nullptr;

NotificationManager::NotificationManager(DisplayManager* display)
    : displayManager(display),
      currentMessage(""),
      hideTime(0),
      active(false) {
    // Set static instance for callback
    instance = this;

    // Register overlay with DisplayManager
    if (displayManager) {
        displayManager->registerOverlay(overlayCallbackStatic);
    }
}

void NotificationManager::show(String message, uint16_t durationMs) {
    currentMessage = message;
    hideTime = millis() + durationMs;
    active = true;
}

void NotificationManager::update() {
    // Check if it's time to hide the notification
    if (active && millis() >= hideTime) {
        clear();
    }
}

void NotificationManager::clear() {
    active = false;
    currentMessage = "";
}

void NotificationManager::drawOverlay(Adafruit_SSD1306& display) {
    if (!active || currentMessage.length() == 0) {
        return;  // Nothing to draw
    }

    // Set small font for compact display
    display.setFont(&TomThumb);
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    // Measure text width (TomThumb is ~4px per character)
    int16_t x1, y1;
    uint16_t textWidth, textHeight;
    display.getTextBounds(currentMessage, 0, 0, &x1, &y1, &textWidth, &textHeight);

    // Position at bottom-center
    // TomThumb baseline is about 5px above cursor position
    const int PADDING = 4;
    const int BOX_HEIGHT = textHeight + (PADDING * 2);
    const int BOX_WIDTH = textWidth + (PADDING * 2);
    const int BOX_X = (DisplayManager::SCREEN_WIDTH - BOX_WIDTH) / 2;
    const int BOX_Y = DisplayManager::SCREEN_HEIGHT - BOX_HEIGHT - 2;

    // Draw background box with border for contrast
    display.fillRect(BOX_X, BOX_Y, BOX_WIDTH, BOX_HEIGHT, SSD1306_WHITE);
    display.drawRect(BOX_X, BOX_Y, BOX_WIDTH, BOX_HEIGHT, SSD1306_BLACK);

    // Draw text (inverted colors for visibility on white background)
    display.setTextColor(SSD1306_BLACK);
    int textX = BOX_X + PADDING;
    int textY = BOX_Y + PADDING + textHeight;  // TomThumb needs baseline adjustment
    display.setCursor(textX, textY);
    display.print(currentMessage);

    // Reset to default font
    display.setFont(nullptr);
}

void NotificationManager::overlayCallbackStatic(Adafruit_SSD1306& display) {
    if (instance) {
        instance->drawOverlay(display);
    }
}
