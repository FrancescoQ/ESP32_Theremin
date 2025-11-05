/*
 * ESP32 Theremin - Main Entry Point
 *
 * Clean, modular architecture using separate classes for:
 * - SensorManager: Handles distance sensor input (VL53L0X sensors)
 * - AudioEngine: Manages audio synthesis (currently PWM, future DAC support)
 * - Theremin: Coordinates sensors and audio
 */

#include <Arduino.h>
#include <Wire.h>
#include "system/Debug.h"
#include "system/Theremin.h"
#include "system/PinConfig.h"
#include "system/PerformanceMonitor.h"
#include "system/DisplayManager.h"

#if ENABLE_OTA
#include "system/OTAManager.h"
#endif

#if ENABLE_GPIO_MONITOR
#include "controls/GPIOMonitor.h"
#endif

// Main loop timing
static const int UPDATE_INTERVAL_MS = 5; // ms update interval in the main loop

// Create performance monitor instance (must be created before Theremin)
PerformanceMonitor performanceMonitor;

// Create display manager instance
DisplayManager display;

// Create theremin instance (pass performance monitor for CPU tracking)
// Used as "dependency injection" to allow monitoring also things that don't
// belong to the Theremin.
Theremin theremin(&performanceMonitor, &display);

#if ENABLE_OTA
// Create OTA manager instance
// AP Name: "Theremin-OTA", AP Password: "" (open network)
// OTA Auth: username "admin", password "theremin"
// Enable pin: PIN_OTA_ENABLE from PinConfig.h (-1 = always active, >=0 = button)
OTAManager ota("Theremin-OTA", "", PIN_OTA_ENABLE);
#endif

#if ENABLE_GPIO_MONITOR
// Create GPIO monitor instance for MCP23017 debugging
// Address 0x20 from PinConfig.h (PIN_SWITCH_EXPANDER_ADDR)
GPIOMonitor gpioMonitor(PIN_SWITCH_EXPANDER_ADDR);
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

  // Initialize I2C bus (shared by sensors, MCP23017, and display)
  Wire.begin(PIN_SENSOR_I2C_SDA, PIN_SENSOR_I2C_SCL);
  DEBUG_PRINTLN("[I2C] Bus initialized on SDA=21, SCL=22");
  delay(50);

  // Initialize display
  if (display.begin()) {
    DEBUG_PRINTLN("[Display] SSD1306 initialized successfully");
    // Show test message
    display.clear();
    display.showCenteredText("TheremAIn 1.0");
    display.update();
    DEBUG_PRINTLN("[Display] Test message displayed");
  } else {
    DEBUG_PRINTLN("[Display] WARNING: Failed to initialize");
    DEBUG_PRINTLN("[Display] Check wiring and I2C address (0x3C or 0x3D)");
  }
  delay(100);

  #if ENABLE_GPIO_MONITOR
    // Initialize GPIO monitor for MCP23017 debugging
    if (gpioMonitor.begin()) {
      DEBUG_PRINTLN("[GPIO] Monitor initialized - wiggle controls!");
      // Print initial state for reference
      gpioMonitor.printCurrentState();
    } else {
      DEBUG_PRINTLN("[GPIO] WARNING: Monitor failed to initialize");
      DEBUG_PRINTLN("[GPIO] Check MCP23017 wiring and I2C address (0x20)");
    }
    delay(100);
  #endif

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
    // If we start the system with 3 oscillators OFF and all octave switch -1 enable OTA:
    OTAManager::OTAForceState otaForcedState = OTAManager::ALWAYS_DISABLE;
    if (theremin.getAudioEngine()->getSpecialState(1)) {
      otaForcedState = OTAManager::ALWAYS_ENABLE;
    }

    if (ota.begin("admin", "theremin", otaForcedState)) {
      DEBUG_PRINTLN("[OTA] OTA updates enabled");
    } else {
      DEBUG_PRINTLN("[OTA] Failed to start OTA manager");
    }
  #endif

  // Turn on effects.
  if (theremin.getAudioEngine()->getSpecialState(2)) {
    theremin.getAudioEngine()->getEffectsChain()->setDelayEnabled(true);
    DEBUG_PRINTLN("[STARTUP] Delay enabled");
  }
  if (theremin.getAudioEngine()->getSpecialState(3)) {
    theremin.getAudioEngine()->getEffectsChain()->setChorusEnabled(true);
    DEBUG_PRINTLN("[STARTUP] Chorus enabled");
  }
  if (theremin.getAudioEngine()->getSpecialState(4)) {
    theremin.getAudioEngine()->getEffectsChain()->setReverbEnabled(true);
    DEBUG_PRINTLN("[STARTUP] Reverb enabled");
  }
}

void loop() {
  // Update theremin (handles controls, sensors, and audio)
  theremin.update();

  #if ENABLE_GPIO_MONITOR
    // Poll GPIO monitor for pin changes (non-blocking)
    gpioMonitor.update();
  #endif

  #if ENABLE_OTA
    // Handle OTA requests (non-blocking)
    ota.handle();
  #endif

  // Update monitoring (checks RAM, prints periodic status)
  performanceMonitor.update();

  // Small delay for stability
  delay(UPDATE_INTERVAL_MS);
}
