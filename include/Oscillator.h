/*
 * Oscillator.h
 *
 * Wavetable-based oscillator for ESP32 Theremin.
 * Generates audio samples using pre-computed lookup tables.
 *
 * Phase 2: Sine wave only (256-sample LUT)
 * Phase 3+: Will add SQUARE (algorithmic), SAW (512-sample LUT), OFF
 */

#pragma once
#include <Arduino.h>

// Audio generation configuration
// Easily adjustable - just change and recompile
constexpr uint32_t SAMPLE_RATE = 22050;      // Hz (22050 matches I2S rate)
constexpr uint16_t WAVETABLE_SIZE = 256;     // Samples (256 or 512)

// Derived values (auto-calculated)
constexpr float PHASE_INCREMENT_SCALE = 1.0f / SAMPLE_RATE;
constexpr float WAVETABLE_INDEX_SCALE = (float)WAVETABLE_SIZE;

// Frequency range for theremin (2 octaves: A3 to A5)
constexpr float MIN_FREQUENCY = 220.0f;   // A3
constexpr float MAX_FREQUENCY = 880.0f;   // A5

class Oscillator {
public:
  /**
   * Constructor - initializes wavetable
   */
  Oscillator();

  /**
   * Set oscillator frequency in Hz
   * @param freq Frequency (will be constrained to MIN_FREQUENCY - MAX_FREQUENCY)
   */
  void setFrequency(float freq);

  /**
   * Get next audio sample (must be in IRAM for ISR)
   * @return int8_t sample value (-128 to 127)
   */
  int8_t IRAM_ATTR getNextSample();

  /**
   * Reset phase to zero (for sync/testing)
   */
  void reset();

  /**
   * Get current frequency
   */
  float getFrequency() const {
    return frequency;
  }

private:
  // Static wavetable shared by all oscillators (explicitly in DRAM for ISR safety)
  static DRAM_ATTR int8_t sineTable[WAVETABLE_SIZE];
  static bool tableGenerated;  // Track if table is initialized

  // Oscillator state (static for ISR access, limits to single oscillator for Phase 2)
  static DRAM_ATTR float phase;        // Current phase (0.0 - 1.0)
  static DRAM_ATTR float frequency;    // Current frequency in Hz

  /**
   * Generate sine wavetable (called in constructor, only once)
   */
  void generateSineTable();
};
