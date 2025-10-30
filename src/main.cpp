/*
 * ESP32 Theremin - Main Entry Point
 *
 * Clean, modular architecture using separate classes for:
 * - SensorManager: Handles distance sensor input (VL53L0X sensors)
 * - AudioEngine: Manages audio synthesis (currently PWM, future DAC support)
 * - Theremin: Coordinates sensors and audio
 */

#include <Arduino.h>
#include "Debug.h"
#include "Theremin.h"
#include "PinConfig.h"
#include "PerformanceMonitor.h"
#include "ControlHandler.h"

#if ENABLE_OTA
#include "OTAManager.h"
#endif

// Main loop timing
static const int UPDATE_INTERVAL_MS = 5; // ms update interval in the main loop

// Create performance monitor instance (must be created before Theremin)
PerformanceMonitor performanceMonitor;

// Create theremin instance (pass performance monitor for CPU tracking)
Theremin theremin(&performanceMonitor);

// Create control handler instance (will be initialized in setup, to pass the
// initialized AudioEngine to the constructor)
ControlHandler* controls = nullptr;

#if ENABLE_OTA
// Create OTA manager instance
// AP Name: "Theremin-OTA", AP Password: "" (open network)
// OTA Auth: username "admin", password "theremin"
// Enable pin: PIN_OTA_ENABLE from PinConfig.h (-1 = always active, >=0 = button)
OTAManager ota("Theremin-OTA", "", PIN_OTA_ENABLE);
#endif

void setup() {
  // Initialize debug output
  Serial.begin(115200);
  delay(500);  // Increased delay to let Serial stabilize

  // Clear startup banner to separate from bootloader output
  DEBUG_PRINTLN("\n\n========================================");
  DEBUG_PRINTLN("   ESP32 Theremin Starting...");
  DEBUG_PRINTLN("========================================\n");
  DEBUG_FLUSH();  // Ensure banner is sent before continuing
  delay(100);

  // Initialize theremin (sensors + audio)
  if (!theremin.begin()) {
    DEBUG_PRINTLN("\n[FATAL] Theremin initialization failed!");
    DEBUG_PRINTLN("System halted.");
    while (1) {
      delay(1000);
    }
  }

  DEBUG_PRINTLN("=== Ready to Play! ===\n");
  DEBUG_FLUSH();  // Ensure initialization messages are sent
  delay(100);

  // Initialize performance monitoring
  performanceMonitor.begin();

  // Run system test
  theremin.getAudioEngine()->systemTest();
  theremin.getAudioEngine()->playStartupSound();
  delay(500);

  // Initialize control handler (pass theremin pointer for full control access)
  controls = new ControlHandler(&theremin);
  controls->begin();

#if ENABLE_OTA
  // Initialize OTA manager
  if (ota.begin("admin", "theremin")) {
    DEBUG_PRINTLN("[OTA] OTA updates enabled");
  } else {
    DEBUG_PRINTLN("[OTA] Failed to start OTA manager");
  }
#endif
}

void loop() {
  // Handle control inputs (serial commands, GPIO in future phases)
  if (controls != nullptr) {
    controls->update();
  }

  // Update theremin (read sensors, generate audio)
  theremin.update();

  #if ENABLE_OTA
    // Handle OTA requests (non-blocking)
    ota.handle();
  #endif

  // Update monitoring (checks RAM, prints periodic status)
  performanceMonitor.update();

  // Small delay for stability
  delay(UPDATE_INTERVAL_MS);
}
