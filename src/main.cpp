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

#if ENABLE_NETWORK
#include "system/NetworkManager.h"
#endif

#if ENABLE_GPIO_MONITOR
#include "controls/GPIOMonitor.h"
#endif

// Main loop timing
static const int UPDATE_INTERVAL_MS = 5; // ms update interval in the main loop

// Create display manager instance (must be created before others)
DisplayManager display;

// Create performance monitor instance (pass nullptr for display initially)
PerformanceMonitor performanceMonitor(nullptr);

// Create theremin instance (pass performance monitor for CPU tracking)
// Used as "dependency injection" to allow monitoring also things that don't
// belong to the Theremin.
Theremin theremin(&performanceMonitor, &display);

#if ENABLE_NETWORK
// Create network manager instance (handles WiFi, OTA, and web server)
NetworkManager network(&display);
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
  } else {
    DEBUG_PRINTLN("[Display] WARNING: Failed to initialize");
    DEBUG_PRINTLN("[Display] Check wiring and I2C address (0x3C or 0x3D)");
  }
  delay(100);

  display.showLoadingScreen();


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

  // Initialize performance monitoring and register display page
  performanceMonitor.setDisplay(&display);
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

  #if ENABLE_NETWORK
    // Initialize network manager (WiFi, OTA, web server)
    // Check for WiFi reset condition (special state + button held)
    bool resetWiFi = false;
    if (theremin.getAudioEngine()->getSpecialState(1)) {
      // Special state detected - check if multi-function button is held
      if (theremin.getMCP().digitalRead(PIN_MULTI_BUTTON) == LOW) {
        resetWiFi = true;
        DEBUG_PRINTLN("[NETWORK] WiFi reset requested (special state + button held)");
      } else {
        // Special state but button not held - disable network completely
        DEBUG_PRINTLN("[NETWORK] Network capabilities disabled for 'special state' of controls.");
      }
    }

    // Start network if not disabled
    if (!theremin.getAudioEngine()->getSpecialState(1) || resetWiFi) {
      DEBUG_PRINTLN("[NETWORK] Enabling network.");
      network.begin("Theremin-Setup", "admin", "theremin", 15, 0, resetWiFi);
    }
  #endif
}

void loop() {
  // Update theremin (handles controls, sensors, and audio)
  theremin.update();

  // Update display (render current page)
  display.update();

  #if ENABLE_GPIO_MONITOR
    // Poll GPIO monitor for pin changes (non-blocking)
    gpioMonitor.update();
  #endif

  #if ENABLE_NETWORK
    // Update network manager (non-blocking)
    network.update();
  #endif

  // Update monitoring (checks RAM, prints periodic status)
  performanceMonitor.update();

  // Small delay for stability
  delay(UPDATE_INTERVAL_MS);
}
