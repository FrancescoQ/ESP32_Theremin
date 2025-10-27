/*
 * AudioEngine.h
 *
 * Manages audio synthesis for the ESP32 Theremin.
 * Uses I2S in built-in DAC mode for audio output (GPIO25).
 * Designed for modular oscillator architecture.
 */

#pragma once
#include <Arduino.h>
#include <driver/i2s.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "PinConfig.h"
#include "Oscillator.h"

class AudioEngine {
 public:
  /**
   * Constructor
   */
  AudioEngine();

  /**
   * Initialize audio hardware (must be called in setup())
   */
  void begin();

  /**
   * Set the audio frequency in Hz
   * @param freq Frequency in Hz (will be constrained to MIN_FREQUENCY-MAX_FREQUENCY)
   */
  void setFrequency(int freq);

  /**
   * Set the audio amplitude (0-100%)
   * @param amplitude Amplitude percentage (0-100)
   */
  void setAmplitude(int amplitude);

  /**
   * Update audio output based on current frequency and amplitude
   * DEPRECATED: No longer needed with continuous audio task
   * Kept for backward compatibility but does nothing
   */
  void update();

  /**
   * Start continuous audio generation task
   * Called automatically by begin()
   */
  void startAudioTask();

  /**
   * Stop continuous audio generation task
   * Call before destroying AudioEngine instance
   */
  void stopAudioTask();

  /**
   * Get current frequency
   */
  int getFrequency() const {
    return currentFrequency;
  }

  /**
   * Get current amplitude
   */
  int getAmplitude() const {
    return currentAmplitude;
  }

  // Audio range constants (A3 to A5, 2 octaves)
  static const int MIN_FREQUENCY = 220;  // A3
  static const int MAX_FREQUENCY = 880;  // A5

 private:
  // I2S configuration
  static const int I2S_NUM = 0;                // I2S port number
  static const int SAMPLE_RATE = 22050;        // Sample rate in Hz
  static const int BUFFER_SIZE = 256;          // Samples per buffer
  static const int DMA_BUFFER_COUNT = 2;       // Number of DMA buffers

  // Amplitude smoothing (prevents sudden volume jumps)
  // Tuning guide - adjust to taste:
  //   0.05 = ~2.0s fade time (very smooth, laggy)
  //   0.10 = ~1.2s fade time (smooth, professional)
  //   0.15 = ~0.8s fade time (balanced - current setting)
  //   0.25 = ~0.5s fade time (responsive, slight smoothing)
  //   0.50 = ~0.2s fade time (minimal smoothing)
  //   1.00 = instant (no smoothing, like before)
  static constexpr float SMOOTHING_FACTOR = 0.15;  // 0.0-1.0 (lower = smoother)

  // Current state
  int currentFrequency;
  int currentAmplitude;     // Target amplitude
  float smoothedAmplitude;  // Actual smoothed amplitude value

  // Oscillator instance
  Oscillator oscillator;
  Oscillator oscillator2;
  Oscillator oscillator3;

  // FreeRTOS task management
  TaskHandle_t audioTaskHandle;
  SemaphoreHandle_t paramMutex;  // Protects frequency/amplitude updates
  volatile bool taskRunning;

  /**
   * Initialize I2S in built-in DAC mode
   * @return true if initialization succeeded, false otherwise
   */
  bool setupI2S();

  /**
   * Generate audio buffer and write to I2S
   * Called continuously by audio task
   */
  void generateAudioBuffer();

  /**
   * Static wrapper for FreeRTOS task
   * Required because FreeRTOS expects static function
   */
  static void audioTaskFunction(void* parameter);

  /**
   * Instance method called by task
   * Continuously generates audio buffers
   */
  void audioTaskLoop();
};
