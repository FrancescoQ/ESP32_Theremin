/*
 * Theremin.cpp
 *
 * Implementation of the main Theremin coordinator class.
 * Maps sensor input to audio output.
 */

#include "Theremin.h"
#include "Debug.h"

// Constructor
Theremin::Theremin(PerformanceMonitor* perfMon)
    : audio(perfMon), debugEnabled(false) {}

// Initialize theremin
bool Theremin::begin() {
  DEBUG_PRINTLN("\n=== ESP32 Theremin Initializing ===");

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

// Helper function: floating-point map for smoother frequency transitions
// Eliminates quantization artifacts from integer map() function
float Theremin::mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Main update loop
void Theremin::update() {
  // Update sensor readings (reads both sensors once and caches results)
  sensors.updateReadings();

  // Get smoothed distances from cached readings
  int pitchDistance = sensors.getPitchDistance();
  int volumeDistance = sensors.getVolumeDistance();

  // Map pitch to frequency using floating-point math for smooth transitions
  float frequencyFloat = mapFloat((float)pitchDistance,
                                   (float)SensorManager::PITCH_MIN_DIST,
                                   (float)SensorManager::PITCH_MAX_DIST,
                                   (float)AudioEngine::MAX_FREQUENCY,
                                   (float)AudioEngine::MIN_FREQUENCY);

  // Constrain and convert to integer
  int frequency = constrain((int)frequencyFloat, AudioEngine::MIN_FREQUENCY, AudioEngine::MAX_FREQUENCY);

  // Map volume using integer math (volume doesn't need sub-Hz precision)
  // Traditional theremin behavior: hand NEAR volume antenna = QUIET, hand FAR = LOUD
  int amplitude =
      map(volumeDistance, SensorManager::VOLUME_MIN_DIST, SensorManager::VOLUME_MAX_DIST,
          MIN_AMPLITUDE_PERCENT,     // Min amplitude (closest) - near sensor = quiet
          MAX_AMPLITUDE_PERCENT);    // Max amplitude (farthest) - far from sensor = loud
  amplitude = constrain(amplitude, MIN_AMPLITUDE_PERCENT, MAX_AMPLITUDE_PERCENT);

  // Update audio engine
  audio.setFrequency(frequency);
  audio.setAmplitude(amplitude);

  // This now do nothing, FreeRTOS task handles audio updates
  audio.update();

  // Debug output (throttled to every 10th loop)
  if (debugEnabled) {
    printDebugInfo(pitchDistance, volumeDistance, frequency, amplitude);
  }
}

// Enable/disable debug output
void Theremin::setDebugMode(bool enabled) {
  debugEnabled = enabled;
}

// Print debug information (throttled to avoid flooding serial monitor)
// Uses static local counter to print only every 10th loop iteration (~5 times/second at 50Hz)
void Theremin::printDebugInfo(int pitchDist, int volumeDist, int freq, int amplitude) {
  static unsigned int loopCounter = 0;  // Static local - only increments when debug is enabled

  if (loopCounter % DEBUG_THROTTLE_FACTOR == 0) {  // Print only when counter is divisible by throttle factor
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

  // For future me: if you're worried about overflow, this will wrap around
  // safely, and in any case at 50Hz it should take something like ~994 days
  // to overflow... that's a long song...
  loopCounter++;
}
