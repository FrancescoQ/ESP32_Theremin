/*
 * Oscillator.h
 *
 * Digital oscillator for ESP32 Theremin.
 * Implements phase accumulator with waveform generation.
 * Currently supports square wave (simplest for initial implementation).
 * Designed for easy expansion to sine, sawtooth, triangle waveforms.
 */

#pragma once
#include <Arduino.h>

class Oscillator {
 public:
  /**
   * Waveform types
   * OFF = no output, saves CPU when oscillator not needed
   */
  enum Waveform {
    OFF = 0,
    SQUARE = 1,
    // Future waveforms:
    // SINE = 2,
    // SAW = 3,
    // TRIANGLE = 4
  };

  /**
   * Constructor
   */
  Oscillator();

  /**
   * Set oscillator frequency in Hz
   * @param freq Frequency in Hz (e.g., 220-880 for A3-A5)
   */
  void setFrequency(float freq);

  /**
   * Set waveform type
   * @param wf Waveform (currently only OFF or SQUARE)
   */
  void setWaveform(Waveform wf);

  /**
   * Set octave shift (-1, 0, +1)
   * @param shift Octave shift (-1 = half freq, 0 = normal, +1 = double freq)
   */
  void setOctaveShift(int shift);

  /**
   * Generate next audio sample
   * @param sampleRate Sample rate in Hz (e.g., 22050)
   * @return 16-bit signed audio sample (-32768 to 32767)
   */
  int16_t getNextSample(float sampleRate);

  /**
   * Check if oscillator is active (not OFF)
   * @return true if oscillator is generating audio
   */
  bool isActive() const {
    return waveform != OFF;
  }

  /**
   * Get current frequency (with octave shift applied)
   * @return Effective frequency in Hz
   */
  float getEffectiveFrequency() const;

 private:
  // Oscillator state
  float phase;        // Phase accumulator (0.0 - 1.0)
  float frequency;    // Base frequency in Hz
  Waveform waveform;  // Current waveform
  int octaveShift;    // Octave shift (-1, 0, +1)

  /**
   * Calculate frequency with octave shift applied
   */
  float calculateShiftedFrequency() const;

  /**
   * Generate square wave sample
   * Simple phase comparison, no LUT needed
   */
  int16_t generateSquareWave() const;
};
