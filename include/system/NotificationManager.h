/*
 * NotificationManager.h
 *
 * Time-limited notification overlay system for displaying control changes.
 * Notifications appear briefly (default 2 seconds) and auto-hide.
 *
 * Example notifications:
 * - "OSC1:SIN" - Oscillator 1 set to Sine wave
 * - "REV:LNG" - Reverb set to Long preset
 * - "SMT:MIN" - Smoothing set to minimum
 */

#pragma once

#include <Arduino.h>
#include "system/DisplayManager.h"

class NotificationManager {
public:
    /**
     * Constructor
     * @param display Pointer to DisplayManager for overlay registration
     */
    NotificationManager(DisplayManager* display);

    /**
     * Show notification message for specified duration
     * @param message Message to display (e.g., "OSC1:SIN", "REV:LNG")
     * @param durationMs How long to show notification in milliseconds (default: 2000)
     */
    void show(String message, uint16_t durationMs = 2000);

    /**
     * Update notification state - call in main loop
     * Handles auto-hide timing
     */
    void update();

    /**
     * Clear current notification immediately
     */
    void clear();

    /**
     * Check if a notification is currently active
     * @return true if notification is being displayed
     */
    bool isActive() const { return active; }

    /**
     * Get current notification message
     * @return Current message, or empty string if not active
     */
    String getCurrentMessage() const { return active ? currentMessage : ""; }

private:
    DisplayManager* displayManager;
    String currentMessage;
    unsigned long hideTime;  // millis() timestamp when to hide
    bool active;

    /**
     * Overlay callback function for DisplayManager
     * Draws the notification at bottom-center of screen
     */
    void drawOverlay(Adafruit_SSD1306& display);

    /**
     * Static wrapper for overlay callback (required for DisplayManager)
     */
    static NotificationManager* instance;
    static void overlayCallbackStatic(Adafruit_SSD1306& display);
};
