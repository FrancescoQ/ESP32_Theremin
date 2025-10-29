/*
 * PerformanceMonitor.cpp
 *
 * Implementation of watchdog-style performance monitoring.
 * Alerts when timing or RAM approaches critical thresholds.
 */

#include "PerformanceMonitor.h"
#include "Debug.h"

PerformanceMonitor::PerformanceMonitor()
    : lastAudioWarn(0),
      lastRamWarn(0),
      latestAudioWorkTimeUs(0),
      lastStatusReport(0) {
}

void PerformanceMonitor::begin() {
  DEBUG_PRINTLN("[PERF] Watchdog monitoring active");
  lastStatusReport = millis();
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
