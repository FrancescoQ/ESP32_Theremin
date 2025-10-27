/*
 * AudioEngine.cpp
 *
 * Implementation of audio synthesis for ESP32 Theremin.
 * Uses I2S in built-in DAC mode for high-quality audio output.
 */

#include "AudioEngine.h"
#include "Debug.h"

// Constructor
AudioEngine::AudioEngine() : currentFrequency(MIN_FREQUENCY), currentAmplitude(0), smoothedAmplitude(0.0) {
  // Initialize oscillator with square wave
  oscillator.setWaveform(Oscillator::SQUARE);
  oscillator.setOctaveShift(0);  // No octave shift by default
}

// Initialize audio hardware
void AudioEngine::begin() {
  setupI2S();
  DEBUG_PRINTLN("[AUDIO] I2S DAC initialized on GPIO25 @ 22050 Hz");
}

// Set frequency
void AudioEngine::setFrequency(int freq) {
  // Constrain to valid range (A3-A5)
  currentFrequency = constrain(freq, MIN_FREQUENCY, MAX_FREQUENCY);

  // Update oscillator frequency
  oscillator.setFrequency((float)currentFrequency);
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

  // Generate and write audio buffer to I2S
  generateAudioBuffer();
}

// Initialize I2S in built-in DAC mode
void AudioEngine::setupI2S() {
  // I2S configuration for built-in DAC
  i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
      .sample_rate = SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,  // Mono output on DAC1 (GPIO25)
      .communication_format = I2S_COMM_FORMAT_STAND_MSB,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = DMA_BUFFER_COUNT,
      .dma_buf_len = BUFFER_SIZE,
      .use_apll = false,
      .tx_desc_auto_clear = true,
      .fixed_mclk = 0
  };

  // Install I2S driver
  esp_err_t err = i2s_driver_install((i2s_port_t)I2S_NUM, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
    DEBUG_PRINT("[AUDIO] I2S driver install failed: ");
    DEBUG_PRINTLN(err);
    return;
  }

  // Set I2S pins for built-in DAC mode (GPIO25 = DAC1)
  i2s_set_dac_mode(I2S_DAC_CHANNEL_RIGHT_EN);  // Enable DAC on GPIO25
}

// Generate audio buffer and write to I2S
void AudioEngine::generateAudioBuffer() {
  int16_t buffer[BUFFER_SIZE];

  // Calculate gain from smoothed amplitude (0-100% → 0.0-1.0)
  float gain = smoothedAmplitude / 100.0f;

  // Generate audio samples
  for (int i = 0; i < BUFFER_SIZE; i++) {
    // Get full-scale sample from oscillator
    int16_t sample = oscillator.getNextSample((float)SAMPLE_RATE);

    // Apply amplitude scaling
    buffer[i] = (int16_t)(sample * gain);
  }

  // Write buffer to I2S
  size_t bytes_written = 0;
  i2s_write((i2s_port_t)I2S_NUM, buffer, BUFFER_SIZE * sizeof(int16_t), &bytes_written, portMAX_DELAY);
}
