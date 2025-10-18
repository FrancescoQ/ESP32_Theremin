/*
 * AudioEngine.h
 *
 * Manages audio synthesis for the ESP32 Theremin.
 * Currently implements PWM-based square wave generation.
 * Designed to be extensible for future DAC and waveform support.
 */

#pragma once
#include <Arduino.h>

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
  static const int MIN_FREQUENCY = 100;
  static const int MAX_FREQUENCY = 2000;

 private:
  // PWM configuration
  static const int BUZZER_PIN = 25;
  static const int PWM_CHANNEL = 0;
  static const int PWM_RESOLUTION = 8;
  static const int PWM_FREQUENCY = 2000;

  // Duty cycle range (0-255 for 8-bit PWM)
  static const int MIN_DUTY_CYCLE = 0;
  static const int MAX_DUTY_CYCLE = 128;   // 50% max for square wave
  static const int SILENCE_THRESHOLD = 5;  // Below this, output silence

  // Current state
  int currentFrequency;
  int currentAmplitude;
  int dutyCycle;

  /**
   * Map amplitude percentage to PWM duty cycle
   */
  void calculateDutyCycle();

  /**
   * Apply current settings to PWM hardware
   */
  void updatePWM();
};
