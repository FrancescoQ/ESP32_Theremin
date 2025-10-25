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

#ifdef ENABLE_OTA
#include "OTAManager.h"
#endif

// Create theremin instance
Theremin theremin;

#ifdef ENABLE_OTA
// Create OTA manager instance
// AP Name: "Theremin-OTA", AP Password: "" (open network)
// OTA Auth: username "admin", password "theremin"
// Enable pin: PIN_OTA_ENABLE from PinConfig.h (-1 = always active, >=0 = button)
OTAManager ota("Theremin-OTA", "", PIN_OTA_ENABLE);
#endif

void setup() {
  // Initialize debug output
  DEBUG_INIT(9600);
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

#ifdef ENABLE_OTA
  // Initialize OTA manager
  if (ota.begin("admin", "theremin")) {
    DEBUG_PRINTLN("[OTA] OTA updates enabled");
  } else {
    DEBUG_PRINTLN("[OTA] Failed to start OTA manager");
  }
#endif
}

void loop() {
  // Update theremin (read sensors, generate audio)
  theremin.update();

  #ifdef ENABLE_OTA
    // Handle OTA requests (non-blocking)
    ota.handle();
  #endif

  // Small delay for stability (~50Hz update rate)
  delay(20);
}
