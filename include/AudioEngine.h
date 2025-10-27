/*
 * AudioEngine.h
 *
 * Manages audio synthesis for the ESP32 Theremin.
 * Uses ESP32 internal DAC with I2S peripheral for DMA-driven audio output.
 */

#pragma once
#include <Arduino.h>
#include <driver/i2s.h>
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
   * Call this after setting frequency/amplitude to apply changes
   */
  void update();

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

  // Audio range constants
  static const int MIN_FREQUENCY = 220;  // A3
  static const int MAX_FREQUENCY = 880;  // A5

private:
  // Oscillator instance
  Oscillator oscillator;

  // Amplitude smoothing (prevents sudden volume jumps)
  static constexpr float SMOOTHING_FACTOR = 0.15;  // 0.0-1.0 (lower = smoother)

  // Current state
  int currentFrequency;
  int currentAmplitude;      // Target amplitude (0-100%)
  float smoothedAmplitude;   // Actual smoothed amplitude value

  // I2S configuration
  static const int I2S_SAMPLE_RATE = 22050;   // 22.05 kHz
  static const int I2S_BUFFER_SIZE = 512;      // Samples per buffer
  uint16_t audioBuffer[I2S_BUFFER_SIZE];       // I2S requires 16-bit samples

  /**
   * Fill buffer with audio samples from oscillator
   */
  void fillBuffer();
};
