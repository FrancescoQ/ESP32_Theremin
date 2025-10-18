/*
 * Theremin.cpp
 *
 * Implementation of the main Theremin coordinator class.
 * Maps sensor input to audio output.
 */

#include "Theremin.h"
#include "Debug.h"

// Constructor
Theremin::Theremin()
    : debugEnabled(true), loopCounter(0) {
}

// Initialize theremin
bool Theremin::begin() {
    DEBUG_PRINTLN("\n=== ESP32 Theremin Initializing ===");

#ifdef WOKWI_SIMULATION
    DEBUG_PRINTLN("[MODE] Simulation (Potentiometers)");
#else
    DEBUG_PRINTLN("[MODE] Hardware (VL53L0X Sensors)");
#endif

    // Initialize sensors
    if (!sensors.begin()) {
        DEBUG_PRINTLN("[ERROR] Sensor initialization failed!");
        return false;
    }

    // Initialize audio
    audio.begin();

    DEBUG_PRINTLN("=== Initialization Complete ===\n");
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
                       0,    // Min amplitude (closest - like real theremin)
                       100); // Max amplitude (farthest - like real theremin)

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
        DEBUG_PRINT("[PITCH] ");
        DEBUG_PRINT(pitchDist);
        DEBUG_PRINT("mm → ");
        DEBUG_PRINT(freq);
        DEBUG_PRINT("Hz  |  [VOLUME] ");
        DEBUG_PRINT(volumeDist);
        DEBUG_PRINT("mm → ");
        DEBUG_PRINT(amplitude);
        DEBUG_PRINTLN("%");
    }
}
