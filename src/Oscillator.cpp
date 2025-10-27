/*
 * Oscillator.cpp
 *
 * Implementation of wavetable oscillator for ESP32 Theremin.
 */

#include "Oscillator.h"
#include "Debug.h"
#include <math.h>

// Define static class members
DRAM_ATTR int8_t Oscillator::sineTable[WAVETABLE_SIZE];
bool Oscillator::tableGenerated = false;
DRAM_ATTR float Oscillator::phase = 0.0f;
DRAM_ATTR float Oscillator::frequency = MIN_FREQUENCY;

// Constructor
Oscillator::Oscillator() {
  // Only generate table once (first oscillator instance)
  if (!tableGenerated) {
    generateSineTable();
    tableGenerated = true;
    DEBUG_PRINTLN("[OSCILLATOR] Sine wavetable generated (256 samples)");
  }
}

// Set frequency
void Oscillator::setFrequency(float freq) {
  // Constrain to valid range
  frequency = constrain(freq, MIN_FREQUENCY, MAX_FREQUENCY);
}

// Get next sample from wavetable
int8_t IRAM_ATTR Oscillator::getNextSample() {
  // Calculate wavetable index from current phase
  int index = (int)(phase * WAVETABLE_INDEX_SCALE);

  // Ensure index is within bounds (safety check)
  index = index % WAVETABLE_SIZE;

  // Get sample from wavetable
  int8_t sample = sineTable[index];

  // Advance phase for next sample
  phase += frequency * PHASE_INCREMENT_SCALE;

  // Wrap phase to 0.0-1.0 range
  if (phase >= 1.0f) {
    phase -= 1.0f;
  }

  return sample;
}

// Reset phase to zero
void Oscillator::reset() {
  phase = 0.0f;
}

// Generate sine wavetable
void Oscillator::generateSineTable() {
  for (int i = 0; i < WAVETABLE_SIZE; i++) {
    // Generate sine wave: sin(2π * i / WAVETABLE_SIZE)
    // Map from [-1.0, 1.0] to [-127, 127] for 8-bit DAC
    float angle = 2.0f * PI * i / WAVETABLE_SIZE;
    float value = sin(angle) * 127.0f;
    sineTable[i] = (int8_t)value;
  }
}
