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
#include "Theremin.h"

// Create theremin instance
Theremin theremin;

void setup() {
    // Initialize serial communication
    Serial.begin(9600);
    delay(100);

    // Initialize theremin (sensors + audio)
    if (!theremin.begin()) {
        Serial.println("\n[FATAL] Theremin initialization failed!");
        Serial.println("System halted.");
        while (1) {
            delay(1000);
        }
    }

    Serial.println("=== Ready to Play! ===\n");
}

void loop() {
    // Update theremin (read sensors, generate audio)
    theremin.update();

    // Small delay for stability (~50Hz update rate)
    delay(20);
}
