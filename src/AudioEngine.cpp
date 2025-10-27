/*
 * AudioEngine.cpp
 *
 * Implementation of audio synthesis for ESP32 Theremin.
 * Uses I2S in built-in DAC mode for high-quality audio output.
 */

#include "AudioEngine.h"
#include "Debug.h"

// Constructor
AudioEngine::AudioEngine() : currentFrequency(MIN_FREQUENCY), currentAmplitude(0), smoothedAmplitude(0.0), audioTaskHandle(NULL), paramMutex(NULL), taskRunning(false) {
  // Initialize oscillator with square wave
  oscillator.setWaveform(Oscillator::SINE);
  oscillator.setOctaveShift(0);  // No octave shift by default

  // Create mutex for thread-safe parameter updates
  paramMutex = xSemaphoreCreateMutex();
}

// Initialize audio hardware
void AudioEngine::begin() {
  // Delay to ensure Serial is fully initialized
  delay(100);

  DEBUG_PRINTLN("[AUDIO] Initializing I2S DAC...");

  if (!setupI2S()) {
    DEBUG_PRINTLN("[AUDIO] ERROR: I2S initialization failed!");
    return;  // Don't start audio task if I2S failed
  }

  DEBUG_PRINTLN("[AUDIO] I2S DAC initialized on GPIO25 @ 22050 Hz");

  // Start continuous audio generation task only if I2S succeeded
  startAudioTask();
}

// Set frequency (thread-safe)
void AudioEngine::setFrequency(int freq) {
  if (paramMutex != NULL && xSemaphoreTake(paramMutex, portMAX_DELAY) == pdTRUE) {
    // Constrain to valid range (A3-A5)
    currentFrequency = constrain(freq, MIN_FREQUENCY, MAX_FREQUENCY);

    // Update oscillator frequency
    oscillator.setFrequency((float)currentFrequency);

    xSemaphoreGive(paramMutex);
  }
}

// Set amplitude (thread-safe)
void AudioEngine::setAmplitude(int amplitude) {
  if (paramMutex != NULL && xSemaphoreTake(paramMutex, portMAX_DELAY) == pdTRUE) {
    // Constrain to 0-100%
    currentAmplitude = constrain(amplitude, 0, 100);

    xSemaphoreGive(paramMutex);
  }
}

// Update audio output
// DEPRECATED: No longer needed with continuous audio task
void AudioEngine::update() {
  // This function is now a no-op
  // Audio generation happens continuously in background task
  // Kept for backward compatibility
}

// Initialize I2S in built-in DAC mode
bool AudioEngine::setupI2S() {
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
    DEBUG_PRINT("[AUDIO] ERROR: I2S driver install failed with error code: ");
    DEBUG_PRINTLN(err);
    return false;
  }

  // Set I2S pins for built-in DAC mode (GPIO25 = DAC1)
  err = i2s_set_dac_mode(I2S_DAC_CHANNEL_RIGHT_EN);
  if (err != ESP_OK) {
    DEBUG_PRINT("[AUDIO] ERROR: I2S DAC mode setup failed with error code: ");
    DEBUG_PRINTLN(err);
    return false;
  }

  return true;
}

// Generate audio buffer and write to I2S
void AudioEngine::generateAudioBuffer() {
  // ESP32 built-in DAC expects unsigned 8-bit samples (0-255)
  // We use uint16_t buffer for proper I2S DMA alignment (2 bytes per sample)
  // Only the upper byte is used by the DAC
  uint16_t buffer[BUFFER_SIZE];

  // Lock mutex to safely read parameters
  if (paramMutex != NULL && xSemaphoreTake(paramMutex, 0) == pdTRUE) {
    // Apply exponential smoothing to amplitude
    smoothedAmplitude += (currentAmplitude - smoothedAmplitude) * SMOOTHING_FACTOR;

    xSemaphoreGive(paramMutex);
  }

  // Calculate gain from smoothed amplitude (0-100% → 0.0-1.0)
  float gain = smoothedAmplitude / 100.0f;

  // Generate audio samples
  for (int i = 0; i < BUFFER_SIZE; i++) {
    // Get full-scale sample from oscillator (-32768 to 32767)
    int16_t sample = oscillator.getNextSample((float)SAMPLE_RATE);

    // Apply amplitude scaling
    int16_t scaledSample = (int16_t)(sample * gain);

    // Convert signed 16-bit to unsigned 8-bit for DAC:
    // 1. Take upper 8 bits (>> 8): -32768..32767 → -128..127
    // 2. Add 128 for DC offset: -128..127 → 0..255
    // 3. Store in upper byte of 16-bit word (DAC reads upper byte)
    uint8_t dacSample = (uint8_t)((scaledSample >> 8) + 128);
    buffer[i] = ((uint16_t)dacSample) << 8;  // Put 8-bit sample in upper byte
  }

  // Write buffer to I2S (blocks until DMA buffer available)
  size_t bytes_written = 0;
  i2s_write((i2s_port_t)I2S_NUM, buffer, BUFFER_SIZE * sizeof(uint16_t), &bytes_written, portMAX_DELAY);
}

// Start continuous audio generation task
void AudioEngine::startAudioTask() {
  if (taskRunning) {
    DEBUG_PRINTLN("[AUDIO] Task already running");
    return;
  }

  taskRunning = true;

  // Create high-priority task on Core 1 (app core)
  // Stack: 4096 bytes, Priority: 2 (higher than default 1), Core: 1
  xTaskCreatePinnedToCore(
      audioTaskFunction,      // Task function
      "AudioTask",            // Task name
      4096,                   // Stack size (bytes)
      this,                   // Parameter (this pointer)
      2,                      // Priority (higher = more important)
      &audioTaskHandle,       // Task handle
      1                       // Core ID (1 = app core)
  );

  DEBUG_PRINTLN("[AUDIO] Continuous audio task started on Core 1");
}

// Stop continuous audio generation task
void AudioEngine::stopAudioTask() {
  if (!taskRunning) {
    return;
  }

  taskRunning = false;

  // Delete task
  if (audioTaskHandle != NULL) {
    vTaskDelete(audioTaskHandle);
    audioTaskHandle = NULL;
  }

  DEBUG_PRINTLN("[AUDIO] Continuous audio task stopped");
}

// Static wrapper for FreeRTOS task
void AudioEngine::audioTaskFunction(void* parameter) {
  // Cast parameter back to AudioEngine instance
  AudioEngine* engine = static_cast<AudioEngine*>(parameter);

  // Call instance method
  engine->audioTaskLoop();
}

// Audio task loop - runs continuously
void AudioEngine::audioTaskLoop() {
  DEBUG_PRINTLN("[AUDIO] Audio task loop started");

  while (taskRunning) {
    // Generate and write one buffer to I2S
    // This blocks until DMA buffer is available (~11ms at 22050Hz)
    generateAudioBuffer();

    // No delay needed - i2s_write() naturally paces us
    // It blocks until hardware needs next buffer
  }

  DEBUG_PRINTLN("[AUDIO] Audio task loop exited");
  vTaskDelete(NULL);  // Delete self
}
