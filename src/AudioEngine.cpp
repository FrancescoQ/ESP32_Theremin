/*
 * AudioEngine.cpp
 *
 * Implementation of audio synthesis for ESP32 Theremin.
 * Uses I2S in built-in DAC mode for high-quality audio output.
 */

#include "AudioEngine.h"
#include "PerformanceMonitor.h"
#include "Debug.h"

// ============================================================================
// SECTION 1: LIFECYCLE & INITIALIZATION
// ============================================================================

// Constructor
AudioEngine::AudioEngine(PerformanceMonitor* perfMon)
    : currentFrequency(MIN_FREQUENCY),
      currentAmplitude(0),
      smoothedAmplitude(0.0),
      audioTaskHandle(NULL),
      paramMutex(NULL),
      taskRunning(false),
      effectsChain(nullptr),
      performanceMonitor(perfMon) {

  // Create mutex for thread-safe parameter updates
  paramMutex = xSemaphoreCreateMutex();

  // Create effects chain
  // NOTE: Allocated on heap (using 'new') rather than as direct member for:
  // 1. Didactic purpose - demonstrates heap allocation pattern with pointers
  // 2. Future flexibility - easy to make effects optional (conditional creation)
  // 3. Explicit about size - clear that EffectsChain is large (~13 KB)
  //
  // Alternative approach: Could be direct member (EffectsChain effectsChain;)
  // since AudioEngine is global (lives in static data, not function stack).
  // Both approaches use same total RAM, just different memory locations.
  // The heap approach is more common for large/optional components in practice.
  effectsChain = new EffectsChain();
  DEBUG_PRINTLN("[AUDIO] Effects chain created");
}

// Destructor
// Note: Never called in practice (AudioEngine lives until power-off),
// but included as C++ best practice (RAII) for proper resource cleanup.
// See AudioEngine.h for detailed explanation.
AudioEngine::~AudioEngine() {
  // Stop audio task first (safety)
  stopAudioTask();

  // Clean up effects chain (frees ~13 KB delay buffer)
  if (effectsChain != nullptr) {
    delete effectsChain;
    effectsChain = nullptr;
  }

  // Delete mutex (FreeRTOS resource)
  if (paramMutex != NULL) {
    vSemaphoreDelete(paramMutex);
    paramMutex = NULL;
  }

  DEBUG_PRINTLN("[AUDIO] AudioEngine destroyed");
}

// Initialize audio hardware
void AudioEngine::begin() {
  // Delay to ensure Serial is fully initialized
  delay(100);

  DEBUG_PRINTLN("[AUDIO] Initializing I2S DAC...");
  delay(50);  // Let Serial transmit before continuing

  if (!setupI2S()) {
    DEBUG_PRINTLN("[AUDIO] ERROR: I2S initialization failed!");
    return;  // Don't start audio task if I2S failed
  }

  DEBUG_PRINTLN("[AUDIO] I2S DAC initialized on GPIO25 @ 22050 Hz");
  delay(50);  // Let Serial transmit before continuing

  // Initialize oscillators with default settings.
  setDefaultSettings();

  // Start continuous audio generation task only if I2S succeeded
  startAudioTask();
}

// Update audio output
// DEPRECATED: No longer needed with continuous audio task
void AudioEngine::update() {
  // This function is now a no-op
  // Audio generation happens continuously in background task
  // Kept for backward compatibility
}

// ============================================================================
// SECTION 2: CONFIGURATION & SETTINGS
// ============================================================================

// Default settings.
void AudioEngine::setDefaultSettings() {
  setAmplitude(0);
  setOscillatorWaveform(1, Oscillator::TRIANGLE);
  setOscillatorOctave(1, Oscillator::OCTAVE_BASE);
  setOscillatorVolume(1, 1.0);

  setOscillatorWaveform(2, Oscillator::OFF);
  setOscillatorOctave(2, Oscillator::OCTAVE_BASE);
  setOscillatorVolume(2, 1.0);

  setOscillatorWaveform(3, Oscillator::OFF);
  setOscillatorOctave(3, Oscillator::OCTAVE_BASE);
  setOscillatorVolume(3, 1.0);
}

// Set frequency (thread-safe)
void AudioEngine::setFrequency(int freq) {
  if (paramMutex != NULL && xSemaphoreTake(paramMutex, portMAX_DELAY) == pdTRUE) {
    // Constrain to valid range (A3-A5)
    currentFrequency = constrain(freq, MIN_FREQUENCY, MAX_FREQUENCY);

    // Update all oscillator frequencies
    oscillator1.setFrequency((float)currentFrequency);
    oscillator2.setFrequency((float)currentFrequency);
    oscillator3.setFrequency((float)currentFrequency);

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

// ============================================================================
// SECTION 3: OSCILLATOR CONTROL - SETTERS
// ============================================================================

// Set waveform for specific oscillator
void AudioEngine::setOscillatorWaveform(int oscNum, Oscillator::Waveform wf) {
  // Validate oscillator number
  if (oscNum < 1 || oscNum > 3) {
    DEBUG_PRINT("[AUDIO] Invalid oscillator number: ");
    DEBUG_PRINTLN(oscNum);
    return;
  }

  // Thread-safe parameter update
  if (paramMutex != NULL && xSemaphoreTake(paramMutex, portMAX_DELAY) == pdTRUE) {
    switch (oscNum) {
      case 1:
        oscillator1.setWaveform(wf);
        break;
      case 2:
        oscillator2.setWaveform(wf);
        break;
      case 3:
        oscillator3.setWaveform(wf);
        break;
    }

    DEBUG_PRINT("[AUDIO] Oscillator ");
    DEBUG_PRINT(oscNum);
    DEBUG_PRINT(" waveform set to ");
    DEBUG_PRINTLN((int)wf);

    xSemaphoreGive(paramMutex);
  }
}

// Set octave shift for specific oscillator
void AudioEngine::setOscillatorOctave(int oscNum, int octave) {
  // Validate oscillator number
  if (oscNum < 1 || oscNum > 3) {
    DEBUG_PRINT("[AUDIO] Invalid oscillator number: ");
    DEBUG_PRINTLN(oscNum);
    return;
  }

  // Validate octave range
  if (octave < -1 || octave > 1) {
    DEBUG_PRINT("[AUDIO] Invalid octave shift: ");
    DEBUG_PRINTLN(octave);
    return;
  }

  // Thread-safe parameter update
  if (paramMutex != NULL && xSemaphoreTake(paramMutex, portMAX_DELAY) == pdTRUE) {
    switch (oscNum) {
      case 1:
        oscillator1.setOctaveShift(octave);
        break;
      case 2:
        oscillator2.setOctaveShift(octave);
        break;
      case 3:
        oscillator3.setOctaveShift(octave);
        break;
    }

    DEBUG_PRINT("[AUDIO] Oscillator ");
    DEBUG_PRINT(oscNum);
    DEBUG_PRINT(" octave shift set to ");
    DEBUG_PRINTLN(octave);

    xSemaphoreGive(paramMutex);
  }
}

// Set volume for specific oscillator
void AudioEngine::setOscillatorVolume(int oscNum, float volume) {
  // Validate oscillator number
  if (oscNum < 1 || oscNum > 3) {
    DEBUG_PRINT("[AUDIO] Invalid oscillator number: ");
    DEBUG_PRINTLN(oscNum);
    return;
  }

  // Thread-safe parameter update
  if (paramMutex != NULL && xSemaphoreTake(paramMutex, portMAX_DELAY) == pdTRUE) {
    switch (oscNum) {
      case 1:
        oscillator1.setVolume(volume);
        break;
      case 2:
        oscillator2.setVolume(volume);
        break;
      case 3:
        oscillator3.setVolume(volume);
        break;
    }

    DEBUG_PRINT("[AUDIO] Oscillator ");
    DEBUG_PRINT(oscNum);
    DEBUG_PRINT(" volume set to ");
    DEBUG_PRINTLN(volume);

    xSemaphoreGive(paramMutex);
  }
}

// ============================================================================
// SECTION 4: OSCILLATOR CONTROL - GETTERS
// ============================================================================

// Get waveform for specific oscillator
Oscillator::Waveform AudioEngine::getOscillatorWaveform(int oscNum) {
  // Validate oscillator number
  if (oscNum < 1 || oscNum > 3) {
    DEBUG_PRINT("[AUDIO] Invalid oscillator number: ");
    DEBUG_PRINTLN(oscNum);
    return Oscillator::OFF;
  }

  Oscillator::Waveform waveform = Oscillator::OFF;

  // Thread-safe parameter read
  if (paramMutex != NULL && xSemaphoreTake(paramMutex, portMAX_DELAY) == pdTRUE) {
    switch (oscNum) {
      case 1:
        waveform = oscillator1.getWaveform();
        break;
      case 2:
        waveform = oscillator2.getWaveform();
        break;
      case 3:
        waveform = oscillator3.getWaveform();
        break;
    }

    xSemaphoreGive(paramMutex);
  }

  return waveform;
}

// Get octave shift for specific oscillator
int AudioEngine::getOscillatorOctave(int oscNum) {
  // Validate oscillator number
  if (oscNum < 1 || oscNum > 3) {
    DEBUG_PRINT("[AUDIO] Invalid oscillator number: ");
    DEBUG_PRINTLN(oscNum);
    return 0;
  }

  int octave = 0;

  // Thread-safe parameter read
  if (paramMutex != NULL && xSemaphoreTake(paramMutex, portMAX_DELAY) == pdTRUE) {
    switch (oscNum) {
      case 1:
        octave = oscillator1.getOctaveShift();
        break;
      case 2:
        octave = oscillator2.getOctaveShift();
        break;
      case 3:
        octave = oscillator3.getOctaveShift();
        break;
    }

    xSemaphoreGive(paramMutex);
  }

  return octave;
}

// Get volume for specific oscillator
float AudioEngine::getOscillatorVolume(int oscNum) {
  // Validate oscillator number
  if (oscNum < 1 || oscNum > 3) {
    DEBUG_PRINT("[AUDIO] Invalid oscillator number: ");
    DEBUG_PRINTLN(oscNum);
    return 0.0;
  }

  float volume = 0.0;

  // Thread-safe parameter read
  if (paramMutex != NULL && xSemaphoreTake(paramMutex, portMAX_DELAY) == pdTRUE) {
    switch (oscNum) {
      case 1:
        volume = oscillator1.getVolume();
        break;
      case 2:
        volume = oscillator2.getVolume();
        break;
      case 3:
        volume = oscillator3.getVolume();
        break;
    }

    xSemaphoreGive(paramMutex);
  }

  return volume;
}

// ============================================================================
// SECTION 5: SOUND EFFECTS & TESTING
// ============================================================================

// Play the startup sound: FF7 Victory Fanfare
void AudioEngine::playStartupSound() {
  // FF7 Victory Fanfare (simplified version)
  DEBUG_PRINTLN("\n[STARTUP] Playing Final Fantasy VII Victory Theme...");
  const int ff7_melody[] = {
      NOTE_C5,  NOTE_C5,   NOTE_C5, NOTE_C5,
      NOTE_GS4, NOTE_AS4,  NOTE_C5, NOTE_REST, NOTE_AS4, NOTE_C5
  };
  const int ff7_durations[] = {
      150, 150, 150, 450, // Quick notes then hold
      450, 450, 150, 150, 150, 600   // Final ascending phrase with long ending
  };
  constexpr int ff7_length = sizeof(ff7_melody) / sizeof(ff7_melody[0]);
  playMelody(ff7_melody, ff7_durations, ff7_length, 1, Oscillator::SQUARE);
  delay(500);  // Brief pause after melody
}

// Play a melody sequence
void AudioEngine::playMelody(const int notes[], const int durations[], int length,
                             int oscNum, Oscillator::Waveform waveform, float staccato) {
  DEBUG_PRINTLN("[AUDIO] Playing melody...");

  // Save current state to restore later
  int savedFrequency = currentFrequency;
  int savedAmplitude = currentAmplitude;
  Oscillator::Waveform savedWaveform1 = oscillator1.getWaveform();
  Oscillator::Waveform savedWaveform2 = oscillator2.getWaveform();
  Oscillator::Waveform savedWaveform3 = oscillator3.getWaveform();

  // Configure specified oscillator for melody, silence others
  setAmplitude(40);

  for (int i = 1; i <= 3; i++) {
    if (i == oscNum) {
      setOscillatorWaveform(i, waveform);
      setOscillatorVolume(i, 1.0);
    } else {
      setOscillatorWaveform(i, Oscillator::OFF);
    }
  }

  // Play each note
  for (int i = 0; i < length; i++) {
    if (notes[i] == NOTE_REST) {
      // Rest = silence for full duration
      setAmplitude(0);
      delay(durations[i]);
    } else {
      // Play note with staccato articulation
      setFrequency(notes[i]);
      setAmplitude(60);

      // Apply staccato: play for X% of duration, then gap
      int soundDuration = (int)(durations[i] * staccato);
      int gapDuration = durations[i] - soundDuration;

      delay(soundDuration);  // Note sounds

      // Add gap after note (if staccato < 1.0)
      if (gapDuration > 0) {
        setAmplitude(0);  // Silence
        delay(gapDuration);
      }
    }
  }

  // Restore previous state
  setFrequency(savedFrequency);
  setAmplitude(0);
  setOscillatorWaveform(1, savedWaveform1);
  setOscillatorWaveform(2, savedWaveform2);
  setOscillatorWaveform(3, savedWaveform3);

  DEBUG_PRINTLN("[AUDIO] Melody complete");
}

// System test sequence
void AudioEngine::systemTest() {
  DEBUG_PRINTLN("\n[TEST] Starting system test...");

  // Test configuration: single oscillator with clear tones
  setFrequency(NOTE_A4);
  const int testAmplitude = 40;
  setAmplitude(testAmplitude);

  // Turn off oscillators 2 and 3 for clarity
  setOscillatorWaveform(2, Oscillator::OFF);
  setOscillatorWaveform(3, Oscillator::OFF);

  // Test 1: Default configuration (oscillator 1, sine wave)
  DEBUG_PRINTLN("[TEST] Test 1: Default settings");
  setDefaultSettings();
  delay(1000);

  setAmplitude(0);
  delay(500);
  setAmplitude(testAmplitude);

  // Test 2: Waveform change
  DEBUG_PRINTLN("[TEST] Test 2: Changing waveforms");
  setOscillatorWaveform(1, Oscillator::TRIANGLE);
  delay(1000);
  setAmplitude(0);
  delay(500);
  setAmplitude(testAmplitude);
  setOscillatorWaveform(1, Oscillator::SAW);
  delay(1000);
  setAmplitude(0);
  delay(500);
  setAmplitude(testAmplitude);
  setOscillatorWaveform(1, Oscillator::SQUARE);
  delay(1000);
  setAmplitude(0);
  delay(500);
  setAmplitude(testAmplitude);
  setOscillatorWaveform(1, Oscillator::SINE);
  delay(1000);
  setAmplitude(0);
  delay(500);
  setAmplitude(testAmplitude);

  // Test 3: Octave shift up
  DEBUG_PRINTLN("[TEST] Test 3: Shifting up/down one octave");
  setOscillatorOctave(1, Oscillator::OCTAVE_UP);
  delay(1000);

  setOscillatorOctave(1, Oscillator::OCTAVE_DOWN);
  delay(1000);

  // Test 4: Volume reduction
  DEBUG_PRINTLN("[TEST] Test 4: Oscillator volume");
  setOscillatorOctave(1, Oscillator::OCTAVE_BASE);
  setOscillatorVolume(1, 0);
  delay(200);
  setOscillatorVolume(1, 0.1);
  delay(200);
  setOscillatorVolume(1, 0.2);
  delay(200);
  setOscillatorVolume(1, 0.3);
  delay(200);
  setOscillatorVolume(1, 0.4);
  delay(200);
  setOscillatorVolume(1, 0.5);
  delay(200);
  setOscillatorVolume(1, 0.6);
  delay(200);
  setOscillatorVolume(1, 0.7);
  delay(200);
  setOscillatorVolume(1, 0.8);
  delay(200);
  setOscillatorVolume(1, 0.9);
  delay(200);
  setOscillatorVolume(1, 1);
  delay(200);

  // Test 5: Restore defaults
  DEBUG_PRINTLN("[TEST] Test 5: Restoring defaults");
  setAmplitude(0);
  delay(1000);
  setDefaultSettings();


  DEBUG_PRINTLN("[TEST] System test complete!\n");
}

// ============================================================================
// SECTION 6: AUDIO TASK MANAGEMENT
// ============================================================================

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
  delay(50);  // Let Serial transmit before continuing
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

  // Initialize CPU measurement
  if (performanceMonitor != nullptr) {
    performanceMonitor->beginAudioMeasurement();
  }

  while (taskRunning) {
    // Generate and write one buffer to I2S
    // CPU measurement happens inside generateAudioBuffer() (excludes blocking I/O)
    generateAudioBuffer();
  }

  DEBUG_PRINTLN("[AUDIO] Audio task loop exited");
  vTaskDelete(NULL);  // Delete self
}

// ============================================================================
// SECTION 7: PRIVATE/INTERNAL IMPLEMENTATION
// ============================================================================

// Initialize I2S in built-in DAC mode
bool AudioEngine::setupI2S() {
  // I2S configuration for built-in DAC
  i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
      .sample_rate = Audio::SAMPLE_RATE,
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

  // Start CPU measurement (only measure actual computation, not blocking I/O)
  uint32_t computeStart = micros();

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
    // Mix multiple oscillators with automatic gain adjustment
    int activeCount = 0;
    int32_t mixedSample = 0;  // Use int32_t to prevent overflow during addition

    // Add samples from all active oscillators
    if (oscillator1.isActive()) {
      mixedSample += oscillator1.getNextSample((float)Audio::SAMPLE_RATE);
      activeCount++;
    }
    if (oscillator2.isActive()) {
      mixedSample += oscillator2.getNextSample((float)Audio::SAMPLE_RATE);
      activeCount++;
    }
    if (oscillator3.isActive()) {
      mixedSample += oscillator3.getNextSample((float)Audio::SAMPLE_RATE);
      activeCount++;
    }

    // Average to prevent clipping and maintain consistent volume
    int16_t sample = (activeCount > 0) ? (mixedSample / activeCount) : 0;

    // Apply amplitude scaling
    int16_t scaledSample = (int16_t)(sample * gain);

    // Process through effects chain
    if (effectsChain != nullptr) {
      scaledSample = effectsChain->process(scaledSample);
    }

    // Convert signed 16-bit to unsigned 8-bit for DAC:
    // 1. Take upper 8 bits (>> DAC_BIT_SHIFT): -32768..32767 → -128..127
    // 2. Add DAC_ZERO_OFFSET for DC offset: -128..127 → 0..255
    // 3. Store in upper byte of 16-bit word (DAC reads upper byte)
    uint8_t dacSample = (uint8_t)((scaledSample >> DAC_BIT_SHIFT) + DAC_ZERO_OFFSET);
    buffer[i] = ((uint16_t)dacSample) << DAC_BIT_SHIFT;  // Put 8-bit sample in upper byte
  }

  // Stop CPU measurement (sample calculation done)
  uint32_t computeTime = micros() - computeStart;

  // Write buffer to I2S (blocks until DMA buffer available ~11ms)
  // Why this blocks for ~11ms:
  // - ESP32 I2S hardware consumes samples at exactly SAMPLE_RATE (22050 Hz)
  // - 256 samples / 22050 Hz = 11.6ms to consume one buffer
  // - i2s_write() blocks until DMA has space (natural flow control)
  // - This is GOOD: prevents buffer overruns, perfectly paced audio output
  // - This blocking is I/O waiting, NOT CPU work (CPU is free for other tasks)
  // - Audio task sleeps here, wakes up when hardware needs next buffer
  size_t bytes_written = 0;
  i2s_write((i2s_port_t)I2S_NUM, buffer, BUFFER_SIZE * sizeof(uint16_t), &bytes_written, portMAX_DELAY);

  // Report only the actual CPU work time (not the blocking time)
  if (performanceMonitor != nullptr) {
    performanceMonitor->recordAudioWork(computeTime);
  }
}
