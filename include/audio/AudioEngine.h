/*
 * AudioEngine.h
 *
 * Manages audio synthesis for the ESP32 Theremin.
 * Uses I2S in built-in DAC mode for audio output (GPIO25).
 * Designed for modular oscillator architecture.
 */

#pragma once
#include <Arduino.h>
#include <driver/i2s.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "system/PinConfig.h"
#include "audio/Oscillator.h"
#include "audio/effects/EffectsChain.h"
#include "audio/AudioConstants.h"

// Forward declaration to avoid circular dependency
class PerformanceMonitor;

// Musical note frequencies (Hz) - Scientific pitch notation
// Based on A4 = 440 Hz (concert pitch)
#define NOTE_REST  0     // Silence (rest)

// Octave 3
#define NOTE_C3   131
#define NOTE_CS3  139
#define NOTE_D3   147
#define NOTE_DS3  156
#define NOTE_E3   165
#define NOTE_F3   175
#define NOTE_FS3  185
#define NOTE_G3   196
#define NOTE_GS3  208
#define NOTE_A3   220
#define NOTE_AS3  233
#define NOTE_B3   247

// Octave 4 (middle octave)
#define NOTE_C4   262
#define NOTE_CS4  277
#define NOTE_D4   294
#define NOTE_DS4  311
#define NOTE_E4   330
#define NOTE_F4   349
#define NOTE_FS4  370
#define NOTE_G4   392
#define NOTE_GS4  415
#define NOTE_A4   440  // Concert pitch
#define NOTE_AS4  466
#define NOTE_B4   494

// Octave 5
#define NOTE_C5   523
#define NOTE_CS5  554
#define NOTE_D5   587
#define NOTE_DS5  622
#define NOTE_E5   659
#define NOTE_F5   698
#define NOTE_FS5  740
#define NOTE_G5   784
#define NOTE_GS5  831
#define NOTE_A5   880
#define NOTE_AS5  932
#define NOTE_B5   988

class AudioEngine {
 public:
  /**
   * Constructor
   * @param perfMon Pointer to PerformanceMonitor instance (optional)
   */
  AudioEngine(PerformanceMonitor* perfMon = nullptr);

  /**
   * Destructor - cleans up effects chain and FreeRTOS resources
   *
   * NOTE: In practice, this destructor is never called in the current design.
   * AudioEngine is created once at startup in main() and lives until the ESP32
   * loses power. However, this destructor exists as a C++ best practice for:
   *
   * 1. RAII (Resource Acquisition Is Initialization) pattern compliance:
   *    RAII is a C++ idiom where resource lifetime is tied to object lifetime.
   *    Constructor acquires resources (allocates memory, creates mutex, starts task),
   *    Destructor releases resources (frees memory, deletes mutex, stops task).
   *    This makes resource management automatic and prevents leaks by design.
   *    Even if never called, having a destructor documents what resources are owned
   *    and ensures cleanup happens correctly if the object lifetime ever changes.
   *
   * 2. Future-proofing (in case AudioEngine is later used dynamically)
   * 3. Self-documenting cleanup requirements (shows what resources are owned)
   * 4. Testing/debugging (allows clean create/destroy cycles in unit tests)
   *
   * Without this destructor, EffectsChain (and its ~13KB delay buffer) would
   * leak if AudioEngine were ever destroyed, and FreeRTOS resources would remain
   * allocated. While this doesn't matter in the embedded single-instance context,
   * it's good practice to make resource ownership and cleanup explicit.
   */
  ~AudioEngine();

  /**
   * Initialize audio hardware (must be called in setup())
   */
  void begin();

  /**
   * Initialize default settings for the audio engine / oscillators.
   */
  void setDefaultSettings();

  /**
   * Set the audio frequency in Hz
   * @param freq Frequency in Hz (will be constrained to MIN_FREQUENCY-MAX_FREQUENCY)
   */
  void setFrequency(int freq);

  /**
   * Set the audio amplitude (0-100%)
   * @param amplitude Amplitude percentage (0-100)
   */
  void setAmplitude(int amplitude);

  /**
   * Set waveform for specific oscillator
   * @param oscNum Oscillator number (1-3)
   * @param wf Waveform type (OFF, SQUARE, SINE, TRIANGLE, SAW)
   */
  void setOscillatorWaveform(int oscNum, Oscillator::Waveform wf);

  /**
   * Set octave shift for specific oscillator
   * @param oscNum Oscillator number (1-3)
   * @param octave Octave shift (-1, 0, +1)
   */
  void setOscillatorOctave(int oscNum, int octave);

  /**
   * Set volume for specific oscillator
   * @param oscNum Oscillator number (1-3)
   * @param volume Volume level (0.0 = silent, 1.0 = full)
   */
  void setOscillatorVolume(int oscNum, float volume);

  /**
   * Get waveform for specific oscillator
   * @param oscNum Oscillator number (1-3)
   * @return Current waveform type, or OFF if invalid oscNum
   */
  Oscillator::Waveform getOscillatorWaveform(int oscNum);

  /**
   * Get octave shift for specific oscillator
   * @param oscNum Oscillator number (1-3)
   * @return Current octave shift, or 0 if invalid oscNum
   */
  int getOscillatorOctave(int oscNum);

  /**
   * Get volume for specific oscillator
   * @param oscNum Oscillator number (1-3)
   * @return Current volume level (0.0-1.0), or 0.0 if invalid oscNum
   */
  float getOscillatorVolume(int oscNum);

  /**
   * Play a startup sound.
   */
  void playStartupSound();

  /**
   * Play a melody sequence
   * @param notes Array of note frequencies (use NOTE_* constants or Hz values)
   * @param durations Array of note durations in milliseconds
   * @param length Number of notes in the melody
   * @param oscNum Which oscillator to use (1-3, default 1)
   * @param waveform Which waveform to use (default SINE)
   * @param staccato Note articulation (1.0 = legato/smooth, 0.8 = standard staccato, 0.5 = very short)
   * @param amplitude Amplitude percentage (0-100, default 20)
   */
  void playMelody(const int notes[], const int durations[], int length,
                  int oscNum = 1, Oscillator::Waveform waveform = Oscillator::SINE,
                  float staccato = 0.8, int amplitude = 40);

  /**
   * Run system test sequence
   * Tests oscillator control capabilities with audible feedback
   */
  void systemTest();

  /**
   * Update audio output based on current frequency and amplitude
   * DEPRECATED: No longer needed with continuous audio task
   * Kept for backward compatibility but does nothing
   */
  void update();

  /**
   * Start continuous audio generation task
   * Called automatically by begin()
   */
  void startAudioTask();

  /**
   * Stop continuous audio generation task
   * Call before destroying AudioEngine instance
   */
  void stopAudioTask();

  /**
   * Get current frequency
   */
  int getFrequency() const {
    return currentFrequency;
  }

  /**
   * Get current amplitude
   */
  int getAmplitude() const {
    return currentAmplitude;
  }

  /**
   * Get effects chain for parameter control
   */
  EffectsChain* getEffectsChain() { return effectsChain; }

  // Audio range constants (A3 to A5, 2 octaves)
  static const int MIN_FREQUENCY = 220;  // A3
  static const int MAX_FREQUENCY = 880;  // A5

 private:
  // I2S configuration
  static const int I2S_NUM = 0;                // I2S port number
  static const int BUFFER_SIZE = 256;          // Samples per buffer
  static const int DMA_BUFFER_COUNT = 2;       // Number of DMA buffers

  // Audio Buffer Timing: Why 11ms?
  // Buffer duration = BUFFER_SIZE / Audio::SAMPLE_RATE = 256 / 22050 = 11.6ms
  // This means the audio task generates a new buffer every ~11ms, which is:
  // - Independent of main loop timing (runs on separate FreeRTOS task on Core 1)
  // - Naturally paced by i2s_write() blocking until DMA buffer is consumed
  // - Why sensor speed (main loop) doesn't affect audio quality
  // - Why reducing main loop delay improves vibrato response without affecting audio

  // Amplitude smoothing (prevents sudden volume jumps)
  // Tuning guide - adjust to taste:
  //   0.05 = ~2.0s fade time (very smooth, laggy)
  //   0.10 = ~1.2s fade time (smooth, professional)
  //   0.15 = ~0.8s fade time (balanced)
  //   0.25 = ~0.5s fade time (responsive, slight smoothing)
  //   0.50 = ~0.2s fade time (minimal smoothing)
  //   1.00 = instant (no smoothing, like before)
  static constexpr float SMOOTHING_FACTOR = 0.80;  // 0.0-1.0 (lower = smoother)

  // DAC conversion constants (16-bit signed → 8-bit unsigned)
  static constexpr uint8_t DAC_ZERO_OFFSET = 128;  // DC offset for unsigned conversion
  static constexpr uint8_t DAC_BIT_SHIFT = 8;      // Bit shift from 16-bit to 8-bit

  // Current state
  int currentFrequency;
  int currentAmplitude;     // Target amplitude
  float smoothedAmplitude;  // Actual smoothed amplitude value

  // Oscillator instance
  Oscillator oscillator1;
  Oscillator oscillator2;
  Oscillator oscillator3;

  // Effects chain
  // ════════════════════════════════════════════════════════════════════════
  // MEMORY MANAGEMENT PATTERN - Manual new/delete (Single Object)
  // ════════════════════════════════════════════════════════════════════════
  //
  // WHY HEAP ALLOCATION:
  //   1. Didactic purpose - demonstrates traditional C++ heap allocation with pointers
  //   2. Future flexibility - easy to make effects optional (conditional creation)
  //   3. Explicit about size - clear that EffectsChain is large (~13 KB)
  //
  // PATTERN USED HERE (Manual):
  //   Constructor:  effectsChain = new EffectsChain();
  //   Destructor:   delete effectsChain; effectsChain = nullptr;
  //
  // MODERN C++ ALTERNATIVE - std::unique_ptr<T> (Single Object):
  //   Header:       std::unique_ptr<EffectsChain> effectsChain;
  //   Constructor:  effectsChain.reset(new EffectsChain());  // C++11 compatible
  //   Destructor:   (nothing - automatic cleanup!)
  //   Usage:        effectsChain->process(sample);  // same syntax with ->
  //
  // WHY MANUAL PATTERN IS USED HERE:
  //   - Educational reference: Shows traditional C++ memory management
  //   - Explicit ownership: Makes resource lifecycle visible
  //   - Industry reality: Still common in embedded codebases
  //   - This file intentionally demonstrates the manual approach for learning
  //
  // MEMORY MANAGEMENT PATTERNS ACROSS THIS CODEBASE:
  //   ┌────────────────────────┬─────────────────────┬──────────────────────┐
  //   │ File                   │ Pattern             │ Use Case             │
  //   ├────────────────────────┼─────────────────────┼──────────────────────┤
  //   │ AudioEngine.h (this)   │ Raw new/delete      │ Single object        │
  //   │                        │ (manual)            │ Educational ref      │
  //   ├────────────────────────┼─────────────────────┼──────────────────────┤
  //   │ DelayEffect.h          │ std::vector         │ Dynamic array        │
  //   │                        │ (RAII)              │ Can resize           │
  //   ├────────────────────────┼─────────────────────┼──────────────────────┤
  //   │ ChorusEffect.h         │ std::unique_ptr<[]> │ Fixed array          │
  //   │                        │ (RAII)              │ Never resizes        │
  //   └────────────────────────┴─────────────────────┴──────────────────────┘
  //
  // WHEN TO USE EACH PATTERN:
  //   • Raw new/delete:          Legacy code, educational purposes, explicit control
  //   • std::unique_ptr<T>:      Single objects on heap, automatic cleanup
  //   • std::unique_ptr<T[]>:    Fixed-size arrays, zero overhead, automatic cleanup
  //   • std::vector<T>:          Dynamic arrays that can grow/shrink
  //
  // In modern embedded C++ (C++11+), prefer smart pointers (unique_ptr/vector)
  // for automatic resource management and exception safety. This file uses
  // manual management intentionally to document the traditional approach.
  //
  // See also: DelayEffect.h and ChorusEffect.h for modern RAII examples
  // ════════════════════════════════════════════════════════════════════════
  EffectsChain* effectsChain;

  // FreeRTOS task management
  TaskHandle_t audioTaskHandle;
  SemaphoreHandle_t paramMutex;  // Protects frequency/amplitude updates
  volatile bool taskRunning;

  // Performance monitoring
  PerformanceMonitor* performanceMonitor;  // Optional monitoring (nullptr = disabled)

  /**
   * Initialize I2S in built-in DAC mode
   * @return true if initialization succeeded, false otherwise
   */
  bool setupI2S();

  /**
   * Generate audio buffer and write to I2S
   * Called continuously by audio task
   */
  void generateAudioBuffer();

  /**
   * Static wrapper for FreeRTOS task
   * Required because FreeRTOS expects static function
   */
  static void audioTaskFunction(void* parameter);

  /**
   * Instance method called by task
   * Continuously generates audio buffers
   */
  void audioTaskLoop();
};
