/*
 * ESP32 Theremin - Main Entry Point
 *
 * Clean, modular architecture using separate classes for:
 * - SensorManager: Handles distance sensor input (simulation or hardware)
 * - AudioEngine: Manages audio synthesis (currently PWM, future DAC support)
 * - Theremin: Coordinates sensors and audio
 *
 * Build Commands:
 * - Simulation: pio run -e esp32dev-wokwi
 * - Hardware:   pio run -e esp32dev
 */

#include <Arduino.h>
#include "Debug.h"
#include "Theremin.h"

#ifdef ENABLE_OTA
#include "OTAManager.h"

// OTA Enable Pin Configuration
// Set to -1 to always enable OTA (no button check)
// Set to GPIO pin number (e.g., 0, 2, 4) to require button press during boot
#define OTA_ENABLE_PIN -1  // -1 = always active, change to GPIO pin when decided
#endif

// Create theremin instance
Theremin theremin;

#ifdef ENABLE_OTA
// Create OTA manager instance
// AP Name: "Theremin-OTA", AP Password: "" (open network)
// OTA Auth: username "admin", password "theremin"
// Enable pin: OTA_ENABLE_PIN (-1 = always active, >=0 = button on that GPIO)
OTAManager ota("Theremin-OTA", "", OTA_ENABLE_PIN);
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
