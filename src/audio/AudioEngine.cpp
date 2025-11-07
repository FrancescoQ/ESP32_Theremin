/*
 * AudioEngine.cpp
 *
 * Implementation of audio synthesis for ESP32 Theremin.
 * Uses I2S with external PCM5102 DAC for professional-grade 16-bit stereo audio output.
 */

#include "audio/AudioEngine.h"
#include "system/PerformanceMonitor.h"
#include "system/NotificationManager.h"
#include "system/Debug.h"

// ============================================================================
// SECTION 1: LIFECYCLE & INITIALIZATION
// ============================================================================

// Constructor
AudioEngine::AudioEngine(PerformanceMonitor* perfMon)
    : currentFrequency(DEFAULT_MIN_FREQUENCY),
      currentAmplitude(0),
      smoothedAmplitude(0.0),
      smoothedFrequency((float)DEFAULT_MIN_FREQUENCY),
      minFrequency(DEFAULT_MIN_FREQUENCY),
      maxFrequency(DEFAULT_MAX_FREQUENCY),
      pitchSmoothingFactor(DEFAULT_PITCH_SMOOTHING),
      volumeSmoothingFactor(DEFAULT_VOLUME_SMOOTHING),
      currentChannelMode(STEREO_BOTH),
      audioTaskHandle(NULL),
      paramMutex(NULL),
      taskRunning(false),
      effectsChain(nullptr),
      performanceMonitor(perfMon),
      notificationManager(nullptr) {

  // Create mutex for thread-safe parameter updates
  paramMutex = xSemaphoreCreateMutex();

  // Create effects chain
  // Manual new/delete pattern - see AudioEngine.h for full memory management explanation
  // and comparison with modern alternatives (std::unique_ptr, std::vector)
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

  DEBUG_PRINTLN("[AUDIO] Initializing PCM5102 I2S DAC...");
  delay(50);  // Let Serial transmit before continuing

  if (!setupI2S()) {
    DEBUG_PRINTLN("[AUDIO] ERROR: I2S initialization failed!");
    return;  // Don't start audio task if I2S failed
  }

  DEBUG_PRINTLN("[AUDIO] PCM5102 initialized (BCK:GPIO26, WS:GPIO27, DIN:GPIO25) @ 22050 Hz stereo");
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
  setOscillatorVolume(2, 0.6);

  setOscillatorWaveform(3, Oscillator::OFF);
  setOscillatorOctave(3, Oscillator::OCTAVE_BASE);
  setOscillatorVolume(3, 0.5);
}

// Set frequency (thread-safe)
void AudioEngine::setFrequency(int freq) {
  if (paramMutex != NULL && xSemaphoreTake(paramMutex, portMAX_DELAY) == pdTRUE) {
    // Constrain to current dynamic range
    currentFrequency = constrain(freq, minFrequency, maxFrequency);

    // Don't update oscillators here - let generateAudioBuffer() do it with smoothing

    xSemaphoreGive(paramMutex);
  }
}

// Set amplitude (thread-safe)
void AudioEngine::setAmplitude(int amplitude) {
  if (paramMutex != NULL && xSemaphoreTake(paramMutex, portMAX_DELAY) == pdTRUE) {
    // Constrain to 0-100%
    currentAmplitude = constrain(amplitude, 0, 100);

    // Don't update oscillators here - let generateAudioBuffer() do it with smoothing

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

    // Show notification if notification manager available
    if (notificationManager != nullptr) {
      // Convert waveform enum to short name
      const char* waveformName;
      switch (wf) {
        case Oscillator::OFF: {
          waveformName = "OFF";
          break;
        }
        case Oscillator::SINE: {
          waveformName = "SIN";
          break;
        }
        case Oscillator::SQUARE: {
          waveformName = "SQR";
          break;
        }
        case Oscillator::TRIANGLE: {
          waveformName = "TRI";
          break;
        }
        case Oscillator::SAW: {
          waveformName = "SAW";
          break;
        }
        default: {
          waveformName = "???";
          break;
        }
      }

      // Format: "OSC1:SIN"
      String message = "OSC" + String(oscNum) + ":" + String(waveformName);
      notificationManager->show(message);
    }
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

// Set pitch smoothing factor (thread-safe)
void AudioEngine::setPitchSmoothingFactor(float factor) {
  if (paramMutex != NULL && xSemaphoreTake(paramMutex, portMAX_DELAY) == pdTRUE) {
    pitchSmoothingFactor = constrain(factor, 0.0f, 1.0f);
    DEBUG_PRINT("[AUDIO] Pitch smoothing factor set to ");
    DEBUG_PRINTLN(pitchSmoothingFactor);
    xSemaphoreGive(paramMutex);
  }
}

// Set volume smoothing factor (thread-safe)
void AudioEngine::setVolumeSmoothingFactor(float factor) {
  if (paramMutex != NULL && xSemaphoreTake(paramMutex, portMAX_DELAY) == pdTRUE) {
    volumeSmoothingFactor = constrain(factor, 0.0f, 1.0f);
    DEBUG_PRINT("[AUDIO] Volume smoothing factor set to ");
    DEBUG_PRINTLN(volumeSmoothingFactor);
    xSemaphoreGive(paramMutex);
  }
}

// Set stereo channel routing mode (thread-safe)
void AudioEngine::setChannelMode(ChannelMode mode) {
  if (paramMutex != NULL && xSemaphoreTake(paramMutex, portMAX_DELAY) == pdTRUE) {
    currentChannelMode = mode;

    DEBUG_PRINT("[AUDIO] Channel mode set to ");
    switch (mode) {
      case STEREO_BOTH:
        DEBUG_PRINTLN("STEREO_BOTH (L+R)");
        break;
      case LEFT_ONLY:
        DEBUG_PRINTLN("LEFT_ONLY");
        break;
      case RIGHT_ONLY:
        DEBUG_PRINTLN("RIGHT_ONLY");
        break;
    }

    xSemaphoreGive(paramMutex);
  }
}

// Get stereo channel routing mode (thread-safe)
AudioEngine::ChannelMode AudioEngine::getChannelMode() const {
  return currentChannelMode;
}

// Set notification manager for displaying control changes
void AudioEngine::setNotificationManager(NotificationManager* notifMgr) {
  notificationManager = notifMgr;
  DEBUG_PRINTLN("[AUDIO] Notification manager connected");
}

// Set frequency range dynamically (thread-safe)
void AudioEngine::setFrequencyRange(int minFreq, int maxFreq) {
  if (paramMutex != NULL && xSemaphoreTake(paramMutex, portMAX_DELAY) == pdTRUE) {
    minFrequency = minFreq;
    maxFrequency = maxFreq;

    // Re-constrain current frequency to new range
    currentFrequency = constrain(currentFrequency, minFrequency, maxFrequency);
    smoothedFrequency = constrain(smoothedFrequency, (float)minFrequency, (float)maxFrequency);

    DEBUG_PRINT("[AUDIO] Frequency range set to ");
    DEBUG_PRINT(minFrequency);
    DEBUG_PRINT(" - ");
    DEBUG_PRINT(maxFrequency);
    DEBUG_PRINTLN(" Hz");

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

// Special states check.
bool AudioEngine::getSpecialState(int state) {
  int currentState = 0;
  Oscillator::Waveform osc1waveform = oscillator1.getWaveform();
  Oscillator::Waveform osc2waveform = oscillator2.getWaveform();
  Oscillator::Waveform osc3waveform = oscillator3.getWaveform();
  int osc1octave = oscillator1.getOctaveShift();
  int osc2octave = oscillator2.getOctaveShift();
  int osc3octave = oscillator3.getOctaveShift();

  // Special state 1: all oscillators OFF and all octave switches at -1
  if (state == 1 && osc1waveform == Oscillator::OFF && osc2waveform == Oscillator::OFF && osc3waveform == Oscillator::OFF &&
      osc1octave == Oscillator::OCTAVE_DOWN && osc2octave == Oscillator::OCTAVE_DOWN && osc3octave == Oscillator::OCTAVE_DOWN) {
    return true;
  }

  // Special state 2, 3, 4: only one oscillator ON (triangle), others OFF, with
  // that oscillator at +1 octave: used to turn on effects during startup.
  if (osc1waveform == Oscillator::OFF &&
      osc2waveform == Oscillator::OFF &&
      osc3waveform == Oscillator::TRIANGLE) {

    if (state == 2 && osc1octave == Oscillator::OCTAVE_UP) {
      return true;
    }
    if (state == 3 && osc2octave == Oscillator::OCTAVE_UP) {
      return true;
    }
    if (state == 4 && osc3octave == Oscillator::OCTAVE_UP) {
      return true;
    }
  }

  return false;
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
      150, 150, 150, 450,
      450, 450, 150, 150, 150, 600
  };
  constexpr int ff7_length = sizeof(ff7_melody) / sizeof(ff7_melody[0]);
  playMelody(ff7_melody, ff7_durations, ff7_length, 1, Oscillator::SQUARE);
  delay(500);  // Brief pause after melody
}

// Play a melody sequence
void AudioEngine::playMelody(const int notes[], const int durations[], int length, int oscNum, Oscillator::Waveform waveform, float staccato, int amplitude) {
  DEBUG_PRINTLN("[AUDIO] Playing melody...");

  // Save current state to restore later
  int savedFrequency = currentFrequency;
  int savedAmplitude = currentAmplitude;
  Oscillator::Waveform savedWaveform1 = oscillator1.getWaveform();
  Oscillator::Waveform savedWaveform2 = oscillator2.getWaveform();
  Oscillator::Waveform savedWaveform3 = oscillator3.getWaveform();

  // Configure specified oscillator for melody, silence others
  setAmplitude(amplitude);

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
      setAmplitude(amplitude);

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

// Initialize I2S for external PCM5102 DAC
bool AudioEngine::setupI2S() {
  // I2S configuration for PCM5102 external DAC (stereo 16-bit)
  i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),  // Standard I2S mode (no built-in DAC)
      .sample_rate = Audio::SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,  // Stereo output (L+R channels)
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
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

  // Configure I2S pins for PCM5102
  i2s_pin_config_t pin_config = {
      .bck_io_num = PIN_I2S_BCK,        // Bit clock (GPIO26)
      .ws_io_num = PIN_I2S_WS,          // Word select / LRCK (GPIO27)
      .data_out_num = PIN_I2S_DOUT,     // Data output (GPIO25)
      .data_in_num = I2S_PIN_NO_CHANGE  // No data input needed
  };

  err = i2s_set_pin((i2s_port_t)I2S_NUM, &pin_config);
  if (err != ESP_OK) {
    DEBUG_PRINT("[AUDIO] ERROR: I2S pin configuration failed with error code: ");
    DEBUG_PRINTLN(err);
    return false;
  }

  return true;
}

// Generate audio buffer and write to I2S
void AudioEngine::generateAudioBuffer() {
  // Master output noise gate threshold
  // Eliminates cumulative quantization noise from stacked effects
  static constexpr int16_t MASTER_NOISE_GATE_THRESHOLD = 150;

  // PCM5102 accepts signed 16-bit stereo samples directly
  // Buffer holds stereo frames: [L0, R0, L1, R1, L2, R2, ...]
  // BUFFER_SIZE = 256 frames = 512 individual samples (L+R)
  int16_t buffer[BUFFER_SIZE * 2];

  // Start CPU measurement (only measure actual computation, not blocking I/O)
  uint32_t computeStart = micros();

  // Lock mutex to safely read parameters
  if (paramMutex != NULL && xSemaphoreTake(paramMutex, 0) == pdTRUE) {
    // Apply exponential smoothing to frequency (uses separate pitch factor)
    smoothedFrequency += (currentFrequency - smoothedFrequency) * pitchSmoothingFactor;

    // Apply exponential smoothing to amplitude (uses separate volume factor)
    smoothedAmplitude += (currentAmplitude - smoothedAmplitude) * volumeSmoothingFactor;

    // Update all oscillator frequencies with smoothed value
    oscillator1.setFrequency(smoothedFrequency);
    oscillator2.setFrequency(smoothedFrequency);
    oscillator3.setFrequency(smoothedFrequency);

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

    // MASTER OUTPUT NOISE GATE
    // Eliminates cumulative quantization noise from stacked effects (delay + chorus + reverb)
    // Kills low-level graininess that accumulates through the effects chain
    if (scaledSample > -MASTER_NOISE_GATE_THRESHOLD && scaledSample < MASTER_NOISE_GATE_THRESHOLD) {
      scaledSample = 0;
    }

    // PCM5102 accepts signed 16-bit samples directly - no conversion needed!
    // Route to channels based on current mode
    switch (currentChannelMode) {
      case LEFT_ONLY:
        buffer[i * 2] = scaledSample;   // Left channel
        buffer[i * 2 + 1] = 0;          // Right channel muted
        break;
      case RIGHT_ONLY:
        buffer[i * 2] = 0;              // Left channel muted
        buffer[i * 2 + 1] = scaledSample; // Right channel
        break;
      case STEREO_BOTH:
      default:
        buffer[i * 2] = scaledSample;     // Left channel
        buffer[i * 2 + 1] = scaledSample; // Right channel (duplicate)
        break;
    }
  }

  // Stop CPU measurement (sample calculation done)
  uint32_t computeTime = micros() - computeStart;

  // Write stereo buffer to I2S (blocks until DMA buffer available ~11ms)
  // Why this blocks for ~11ms:
  // - ESP32 I2S hardware consumes samples at exactly SAMPLE_RATE (22050 Hz)
  // - 256 stereo frames / 22050 Hz = 11.6ms to consume one buffer
  // - i2s_write() blocks until DMA has space (natural flow control)
  // - This is GOOD: prevents buffer overruns, perfectly paced audio output
  // - This blocking is I/O waiting, NOT CPU work (CPU is free for other tasks)
  // - Audio task sleeps here, wakes up when hardware needs next buffer
  size_t bytes_written = 0;
  i2s_write((i2s_port_t)I2S_NUM, buffer, BUFFER_SIZE * 2 * sizeof(int16_t), &bytes_written, portMAX_DELAY);

  // Report only the actual CPU work time (not the blocking time)
  if (performanceMonitor != nullptr) {
    performanceMonitor->recordAudioWork(computeTime);
  }
}
