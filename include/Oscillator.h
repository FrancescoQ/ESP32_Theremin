/*
 * Oscillator.h
 *
 * Digital oscillator for ESP32 Theremin.
 * Implements phase accumulator with waveform generation.
 * Supports square, sine, triangle, and sawtooth waveforms.
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
    SINE = 2,
    TRIANGLE = 3,
    SAW = 4
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
   * Set oscillator volume (0.0 - 1.0)
   * @param vol Volume level (0.0 = silent, 1.0 = full volume)
   */
  void setVolume(float vol);

  // Octave shift constants
  static constexpr int OCTAVE_DOWN = -1;  // One octave down (half frequency)
  static constexpr int OCTAVE_BASE = 0;   // No shift (base frequency)
  static constexpr int OCTAVE_UP = 1;     // One octave up (double frequency)

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
   * Get current waveform type
   * @return Current waveform
   */
  Waveform getWaveform() const {
    return waveform;
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
  float volume;       // Volume level (0.0 - 1.0)

  /**
   * Calculate frequency with octave shift applied
   */
  float calculateShiftedFrequency() const;

  /**
   * Generate square wave sample
   * Simple phase comparison, no LUT needed
   */
  int16_t generateSquareWave() const;

  /**
   * Generate sine wave sample
   * Uses lookup table for efficiency
   */
  int16_t generateSineWave() const;

  /**
   * Generate triangle wave sample
   * Mathematical generation, no LUT needed
   */
  int16_t generateTriangleWave() const;

  /**
   * Generate sawtooth wave sample
   * Mathematical generation, no LUT needed
   */
  int16_t generateSawtoothWave() const;

  // Sine wave lookup table (256 entries)
  // Stored in Flash memory to save RAM
  static const int16_t SINE_TABLE[256] PROGMEM;

  // Audio sample constants (16-bit signed audio)
  static constexpr int16_t SAMPLE_MAX = 32767;
  static constexpr int16_t SAMPLE_MIN = -32768;
  static constexpr uint32_t SAMPLE_RANGE = 65535;
  static constexpr uint16_t SINE_TABLE_SIZE = 256;
  static constexpr float PHASE_HALF_CYCLE = 0.5f;
  static constexpr float OCTAVE_MULTIPLIER = 2.0f;
};
