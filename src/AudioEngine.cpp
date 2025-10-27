/*
 * AudioEngine.cpp
 *
 * Implementation of audio synthesis for ESP32 Theremin.
 * Uses I2S peripheral in built-in DAC mode for DMA-driven audio output.
 */

#include "AudioEngine.h"
#include "Debug.h"

// Constructor
AudioEngine::AudioEngine() : currentFrequency(MIN_FREQUENCY), currentAmplitude(0), smoothedAmplitude(0.0f) {
}

// Initialize audio hardware
void AudioEngine::begin() {
  DEBUG_PRINTLN("[AUDIO] Initializing I2S with built-in DAC mode");

  // Configure I2S in built-in DAC mode
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
    .sample_rate = I2S_SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,  // GPIO 25 (DAC1)
    .communication_format = I2S_COMM_FORMAT_STAND_MSB,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 2,
    .dma_buf_len = I2S_BUFFER_SIZE,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  // Install I2S driver
  esp_err_t result = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  if (result != ESP_OK) {
    DEBUG_PRINT("[AUDIO] ERROR: I2S driver install failed: ");
    DEBUG_PRINTLN(result);
    return;
  }

  // Enable built-in DAC on GPIO 25 (right channel)
  i2s_set_dac_mode(I2S_DAC_CHANNEL_RIGHT_EN);

  DEBUG_PRINTLN("[AUDIO] Oscillator initialized");
  DEBUG_PRINT("[AUDIO] I2S configured: ");
  DEBUG_PRINT(I2S_SAMPLE_RATE);
  DEBUG_PRINT(" Hz sample rate, ");
  DEBUG_PRINT(I2S_BUFFER_SIZE);
  DEBUG_PRINTLN(" samples per buffer");
  DEBUG_PRINTLN("[AUDIO] DMA-driven continuous audio output enabled");
}

// Set frequency
void AudioEngine::setFrequency(int freq) {
  // Constrain to valid range
  currentFrequency = constrain(freq, MIN_FREQUENCY, MAX_FREQUENCY);

  // Update oscillator frequency
  oscillator.setFrequency((float)currentFrequency);
}

// Set amplitude
void AudioEngine::setAmplitude(int amplitude) {
  // Constrain to 0-100%
  currentAmplitude = constrain(amplitude, 0, 100);
}

// Update audio output (fill buffer and send to I2S)
void AudioEngine::update() {
  // Apply exponential smoothing to amplitude
  smoothedAmplitude += (currentAmplitude - smoothedAmplitude) * SMOOTHING_FACTOR;

  // Fill buffer with samples
  fillBuffer();

  // Send buffer to I2S (DMA handles continuous output)
  size_t bytes_written;
  i2s_write(I2S_NUM_0, audioBuffer, I2S_BUFFER_SIZE * sizeof(uint16_t), &bytes_written, portMAX_DELAY);
}

// Fill buffer with audio samples from oscillator
void AudioEngine::fillBuffer() {
  for (int i = 0; i < I2S_BUFFER_SIZE; i++) {
    // Get next sample from oscillator (-128 to 127)
    int8_t sample = oscillator.getNextSample();

    // Apply amplitude scaling (0-100%)
    int16_t scaledSample = (int16_t)(sample * smoothedAmplitude / 100.0f);

    // Convert to unsigned 8-bit DAC range (0-255), center at 128
    uint8_t dacValue = (uint8_t)(scaledSample + 128);

    // I2S built-in DAC mode expects 16-bit samples
    // Upper 8 bits = DAC value, lower 8 bits = ignored
    audioBuffer[i] = (uint16_t)(dacValue << 8);
  }
}
