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
#include "DelayEffect.h"

#if ENABLE_OTA
#include "OTAManager.h"
#endif

// Main loop timing
static const int UPDATE_INTERVAL_MS = 5; // ms update interval in the main loop

// Create performance monitor instance (must be created before Theremin)
PerformanceMonitor performanceMonitor;

// Create theremin instance (pass performance monitor for CPU tracking)
Theremin theremin(&performanceMonitor);

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

  // ========== DELAY EFFECT UNIT TEST ==========
  // Temporary test to verify DelayEffect class works correctly
  // Remove this section after Phase A is verified
  #if 1  // Set to 1 to enable test, 0 to disable
    DEBUG_PRINTLN("\n========== DELAY EFFECT UNIT TEST ==========");

    // Create delay effect (100ms @ 22050 Hz)
    DelayEffect testDelay(100, 22050);
    testDelay.setEnabled(true);
    testDelay.setFeedback(0.5);
    testDelay.setMix(0.5);

    DEBUG_PRINTLN("[TEST] Testing delay with pulse pattern...");

    // Test with pulse pattern: pulse every 10 samples
    for (int i = 0; i < 50; i++) {
      int16_t input = (i % 10 == 0) ? 10000 : 0;  // Pulse: 10000, then 9 zeros
      int16_t output = testDelay.process(input);

      // Print sample number, input, and output
      Serial.printf("Sample %2d: in=%5d, out=%5d\n", i, input, output);

      if (i == 49) {
        DEBUG_PRINTLN("\n[TEST] Pattern analysis:");
        DEBUG_PRINTLN("  - Samples 0-9: Building delay buffer (should see input pulses)");
        DEBUG_PRINTLN("  - Samples 10+: Should see echoes from feedback");
        DEBUG_PRINTLN("  - Output should be mix of dry (input) and wet (delayed)");
      }
    }

    // Test reset functionality
    DEBUG_PRINTLN("\n[TEST] Testing reset...");
    testDelay.reset();
    DEBUG_PRINTLN("[TEST] Processing after reset (should be clean):");
    for (int i = 0; i < 5; i++) {
      int16_t input = (i == 0) ? 10000 : 0;
      int16_t output = testDelay.process(input);
      Serial.printf("Sample %2d: in=%5d, out=%5d\n", i, input, output);
    }

    DEBUG_PRINTLN("\n[TEST] ✓ DelayEffect unit test complete!");
    DEBUG_PRINTLN("============================================\n");
    delay(2000);  // Pause so user can read results
  #endif

    // Run system test (if enabled)
  #if ENABLE_STARTUP_TEST
    theremin.getAudioEngine()->systemTest();
  #endif

    // Play startup sound (if enabled)
  #if ENABLE_STARTUP_SOUND
    theremin.getAudioEngine()->playStartupSound();
    delay(500);
  #endif

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
  // Update theremin (handles controls, sensors, and audio)
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
