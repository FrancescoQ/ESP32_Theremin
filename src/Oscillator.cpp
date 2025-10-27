/*
 * Oscillator.cpp
 *
 * Implementation of digital oscillator for ESP32 Theremin.
 * Uses phase accumulator for accurate frequency generation.
 */

#include "Oscillator.h"

// Constructor
Oscillator::Oscillator() : phase(0.0f), frequency(440.0f), waveform(SQUARE), octaveShift(0) {}

// Set frequency
void Oscillator::setFrequency(float freq) {
  // Constrain to reasonable range to prevent issues
  frequency = constrain(freq, 20.0f, 20000.0f);
}

// Set waveform
void Oscillator::setWaveform(Waveform wf) {
  waveform = wf;
}

// Set octave shift
void Oscillator::setOctaveShift(int shift) {
  // Constrain to -1, 0, +1
  octaveShift = constrain(shift, -1, 1);
}

// Get next audio sample
int16_t Oscillator::getNextSample(float sampleRate) {
  // Early return if oscillator is OFF (saves CPU)
  if (waveform == OFF) {
    return 0;
  }

  // Generate sample based on waveform
  int16_t sample = 0;

  switch (waveform) {
    case SQUARE:
      sample = generateSquareWave();
      break;

    case OFF:
      // Already handled above, but include for completeness
      sample = 0;
      break;

    default:
      // Unknown waveform, output silence
      sample = 0;
      break;
  }

  // Advance phase accumulator
  // Phase increment = frequency / sample rate
  float effectiveFreq = calculateShiftedFrequency();
  float phaseIncrement = effectiveFreq / sampleRate;

  phase += phaseIncrement;

  // Wrap phase to stay in [0.0, 1.0] range
  if (phase >= 1.0f) {
    phase -= 1.0f;
  }

  return sample;
}

// Get effective frequency (with octave shift)
float Oscillator::getEffectiveFrequency() const {
  return calculateShiftedFrequency();
}

// Calculate frequency with octave shift applied
float Oscillator::calculateShiftedFrequency() const {
  switch (octaveShift) {
    case -1:
      return frequency * 0.5f;  // One octave down (half frequency)
    case 1:
      return frequency * 2.0f;  // One octave up (double frequency)
    case 0:
    default:
      return frequency;         // No shift
  }
}

// Generate square wave sample
int16_t Oscillator::generateSquareWave() const {
  // Simple square wave: compare phase to 0.5
  // First half of cycle: maximum value
  // Second half of cycle: minimum value

  if (phase < 0.5f) {
    return 32767;   // Maximum positive value (16-bit)
  } else {
    return -32768;  // Maximum negative value (16-bit)
  }
}
