/*
 * PerformanceMonitor.h
 *
 * Watchdog-style performance monitoring for ESP32 Theremin.
 * Alerts when timing or RAM approaches critical thresholds.
 * Silent when everything is OK.
 */

#pragma once
#include <Arduino.h>

// Forward declarations
class DisplayManager;
class Adafruit_SSD1306;

class PerformanceMonitor {
 public:
  /**
   * Constructor
   * @param displayMgr Pointer to DisplayManager for page registration (optional)
   */
  PerformanceMonitor(DisplayManager* displayMgr = nullptr);

  /**
   * Initialize performance monitoring
   * Call in setup()
   */
  void begin();

  /**
   * Set display manager and register performance page
   * Call after display is initialized
   */
  void setDisplay(DisplayManager* displayMgr);

  /**
   * Update monitoring (checks RAM, prints periodic status)
   * Call frequently in loop()
   */
  void update();

  /**
   * Begin audio measurement (call once at task start)
   * Not actually needed anymore but kept for API compatibility
   */
  void beginAudioMeasurement();

  /**
   * Record audio work time and check threshold
   * @param workTimeUs Microseconds spent doing audio work
   */
  void recordAudioWork(uint32_t workTimeUs);

  /**
   * Get audio work time in milliseconds (for display)
   * @return Audio work time in ms
   */
  float getAudioTimeMs() const { return latestAudioWorkTimeUs / 1000.0f; }

  /**
   * Get free RAM in kilobytes (for display)
   * @return Free heap memory in KB
   */
  uint32_t getFreeRAMKB() const { return ESP.getFreeHeap() / 1024; }

  /**
   * Check if system is within performance thresholds
   * @return true if system is OK, false if warnings present
   */
  bool isSystemOK() const {
    return (latestAudioWorkTimeUs < AUDIO_WARN_US) &&
           (ESP.getFreeHeap() > RAM_WARN_BYTES);
  }

 private:
  DisplayManager* display;

  /**
   * Draw performance page for display
   */
  void drawPerformancePage(Adafruit_SSD1306& oled);
  // Performance thresholds (when to warn)
  static const uint32_t AUDIO_WARN_US = 8000;      // 8ms (70% of 11ms buffer period)
  static const uint32_t RAM_WARN_BYTES = 50000;    // 50KB minimum free

  // Warning throttling (don't spam console)
  static const uint32_t WARN_THROTTLE_MS = 5000;  // Only warn every 5 seconds
  uint32_t lastAudioWarn;
  uint32_t lastRamWarn;

  // Track latest audio timing for status report
  uint32_t latestAudioWorkTimeUs;

  // Periodic status report
  static const uint32_t STATUS_INTERVAL_MS = 30000;  // Report every 30 seconds
  uint32_t lastStatusReport;

  /**
   * Check RAM and warn if low
   */
  void checkRAM();

  /**
   * Print periodic status report (optional reassurance)
   */
  void printStatus();
};
