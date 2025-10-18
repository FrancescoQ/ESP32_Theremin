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

// Create theremin instance
Theremin theremin;

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
}

void loop() {
    // Update theremin (read sensors, generate audio)
    theremin.update();

    // Small delay for stability (~50Hz update rate)
    delay(20);
}
