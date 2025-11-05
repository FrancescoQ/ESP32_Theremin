/*
 * PerformanceMonitor.cpp
 *
 * Implementation of watchdog-style performance monitoring.
 * Alerts when timing or RAM approaches critical thresholds.
 */

#include "system/PerformanceMonitor.h"
#include "system/DisplayManager.h"
#include "system/Debug.h"

PerformanceMonitor::PerformanceMonitor(DisplayManager* displayMgr)
    : display(displayMgr),
      lastAudioWarn(0),
      lastRamWarn(0),
      latestAudioWorkTimeUs(0),
      lastStatusReport(0) {
}

void PerformanceMonitor::begin() {
  DEBUG_PRINTLN("[PERF] Watchdog monitoring active");
  lastStatusReport = millis();
}

void PerformanceMonitor::setDisplay(DisplayManager* displayMgr) {
  display = displayMgr;

  // Register performance page if display is available
  if (display) {
    display->registerPage("Performance", [this](Adafruit_SSD1306& oled) {
      this->drawPerformancePage(oled);
    }, "SYSTEM");  // Add title

    // Register performance warning overlay (appears on all pages)
    display->registerOverlay([this](Adafruit_SSD1306& oled) {
      if (!this->isSystemOK()) {
        // Draw warning indicator in top-left corner
        // Triangle warning icon
        oled.fillRect(0, 0, 16, 16, SSD1306_BLACK);
        oled.drawRect(0, 0, 14, 12, SSD1306_WHITE);
        oled.fillTriangle(3, 9, 7, 2, 11, 9, SSD1306_WHITE);
        oled.drawPixel(7, 6, SSD1306_BLACK);  // Exclamation mark dot
        oled.drawLine(7, 4, 7, 4, SSD1306_BLACK);  // Exclamation mark line
      }
    });
  }
}

void PerformanceMonitor::update() {
  // Check RAM
  checkRAM();

  // Print periodic status report
  printStatus();
}

void PerformanceMonitor::beginAudioMeasurement() {
  // No-op, kept for API compatibility
}

void PerformanceMonitor::recordAudioWork(uint32_t workTimeUs) {
  // Track latest timing for status report
  latestAudioWorkTimeUs = workTimeUs;

  // Check if audio processing is taking too long
  // Why "11ms" is the target:
  // - Audio buffer duration = 256 samples / 22050 Hz = 11.6ms
  // - Audio task must finish computation before next buffer needed
  // - If CPU work exceeds 11ms, audio will start dropping samples (distortion)
  // - Current typical values: ~0.3ms (2-3% of available time)
  if (workTimeUs > AUDIO_WARN_US) {
    // Throttle warnings (don't spam console)
    uint32_t now = millis();
    if (now - lastAudioWarn > WARN_THROTTLE_MS) {
      DEBUG_PRINT("[WARN] AUDIO CPU HIGH: ");
      DEBUG_PRINT(workTimeUs / 1000);
      DEBUG_PRINT(".");
      DEBUG_PRINT((workTimeUs % 1000) / 100);
      DEBUG_PRINT("ms / 11ms available (");
      DEBUG_PRINT((workTimeUs * 100) / 11000);
      DEBUG_PRINTLN("%)");
      lastAudioWarn = now;
    }
  }
}


void PerformanceMonitor::checkRAM() {
  uint32_t freeHeap = ESP.getFreeHeap();

  if (freeHeap < RAM_WARN_BYTES) {
    // Throttle warnings (don't spam console)
    uint32_t now = millis();
    if (now - lastRamWarn > WARN_THROTTLE_MS) {
      DEBUG_PRINT("[WARN] RAM LOW: ");
      DEBUG_PRINT(freeHeap / 1024);
      DEBUG_PRINT(".");
      DEBUG_PRINT((freeHeap % 1024) / 102);
      DEBUG_PRINTLN(" KB free");
      lastRamWarn = now;
    }
  }
}

void PerformanceMonitor::printStatus() {
  uint32_t now = millis();

  if (now - lastStatusReport > STATUS_INTERVAL_MS) {
    uint32_t freeHeap = ESP.getFreeHeap();

    // Display format: "Audio: X.Xms/11ms (Y%)"
    // - X.Xms = actual CPU time spent generating audio samples
    // - 11ms = time available per buffer (256 samples / 22050 Hz)
    // - Y% = CPU utilization (should stay well below 100%)
    DEBUG_PRINT("[OK] System OK - Audio: ");
    DEBUG_PRINT(latestAudioWorkTimeUs / 1000);
    DEBUG_PRINT(".");
    DEBUG_PRINT((latestAudioWorkTimeUs % 1000) / 100);
    DEBUG_PRINT("ms/11ms (");
    DEBUG_PRINT((latestAudioWorkTimeUs * 100) / 11000);
    DEBUG_PRINT("%) / RAM: ");
    DEBUG_PRINT(freeHeap / 1024);
    DEBUG_PRINT(".");
    DEBUG_PRINT((freeHeap % 1024) / 102);
    DEBUG_PRINTLN(" KB free");

    lastStatusReport = now;
  }
}

void PerformanceMonitor::drawPerformancePage(Adafruit_SSD1306& oled) {
  // Title and separator are auto-drawn by DisplayManager
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);

  // Status line (starts at y=14 after title+separator)
  oled.setCursor(0, 14);
  oled.print("Status: ");
  oled.print(isSystemOK() ? "OK" : "WARN");

  // Audio timing line
  oled.setCursor(0, 24);
  oled.print("Audio:  ");
  float audioMs = getAudioTimeMs();
  oled.print(audioMs, 1);  // 1 decimal place
  oled.print("ms/11ms");

  // RAM line
  oled.setCursor(0, 34);
  oled.print("RAM:    ");
  oled.print(getFreeRAMKB());
  oled.print(" KB free");
}
