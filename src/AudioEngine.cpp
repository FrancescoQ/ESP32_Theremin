/*
 * AudioEngine.cpp
 *
 * Implementation of audio synthesis for ESP32 Theremin.
 * PWM code removed - ready for I2S DAC implementation.
 */

#include "AudioEngine.h"
#include "Debug.h"

// Constructor
AudioEngine::AudioEngine() : currentFrequency(MIN_FREQUENCY), currentAmplitude(0), smoothedAmplitude(0.0) {}

// Initialize audio hardware
void AudioEngine::begin() {
  // TODO: Initialize I2S in built-in DAC mode
  DEBUG_PRINTLN("[AUDIO] AudioEngine initialized (no output yet - awaiting I2S implementation)");
}

// Set frequency
void AudioEngine::setFrequency(int freq) {
  // Constrain to valid range (A3-A5)
  currentFrequency = constrain(freq, MIN_FREQUENCY, MAX_FREQUENCY);
}

// Set amplitude
void AudioEngine::setAmplitude(int amplitude) {
  // Constrain to 0-100%
  currentAmplitude = constrain(amplitude, 0, 100);
}

// Update audio output
void AudioEngine::update() {
  // Apply exponential smoothing to amplitude
  // This creates smooth fade-in/fade-out instead of sudden volume jumps
  smoothedAmplitude += (currentAmplitude - smoothedAmplitude) * SMOOTHING_FACTOR;

  // TODO: Generate audio samples and write to I2S buffer
}
