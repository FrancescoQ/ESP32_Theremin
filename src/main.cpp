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
  DEBUG_INIT(115200);
  delay(500);  // Increased delay to let Serial stabilize

  // Clear startup banner to separate from bootloader output
  DEBUG_PRINTLN("\n\n========================================");
  DEBUG_PRINTLN("   ESP32 Theremin Starting...");
  DEBUG_PRINTLN("========================================\n");
  Serial.flush();  // Ensure banner is sent before continuing
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
  Serial.flush();  // Ensure initialization messages are sent
  delay(100);

  // Test Phase A: Direct API calls
  DEBUG_PRINTLN("\n[TEST] Testing oscillator control API...");
  theremin.getAudioEngine()->setFrequency(440);  // A4 - concert pitch
  theremin.getAudioEngine()->setAmplitude(50);   // 50% volume
  delay(2000);  // Play default sound for 2 seconds

  // Change oscillator 1 to sawtooth
  DEBUG_PRINTLN("[TEST] Changing OSC1 to sawtooth...");
  theremin.getAudioEngine()->setOscillatorWaveform(1, Oscillator::TRIANGLE);
  theremin.getAudioEngine()->setOscillatorWaveform(2, Oscillator::OFF);
  theremin.getAudioEngine()->setOscillatorWaveform(3, Oscillator::OFF);
  delay(2000);

  // Change oscillator 1 octave down
  DEBUG_PRINTLN("[TEST] Shifting OSC1 down one octave...");
  theremin.getAudioEngine()->setOscillatorOctave(1, Oscillator::OCTAVE_UP);
  delay(2000);

  // Change oscillator 1 volume to 50%
  DEBUG_PRINTLN("[TEST] Setting OSC1 volume to 50%...");
  theremin.getAudioEngine()->setOscillatorVolume(1, 0.5);
  delay(2000);

  DEBUG_PRINTLN("[TEST] Restoring default settings...");
  theremin.getAudioEngine()->setOscillatorWaveform(1, Oscillator::SINE);
  theremin.getAudioEngine()->setOscillatorOctave(1, Oscillator::OCTAVE_BASE);
  theremin.getAudioEngine()->setOscillatorVolume(1, 1.0);
  delay(2000);

  DEBUG_PRINTLN("[TEST] Phase A test complete!\n");

  // Initialize performance monitoring
  performanceMonitor.begin();

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
