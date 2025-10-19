/*
 * AudioEngine.cpp
 *
 * Implementation of audio synthesis for ESP32 Theremin.
 * Currently uses PWM for square wave generation.
 */

#include "AudioEngine.h"
#include "Debug.h"

// Constructor
AudioEngine::AudioEngine() : currentFrequency(MIN_FREQUENCY), currentAmplitude(0), smoothedAmplitude(0.0), dutyCycle(0) {}

// Initialize audio hardware
void AudioEngine::begin() {
  // Configure PWM channel
  ledcSetup(PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(BUZZER_PIN, PWM_CHANNEL);
  DEBUG_PRINTLN("[AUDIO] PWM configured on GPIO25");
}

// Set frequency
void AudioEngine::setFrequency(int freq) {
  // Constrain to valid range
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

  // Calculate duty cycle based on smoothed amplitude
  calculateDutyCycle();

  // Apply to PWM hardware
  updatePWM();
}

// Map amplitude to duty cycle
void AudioEngine::calculateDutyCycle() {
  // Map smoothed amplitude (0-100) to duty cycle (0-MAX_DUTY_CYCLE)
  // Using smoothedAmplitude instead of currentAmplitude gives smooth transitions
  dutyCycle = map((int)smoothedAmplitude, 0, 100, MIN_DUTY_CYCLE, MAX_DUTY_CYCLE);
  dutyCycle = constrain(dutyCycle, MIN_DUTY_CYCLE, MAX_DUTY_CYCLE);
}

// Apply settings to PWM
void AudioEngine::updatePWM() {
  if (dutyCycle > SILENCE_THRESHOLD) {
    // Generate tone at current frequency with duty cycle
    ledcChangeFrequency(PWM_CHANNEL, currentFrequency, PWM_RESOLUTION);
    ledcWrite(PWM_CHANNEL, dutyCycle);
  } else {
    // Silence - set duty cycle to 0
    ledcWrite(PWM_CHANNEL, 0);
  }
}
