/*
 * Oscillator.cpp
 *
 * Implementation of digital oscillator for ESP32 Theremin.
 * Uses phase accumulator for accurate frequency generation.
 */

#include "Oscillator.h"

// Sine wave lookup table (SINE_TABLE_SIZE entries)
// Pre-calculated sine values for one complete cycle
// Stored in Flash memory (PROGMEM) to save RAM
const int16_t Oscillator::SINE_TABLE[SINE_TABLE_SIZE] PROGMEM = {
  0, 804, 1608, 2410, 3212, 4011, 4808, 5602, 6393, 7179, 7962, 8739, 9512, 10278, 11039, 11793,
  12539, 13279, 14010, 14732, 15446, 16151, 16846, 17530, 18204, 18868, 19519, 20159, 20787, 21403, 22005, 22594,
  23170, 23731, 24279, 24811, 25329, 25832, 26319, 26790, 27245, 27683, 28105, 28510, 28898, 29268, 29621, 29956,
  30273, 30571, 30852, 31113, 31356, 31580, 31785, 31971, 32137, 32285, 32412, 32521, 32609, 32678, 32728, 32757,
  32767, 32757, 32728, 32678, 32609, 32521, 32412, 32285, 32137, 31971, 31785, 31580, 31356, 31113, 30852, 30571,
  30273, 29956, 29621, 29268, 28898, 28510, 28105, 27683, 27245, 26790, 26319, 25832, 25329, 24811, 24279, 23731,
  23170, 22594, 22005, 21403, 20787, 20159, 19519, 18868, 18204, 17530, 16846, 16151, 15446, 14732, 14010, 13279,
  12539, 11793, 11039, 10278, 9512, 8739, 7962, 7179, 6393, 5602, 4808, 4011, 3212, 2410, 1608, 804,
  0, -804, -1608, -2410, -3212, -4011, -4808, -5602, -6393, -7179, -7962, -8739, -9512, -10278, -11039, -11793,
  -12539, -13279, -14010, -14732, -15446, -16151, -16846, -17530, -18204, -18868, -19519, -20159, -20787, -21403, -22005, -22594,
  -23170, -23731, -24279, -24811, -25329, -25832, -26319, -26790, -27245, -27683, -28105, -28510, -28898, -29268, -29621, -29956,
  -30273, -30571, -30852, -31113, -31356, -31580, -31785, -31971, -32137, -32285, -32412, -32521, -32609, -32678, -32728, -32757,
  -32767, -32757, -32728, -32678, -32609, -32521, -32412, -32285, -32137, -31971, -31785, -31580, -31356, -31113, -30852, -30571,
  -30273, -29956, -29621, -29268, -28898, -28510, -28105, -27683, -27245, -26790, -26319, -25832, -25329, -24811, -24279, -23731,
  -23170, -22594, -22005, -21403, -20787, -20159, -19519, -18868, -18204, -17530, -16846, -16151, -15446, -14732, -14010, -13279,
  -12539, -11793, -11039, -10278, -9512, -8739, -7962, -7179, -6393, -5602, -4808, -4011, -3212, -2410, -1608, -804
};

// Constructor
Oscillator::Oscillator() : phase(0.0f), frequency(440.0f), waveform(SQUARE), octaveShift(0), volume(1.0f) {}

// Set frequency
void Oscillator::setFrequency(float freq) {
  // Constrain to reasonable range to prevent issues
  // Minimum 0.1 Hz allows LFO use (10 second cycle)
  // Maximum 20 kHz covers audio range
  frequency = constrain(freq, 0.1f, 20000.0f);
}

// Set waveform
void Oscillator::setWaveform(Waveform wf) {
  waveform = wf;
}

// Set octave shift
void Oscillator::setOctaveShift(int shift) {
  // Constrain to OCTAVE_DOWN, OCTAVE_BASE, OCTAVE_UP
  octaveShift = constrain(shift, OCTAVE_DOWN, OCTAVE_UP);
}

// Set volume
void Oscillator::setVolume(float vol) {
  // Constrain to 0.0-1.0 range
  volume = constrain(vol, 0.0f, 1.0f);
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

    case SINE:
      sample = generateSineWave();
      break;

    case TRIANGLE:
      sample = generateTriangleWave();
      break;

    case SAW:
      sample = generateSawtoothWave();
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
  float effectiveFreq = getEffectiveFrequency();
  float phaseIncrement = effectiveFreq / sampleRate;

  phase += phaseIncrement;

  // Wrap phase to stay in [0.0, 1.0] range
  if (phase >= 1.0f) {
    phase -= 1.0f;
  }

  // Apply volume control before returning
  return (int16_t)(sample * volume);
}

// Get effective frequency (with octave shift)
float Oscillator::getEffectiveFrequency() const {
  return calculateShiftedFrequency();
}

float Oscillator::getNextSampleNormalized(float sampleRate) {
  int16_t sample = getNextSample(sampleRate);
  return sample / 32768.0f;  // Convert int16_t to float -1.0 to 1.0
}

// Calculate frequency with octave shift applied
float Oscillator::calculateShiftedFrequency() const {
  switch (octaveShift) {
    case -1:
      return frequency / OCTAVE_MULTIPLIER;  // One octave down (half frequency)
    case 1:
      return frequency * OCTAVE_MULTIPLIER;  // One octave up (double frequency)
    case 0:
    default:
      return frequency;                      // No shift
  }
}

// Generate square wave sample
int16_t Oscillator::generateSquareWave() const {
  // Simple square wave: compare phase to half cycle
  // First half of cycle: maximum value
  // Second half of cycle: minimum value

  if (phase < PHASE_HALF_CYCLE) {
    return Audio::SAMPLE_MAX;  // Maximum positive value (16-bit)
  } else {
    return Audio::SAMPLE_MIN;  // Maximum negative value (16-bit)
  }
}

// Generate sine wave sample
int16_t Oscillator::generateSineWave() const {
  // Convert phase (0.0 - 1.0) to table index (0 - 255)
  uint8_t index = (uint8_t)(phase * (float)SINE_TABLE_SIZE);

  // Read from Flash memory lookup table
  return pgm_read_word(&SINE_TABLE[index]);
}

// Generate triangle wave sample
int16_t Oscillator::generateTriangleWave() const {
  // Triangle wave: linear rise and fall
  // Phase 0.0 - 0.5: rising from SAMPLE_MIN to SAMPLE_MAX
  // Phase 0.5 - 1.0: falling from SAMPLE_MAX to SAMPLE_MIN

  int16_t sample;

  if (phase < PHASE_HALF_CYCLE) {
    // Rising edge: map 0.0-0.5 to SAMPLE_MIN to SAMPLE_MAX
    sample = (int16_t)((phase * OCTAVE_MULTIPLIER * (float)Audio::SAMPLE_RANGE) - (float)Audio::SAMPLE_MAX - 1.0f);
  } else {
    // Falling edge: map 0.5-1.0 to SAMPLE_MAX to SAMPLE_MIN
    sample = (int16_t)(((1.0f - phase) * OCTAVE_MULTIPLIER * (float)Audio::SAMPLE_RANGE) - (float)Audio::SAMPLE_MAX - 1.0f);
  }

  return sample;
}

// Generate sawtooth wave sample
int16_t Oscillator::generateSawtoothWave() const {
  // Sawtooth wave: linear rise from SAMPLE_MIN to SAMPLE_MAX
  // Simplest waveform - direct linear mapping of phase to amplitude
  // Phase 0.0 → SAMPLE_MIN, Phase 1.0 → SAMPLE_MAX

  return (int16_t)((phase * (float)Audio::SAMPLE_RANGE) - (float)Audio::SAMPLE_MAX - 1.0f);
}
