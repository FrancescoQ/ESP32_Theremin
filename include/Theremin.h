/*
 * Theremin.h
 *
 * Main coordinator class for the ESP32 Theremin.
 * Manages sensors and audio engine, mapping sensor input to sound output.
 */

#pragma once
#include "SensorManager.h"
#include "AudioEngine.h"

// Forward declaration
class PerformanceMonitor;

class Theremin {
 public:
  /**
   * Constructor
   * @param perfMon Pointer to PerformanceMonitor instance (optional)
   */
  Theremin(PerformanceMonitor* perfMon = nullptr);

  /**
   * Initialize theremin (sensors and audio)
   * Must be called in setup()
   * @return true if successful, false if initialization fails
   */
  bool begin();

  /**
   * Main update loop - read sensors and generate audio
   * Call this from loop()
   */
  void update();

  /**
   * Enable or disable debug output
   */
  void setDebugMode(bool enabled);

  /**
   * Get pointer to AudioEngine instance (for direct control access)
   * @return Pointer to AudioEngine
   */
  AudioEngine* getAudioEngine() { return &audio; }

  /**
   * Get pointer to SensorManager instance (for control access)
   * @return Pointer to SensorManager
   */
  SensorManager* getSensorManager() { return &sensors; }

 private:
  SensorManager sensors;
  AudioEngine audio;
  bool debugEnabled;

  // Amplitude range constants (internal use only)
  static const int MIN_AMPLITUDE_PERCENT = 0;
  static const int MAX_AMPLITUDE_PERCENT = 100;

  // Debug output throttling (internal use only)
  static const int DEBUG_THROTTLE_FACTOR = 10;  // Print every Nth loop iteration

  /**
   * Map sensor distances to audio parameters
   * Closer distance = higher frequency and louder volume
   */
  void mapSensorsToAudio();

  /**
   * Floating-point map function for smoother frequency transitions
   * Eliminates quantization artifacts from integer map() function
   */
  float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);

  /**
   * Print debug information to serial
   */
  void printDebugInfo(int pitchDist, int volumeDist, int freq, int amplitude);
};
