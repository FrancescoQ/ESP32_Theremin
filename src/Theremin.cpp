/*
 * Theremin.cpp
 *
 * Implementation of the main Theremin coordinator class.
 * Maps sensor input to audio output.
 */

#include "Theremin.h"

// Constructor
Theremin::Theremin()
    : debugEnabled(true), loopCounter(0) {
}

// Initialize theremin
bool Theremin::begin() {
    Serial.println("\n=== ESP32 Theremin Initializing ===");

#ifdef WOKWI_SIMULATION
    Serial.println("[MODE] Simulation (Potentiometers)");
#else
    Serial.println("[MODE] Hardware (VL53L0X Sensors)");
#endif

    // Initialize sensors
    if (!sensors.begin()) {
        Serial.println("[ERROR] Sensor initialization failed!");
        return false;
    }

    // Initialize audio
    audio.begin();

    Serial.println("=== Initialization Complete ===\n");
    return true;
}

// Main update loop
void Theremin::update() {
    // Get sensor readings
    int pitchDistance = sensors.getPitchDistance();
    int volumeDistance = sensors.getVolumeDistance();

    // Map to audio parameters (inverse mapping: closer = higher/louder)
    int frequency = map(pitchDistance,
                       SensorManager::PITCH_MIN_DIST,
                       SensorManager::PITCH_MAX_DIST,
                       AudioEngine::MAX_FREQUENCY,
                       AudioEngine::MIN_FREQUENCY);

    int amplitude = map(volumeDistance,
                       SensorManager::VOLUME_MIN_DIST,
                       SensorManager::VOLUME_MAX_DIST,
                       100,  // Max amplitude (closest)
                       0);   // Min amplitude (farthest)

    // Constrain values
    frequency = constrain(frequency, AudioEngine::MIN_FREQUENCY, AudioEngine::MAX_FREQUENCY);
    amplitude = constrain(amplitude, 0, 100);

    // Update audio engine
    audio.setFrequency(frequency);
    audio.setAmplitude(amplitude);
    audio.update();

    // Debug output (throttled to every 10th loop)
    if (debugEnabled) {
        printDebugInfo(pitchDistance, volumeDistance, frequency, amplitude);
    }

    loopCounter++;
}

// Enable/disable debug output
void Theremin::setDebugMode(bool enabled) {
    debugEnabled = enabled;
}

// Print debug information
void Theremin::printDebugInfo(int pitchDist, int volumeDist, int freq, int amplitude) {
    if (loopCounter % 10 == 0) {
        Serial.print("[PITCH] ");
        Serial.print(pitchDist);
        Serial.print("mm → ");
        Serial.print(freq);
        Serial.print("Hz  |  [VOLUME] ");
        Serial.print(volumeDist);
        Serial.print("mm → ");
        Serial.print(amplitude);
        Serial.println("%");
    }
}
