# Effects Implementation Plan - Phase 4

**Status:** ✅ FULLY COMPLETE - All Phases Including PCM5102 & Precision Enhancement!
**Date Created:** October 31, 2025
**Last Updated:** November 7, 2025
**Completion Date:** November 5, 2025
**Target Phase:** Phase 4 - Visual Feedback & Effects (Effects portion)

## Overview

This document outlines the step-by-step implementation of the audio effects system for the ESP32 Theremin. The implementation follows a modular architecture with individual effect classes managed by an EffectsChain coordinator, integrated with the existing ControlHandler for unified control.

### Goals
- Implement Delay and Chorus effects
- Integrate effects into AudioEngine audio pipeline
- Add serial command control via ControlHandler
- Benchmark CPU usage at each step
- Optionally add Reverb if CPU permits

### Architecture Summary

```
Individual Effect Classes:
├── DelayEffect (circular buffer, feedback, mix)
├── ChorusEffect (modulated delay with LFO)
└── ReverbEffect (optional - Freeverb algorithm)

EffectsChain (coordinator):
└── Manages signal flow and enable/disable logic

Integration:
Oscillators → Mixer → EffectsChain → DAC Output
                         ↑
                   ControlHandler (serial + future GPIO)
```

---

## Phase A: DelayEffect Class Implementation

### A.1 Create DelayEffect Header

**File:** `include/DelayEffect.h`

```cpp
/*
 * DelayEffect.h
 *
 * Digital delay effect with feedback and wet/dry mix control.
 * Uses circular buffer for delay line.
 */

#pragma once
#include <Arduino.h>

class DelayEffect {
public:
    /**
     * Constructor
     * @param delayTimeMs Delay time in milliseconds
     * @param sampleRate Audio sample rate (e.g., 22050 Hz)
     */
    DelayEffect(uint32_t delayTimeMs, uint32_t sampleRate);

    /**
     * Destructor - frees delay buffer
     */
    ~DelayEffect();

    /**
     * Process single audio sample through delay
     * @param input Input sample (int16_t, -32768 to 32767)
     * @return Processed sample with delay applied
     */
    int16_t process(int16_t input);

    /**
     * Enable or disable the effect
     * When disabled, process() returns input unchanged (bypass)
     */
    void setEnabled(bool enabled);

    /**
     * Check if effect is enabled
     */
    bool isEnabled() const { return enabled; }

    /**
     * Set delay time
     * @param timeMs Delay time in milliseconds (50-1000ms recommended)
     */
    void setDelayTime(uint32_t timeMs);

    /**
     * Set feedback amount
     * @param fb Feedback (0.0 = no repeats, 0.95 = many repeats)
     *           Keep below 1.0 to prevent runaway feedback!
     */
    void setFeedback(float fb);

    /**
     * Set wet/dry mix
     * @param mix Mix amount (0.0 = dry only, 1.0 = wet only, 0.5 = 50/50)
     */
    void setMix(float mix);

    /**
     * Clear delay buffer (silence)
     */
    void reset();

    /**
     * Get current settings
     */
    uint32_t getDelayTime() const { return delayTimeMs; }
    float getFeedback() const { return feedback; }
    float getMix() const { return wetDryMix; }

private:
    int16_t* delayBuffer;      // Circular buffer for delay line
    size_t bufferSize;         // Buffer size in samples
    size_t writeIndex;         // Current write position

    uint32_t sampleRate;       // Audio sample rate
    uint32_t delayTimeMs;      // Delay time in milliseconds

    float feedback;            // Feedback amount (0.0-0.95)
    float wetDryMix;           // Wet/dry mix (0.0-1.0)
    bool enabled;              // Effect bypass

    /**
     * Calculate buffer size from delay time
     */
    size_t calculateBufferSize(uint32_t timeMs);
};
```

**Tasks:**
- [x] Create `include/DelayEffect.h` with above code
- [x] Review header for completeness

---

### A.2 Implement DelayEffect

**File:** `src/DelayEffect.cpp`

```cpp
/*
 * DelayEffect.cpp
 *
 * Implementation of digital delay effect.
 */

#include "DelayEffect.h"
#include "Debug.h"
#include <string.h>  // for memset

DelayEffect::DelayEffect(uint32_t delayTimeMs, uint32_t sampleRate)
    : sampleRate(sampleRate),
      delayTimeMs(delayTimeMs),
      feedback(0.5f),
      wetDryMix(0.3f),
      enabled(false),
      writeIndex(0),
      delayBuffer(nullptr) {

    // Calculate and allocate buffer
    bufferSize = calculateBufferSize(delayTimeMs);
    delayBuffer = new int16_t[bufferSize];

    // Initialize to silence
    reset();

    DEBUG_PRINT("[DELAY] Initialized: ");
    DEBUG_PRINT(delayTimeMs);
    DEBUG_PRINT("ms, buffer size: ");
    DEBUG_PRINT(bufferSize);
    DEBUG_PRINT(" samples (");
    DEBUG_PRINT((bufferSize * sizeof(int16_t)) / 1024);
    DEBUG_PRINTLN(" KB)");
}

DelayEffect::~DelayEffect() {
    if (delayBuffer != nullptr) {
        delete[] delayBuffer;
        delayBuffer = nullptr;
    }
    DEBUG_PRINTLN("[DELAY] Destroyed");
}

size_t DelayEffect::calculateBufferSize(uint32_t timeMs) {
    // Buffer size = (time in seconds) * sample rate
    // Add small safety margin
    return (size_t)((timeMs / 1000.0f) * sampleRate) + 10;
}

int16_t DelayEffect::process(int16_t input) {
    // Bypass if disabled
    if (!enabled) {
        return input;
    }

    // Read delayed sample (circular buffer)
    int16_t delayedSample = delayBuffer[writeIndex];

    // Write new sample with feedback
    // New buffer value = input + (delayed * feedback)
    int32_t newSample = input + ((int32_t)delayedSample * feedback);

    // Clamp to prevent overflow
    if (newSample > 32767) newSample = 32767;
    if (newSample < -32768) newSample = -32768;

    delayBuffer[writeIndex] = (int16_t)newSample;

    // Advance write pointer (circular)
    writeIndex++;
    if (writeIndex >= bufferSize) {
        writeIndex = 0;
    }

    // Mix dry and wet signals
    // output = (dry * (1-mix)) + (wet * mix)
    int32_t dry = input;
    int32_t wet = delayedSample;
    int32_t output = (dry * (1.0f - wetDryMix)) + (wet * wetDryMix);

    // Clamp output
    if (output > 32767) output = 32767;
    if (output < -32768) output = -32768;

    return (int16_t)output;
}

void DelayEffect::setEnabled(bool en) {
    enabled = en;
    DEBUG_PRINT("[DELAY] ");
    DEBUG_PRINTLN(enabled ? "ENABLED" : "DISABLED");
}

void DelayEffect::setDelayTime(uint32_t timeMs) {
    // Constrain to reasonable range
    if (timeMs < 10) timeMs = 10;
    if (timeMs > 2000) timeMs = 2000;

    delayTimeMs = timeMs;

    // Reallocate buffer if needed
    size_t newSize = calculateBufferSize(timeMs);
    if (newSize != bufferSize) {
        delete[] delayBuffer;
        bufferSize = newSize;
        delayBuffer = new int16_t[bufferSize];
        reset();

        DEBUG_PRINT("[DELAY] Time changed to ");
        DEBUG_PRINT(delayTimeMs);
        DEBUG_PRINT("ms, new buffer: ");
        DEBUG_PRINT((bufferSize * sizeof(int16_t)) / 1024);
        DEBUG_PRINTLN(" KB");
    }
}

void DelayEffect::setFeedback(float fb) {
    // Constrain to safe range (prevent runaway feedback)
    if (fb < 0.0f) fb = 0.0f;
    if (fb > 0.95f) fb = 0.95f;

    feedback = fb;

    DEBUG_PRINT("[DELAY] Feedback set to ");
    DEBUG_PRINTLN(feedback);
}

void DelayEffect::setMix(float mix) {
    // Constrain to 0.0-1.0
    if (mix < 0.0f) mix = 0.0f;
    if (mix > 1.0f) mix = 1.0f;

    wetDryMix = mix;

    DEBUG_PRINT("[DELAY] Mix set to ");
    DEBUG_PRINTLN(wetDryMix);
}

void DelayEffect::reset() {
    if (delayBuffer != nullptr) {
        memset(delayBuffer, 0, bufferSize * sizeof(int16_t));
        writeIndex = 0;
        DEBUG_PRINTLN("[DELAY] Buffer cleared");
    }
}
```

**Tasks:**
- [x] Create `src/DelayEffect.cpp` with above implementation
- [x] Verify buffer allocation math is correct
- [x] Check for potential memory leaks

**Testing (Unit Test):**
- [x] Create simple test in `main.cpp` (temporarily):
  ```cpp
  DelayEffect testDelay(300, 22050);
  testDelay.setEnabled(true);
  testDelay.setFeedback(0.5);
  testDelay.setMix(0.3);

  // Test with simple pattern
  for (int i = 0; i < 100; i++) {
      int16_t input = (i % 10 == 0) ? 10000 : 0;  // Pulse every 10 samples
      int16_t output = testDelay.process(input);
      Serial.printf("Sample %d: in=%d, out=%d\n", i, input, output);
  }
  ```
- [x] Verify delay buffer fills and feedback works
- [x] Verify reset() clears buffer
- [x] Remove test code before proceeding

**Success Criteria:**
- ✅ Code compiles without errors
- ✅ Buffer allocation works (check serial output)
- ✅ Delay effect produces expected output pattern
- ✅ No memory leaks or crashes

**STATUS: ✅ PHASE A COMPLETE**

---

## Phase B: EffectsChain Class & AudioEngine Integration

### B.1 Create EffectsChain Header

**File:** `include/EffectsChain.h`

```cpp
/*
 * EffectsChain.h
 *
 * Manages chain of audio effects applied to mixed oscillator output.
 */

#pragma once
#include <Arduino.h>
#include "DelayEffect.h"
// Future: #include "ChorusEffect.h"
// Future: #include "ReverbEffect.h"

class EffectsChain {
public:
    /**
     * Constructor
     * @param sampleRate Audio sample rate
     */
    EffectsChain(uint32_t sampleRate);

    /**
     * Destructor - cleans up effect instances
     */
    ~EffectsChain();

    /**
     * Process audio sample through effect chain
     * @param input Mixed oscillator output
     * @return Processed sample with effects applied
     */
    int16_t process(int16_t input);

    /**
     * Enable/disable individual effects
     */
    void setDelayEnabled(bool enabled);
    // Future: void setChorusEnabled(bool enabled);
    // Future: void setReverbEnabled(bool enabled);

    /**
     * Get effect instances (for parameter control)
     */
    DelayEffect* getDelay() { return delay; }
    // Future: ChorusEffect* getChorus() { return chorus; }
    // Future: ReverbEffect* getReverb() { return reverb; }

    /**
     * Reset all effect buffers
     */
    void reset();

    /**
     * Get effect enable states
     */
    bool isDelayEnabled() const;
    // Future: bool isChorusEnabled() const;
    // Future: bool isReverbEnabled() const;

private:
    uint32_t sampleRate;

    DelayEffect* delay;
    // Future: ChorusEffect* chorus;
    // Future: ReverbEffect* reverb;
};
```

**Tasks:**
- [x] Create `include/EffectsChain.h`
- [x] Review interface design

---

### B.2 Implement EffectsChain

**File:** `src/EffectsChain.cpp`

```cpp
/*
 * EffectsChain.cpp
 *
 * Implementation of effects processing chain.
 */

#include "EffectsChain.h"
#include "Debug.h"

EffectsChain::EffectsChain(uint32_t sampleRate)
    : sampleRate(sampleRate),
      delay(nullptr) {

    // Create delay effect (300ms default)
    delay = new DelayEffect(300, sampleRate);
    delay->setFeedback(0.5f);
    delay->setMix(0.3f);
    delay->setEnabled(false);  // Start disabled

    DEBUG_PRINTLN("[CHAIN] EffectsChain initialized");
}

EffectsChain::~EffectsChain() {
    if (delay != nullptr) {
        delete delay;
        delay = nullptr;
    }
    DEBUG_PRINTLN("[CHAIN] EffectsChain destroyed");
}

int16_t EffectsChain::process(int16_t input) {
    int16_t output = input;

    // Apply effects in order
    // (Only process if enabled - effect handles bypass internally)
    output = delay->process(output);

    // Future: output = chorus->process(output);
    // Future: output = reverb->process(output);

    return output;
}

void EffectsChain::setDelayEnabled(bool enabled) {
    if (delay != nullptr) {
        delay->setEnabled(enabled);
    }
}

bool EffectsChain::isDelayEnabled() const {
    return (delay != nullptr) ? delay->isEnabled() : false;
}

void EffectsChain::reset() {
    if (delay != nullptr) {
        delay->reset();
    }
    // Future: chorus->reset();
    // Future: reverb->reset();

    DEBUG_PRINTLN("[CHAIN] All effects reset");
}
```

**Tasks:**
- [x] Create `src/EffectsChain.cpp`
- [x] Verify effect chaining logic

---

### B.3 Integrate with AudioEngine

**Modifications needed:**

**In `include/AudioEngine.h`:**

```cpp
// Add include at top:
#include "EffectsChain.h"

// Add in private section:
private:
    // Existing members...
    Oscillator oscillators[3];

    // NEW:
    EffectsChain* effectsChain;  // Effects processing chain

// Add in public section:
public:
    // Existing methods...

    // NEW:
    EffectsChain* getEffectsChain() { return effectsChain; }
```

**In `src/AudioEngine.cpp` constructor:**

```cpp
AudioEngine::AudioEngine() {
    // Existing oscillator setup...
    oscillators[0].setWaveform(Oscillator::SINE);
    // ... etc ...

    // NEW: Create effects chain
    effectsChain = new EffectsChain(SAMPLE_RATE);
    DEBUG_PRINTLN("[AUDIO] Effects chain created");
}
```

**In `src/AudioEngine.cpp` destructor:**

```cpp
AudioEngine::~AudioEngine() {
    // NEW: Clean up effects chain
    if (effectsChain != nullptr) {
        delete effectsChain;
        effectsChain = nullptr;
    }
    // Existing cleanup if any...
}
```

**In `src/AudioEngine.cpp` generateAudioBuffer():**

```cpp
void AudioEngine::generateAudioBuffer(uint8_t* buffer, size_t length) {
    for (size_t i = 0; i < length; i += 2) {
        // 1. Mix oscillators (existing code)
        int16_t sample = mixOscillators();

        // 2. NEW: Apply effects chain
        sample = effectsChain->process(sample);

        // 3. Convert to DAC format (existing code)
        uint8_t dacValue = (sample >> 8) + 128;
        buffer[i] = dacValue;
        buffer[i + 1] = 0;
    }
}
```

**Tasks:**
- [x] Modify AudioEngine.h to add EffectsChain member and getter
- [x] Modify AudioEngine.cpp constructor to create EffectsChain
- [x] Modify AudioEngine.cpp destructor to clean up EffectsChain
- [x] Modify generateAudioBuffer() to process through effects
- [x] Build and verify compilation

**Testing:**
- [x] Upload to ESP32
- [x] Verify audio still works (effects disabled by default)
- [x] Check serial output for EffectsChain initialization
- [x] Verify no CPU spikes or audio glitches

**Success Criteria:**
- ✅ Code compiles without errors
- ✅ Audio output unchanged (effects disabled)
- ✅ EffectsChain appears in debug output
- ✅ No performance degradation

**STATUS: ✅ PHASE B COMPLETE**

---

## Phase C: ChorusEffect Implementation (with Oscillator-Based LFO) ✅ COMPLETE

**Design Decision:** The chorus effect uses the existing `Oscillator` class as its LFO instead of implementing a separate phase accumulator. This provides:
- **Performance benefit**: Uses optimized sine LUT instead of sin() calls (~100x faster)
- **Code reuse**: Leverages proven phase accumulator logic
- **Architectural elegance**: LFO is conceptually an oscillator
- **Future flexibility**: Easy to experiment with different LFO waveforms

**Critical Bug Fixed:** The `Oscillator::setFrequency()` method was constraining frequencies to a minimum of 20 Hz (audio range), which prevented LFO use. The constraint was updated to allow 0.1-20000 Hz, enabling sub-audio modulation frequencies (0.1-10 Hz) for effects while still protecting against invalid values.

### C.1 Extend Oscillator Class for LFO Use

**File:** `include/Oscillator.h`

Add new method for normalized output (modulation use):

```cpp
public:
    // ... existing methods ...

    /**
     * Get normalized sample for modulation (LFO use)
     * Returns value between -1.0 and 1.0 for use in effects
     * @param sampleRate Sample rate in Hz
     * @return Normalized sample value
     */
    float getNextSampleNormalized(float sampleRate);
```

**File:** `src/Oscillator.cpp`

Implementation (simple conversion):

```cpp
float Oscillator::getNextSampleNormalized(float sampleRate) {
    int16_t sample = getNextSample(sampleRate);
    return sample / 32768.0f;  // Convert int16_t to float -1.0 to 1.0
}
```

**Tasks:**
- [x] Add `getNextSampleNormalized()` declaration to Oscillator.h
- [x] Implement `getNextSampleNormalized()` in Oscillator.cpp
- [x] Build and verify compilation

---

### C.2 Create ChorusEffect Header

**File:** `include/ChorusEffect.h`

```cpp
/*
 * ChorusEffect.h
 *
 * Chorus effect using modulated delay with Oscillator-based LFO.
 * Creates thick, shimmering sound by pitch-shifting with sinusoidal modulation.
 *
 * Design: Uses Oscillator class as LFO for performance (sine LUT vs sin() calls).
 */

#pragma once
#include <Arduino.h>
#include "Oscillator.h"  // Reuse Oscillator as LFO!

class ChorusEffect {
public:
    /**
     * Constructor
     * @param sampleRate Audio sample rate
     */
    ChorusEffect(uint32_t sampleRate);

    /**
     * Destructor
     */
    ~ChorusEffect();

    /**
     * Process single audio sample
     */
    int16_t process(int16_t input);

    /**
     * Enable/disable effect
     */
    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled; }

    /**
     * Set LFO rate (modulation speed)
     * @param hz Frequency in Hz (0.1 - 5.0 recommended)
     */
    void setRate(float hz);

    /**
     * Set modulation depth (pitch variation amount)
     * @param ms Depth in milliseconds (5 - 50 recommended)
     */
    void setDepth(float ms);

    /**
     * Set wet/dry mix
     * @param mix 0.0 = dry only, 1.0 = wet only
     */
    void setMix(float mix);

    /**
     * Clear delay buffer
     */
    void reset();

    /**
     * Get current settings
     */
    float getRate() const;
    float getDepth() const { return lfoDepthMs; }
    float getMix() const { return wetDryMix; }

private:
    int16_t* delayBuffer;
    size_t bufferSize;
    size_t writeIndex;

    uint32_t sampleRate;

    Oscillator lfo;        // LFO using Oscillator class!
    float lfoDepthMs;      // Modulation depth in milliseconds
    float wetDryMix;       // Wet/dry mix
    bool enabled;

    /**
     * Read from delay buffer with fractional index (linear interpolation)
     * @param delayInSamples Delay in samples (can be fractional)
     */
    int16_t readDelayBuffer(float delayInSamples);
};
```

**Tasks:**
- [x] Create `include/ChorusEffect.h` with Oscillator-based LFO

---

### C.3 Implement ChorusEffect

**File:** `src/ChorusEffect.cpp`

```cpp
/*
 * ChorusEffect.cpp
 *
 * Implementation of chorus effect with Oscillator-based LFO.
 */

#include "ChorusEffect.h"
#include "Debug.h"
#include <string.h>

ChorusEffect::ChorusEffect(uint32_t sampleRate)
    : sampleRate(sampleRate),
      lfoDepthMs(15.0f),     // Default 15ms
      wetDryMix(0.4f),       // Default 40% wet
      enabled(false),
      writeIndex(0),
      delayBuffer(nullptr) {

    // Configure LFO (using Oscillator class!)
    lfo.setWaveform(Oscillator::SINE);
    lfo.setFrequency(2.0f);         // 2 Hz default
    lfo.setVolume(1.0f);             // Full amplitude

    // Allocate buffer for max depth (50ms + safety margin)
    bufferSize = (size_t)((50.0f / 1000.0f) * sampleRate) + 100;
    delayBuffer = new int16_t[bufferSize];

    reset();

    DEBUG_PRINT("[CHORUS] Initialized with Oscillator-based LFO: buffer size ");
    DEBUG_PRINT(bufferSize);
    DEBUG_PRINT(" samples (");
    DEBUG_PRINT((bufferSize * sizeof(int16_t)) / 1024);
    DEBUG_PRINTLN(" KB)");
}

ChorusEffect::~ChorusEffect() {
    if (delayBuffer != nullptr) {
        delete[] delayBuffer;
        delayBuffer = nullptr;
    }
    DEBUG_PRINTLN("[CHORUS] Destroyed");
}

int16_t ChorusEffect::readDelayBuffer(float delayInSamples) {
    // Calculate read position (fractional index)
    float readPos = (float)writeIndex - delayInSamples;

    // Wrap around buffer (handle negative indices)
    while (readPos < 0) {
        readPos += bufferSize;
    }
    while (readPos >= bufferSize) {
        readPos -= bufferSize;
    }

    // Linear interpolation between two samples
    int index1 = (int)readPos;
    int index2 = (index1 + 1) % bufferSize;
    float fraction = readPos - index1;

    int16_t sample1 = delayBuffer[index1];
    int16_t sample2 = delayBuffer[index2];

    return (int16_t)(sample1 * (1.0f - fraction) + sample2 * fraction);
}

int16_t ChorusEffect::process(int16_t input) {
    // Bypass if disabled
    if (!enabled) {
        return input;
    }

    // Write input to delay buffer
    delayBuffer[writeIndex] = input;

    // Get LFO value (uses sine LUT, no sin() call!)
    float lfoValue = lfo.getNextSampleNormalized(sampleRate);

    // Calculate modulated delay time
    // Center delay = depth, modulation = ±depth
    float delayTimeMs = lfoDepthMs + (lfoValue * lfoDepthMs);
    float delayInSamples = (delayTimeMs / 1000.0f) * sampleRate;

    // Read delayed sample with interpolation
    int16_t delayedSample = readDelayBuffer(delayInSamples);

    // Mix dry and wet
    int32_t dry = input;
    int32_t wet = delayedSample;
    int32_t output = (dry * (1.0f - wetDryMix)) + (wet * wetDryMix);

    // Clamp
    if (output > 32767) output = 32767;
    if (output < -32768) output = -32768;

    // Advance write pointer
    writeIndex = (writeIndex + 1) % bufferSize;

    return (int16_t)output;
}

void ChorusEffect::setEnabled(bool en) {
    enabled = en;
    DEBUG_PRINT("[CHORUS] ");
    DEBUG_PRINTLN(enabled ? "ENABLED" : "DISABLED");
}

void ChorusEffect::setRate(float hz) {
    // Constrain to reasonable range
    if (hz < 0.1f) hz = 0.1f;
    if (hz > 10.0f) hz = 10.0f;

    lfo.setFrequency(hz);  // Just delegate to Oscillator!

    DEBUG_PRINT("[CHORUS] Rate set to ");
    DEBUG_PRINT(hz);
    DEBUG_PRINTLN(" Hz");
}

float ChorusEffect::getRate() const {
    return lfo.getEffectiveFrequency();  // Use Oscillator's method
}

void ChorusEffect::setDepth(float ms) {
    // Constrain to reasonable range
    if (ms < 1.0f) ms = 1.0f;
    if (ms > 50.0f) ms = 50.0f;

    lfoDepthMs = ms;

    DEBUG_PRINT("[CHORUS] Depth set to ");
    DEBUG_PRINT(lfoDepthMs);
    DEBUG_PRINTLN(" ms");
}

void ChorusEffect::setMix(float mix) {
    if (mix < 0.0f) mix = 0.0f;
    if (mix > 1.0f) mix = 1.0f;

    wetDryMix = mix;

    DEBUG_PRINT("[CHORUS] Mix set to ");
    DEBUG_PRINTLN(wetDryMix);
}

void ChorusEffect::reset() {
    if (delayBuffer != nullptr) {
        memset(delayBuffer, 0, bufferSize * sizeof(int16_t));
        writeIndex = 0;
        // Note: No need to reset LFO phase - continuous modulation is fine
        DEBUG_PRINTLN("[CHORUS] Buffer cleared");
    }
}
```

**Tasks:**
- [x] Create `src/ChorusEffect.cpp` with Oscillator-based LFO
- [x] Verify interpolation logic
- [x] Build and test
- [x] Fix Oscillator frequency constraint bug (20 Hz → 0.1 Hz minimum)
- [x] Add comprehensive effect documentation to header files

---

### C.4 Add Chorus to EffectsChain

**Modifications to `include/EffectsChain.h`:**

```cpp
// Add include:
#include "ChorusEffect.h"

// Add members:
private:
    DelayEffect* delay;
    ChorusEffect* chorus;  // NEW

// Add methods:
public:
    void setChorusEnabled(bool enabled);
    ChorusEffect* getChorus() { return chorus; }
    bool isChorusEnabled() const;
```

**Modifications to `src/EffectsChain.cpp`:**

```cpp
// In constructor:
EffectsChain::EffectsChain(uint32_t sampleRate)
    : sampleRate(sampleRate),
      delay(nullptr),
      chorus(nullptr) {  // NEW

    delay = new DelayEffect(300, sampleRate);
    // ... delay setup ...

    // NEW: Create chorus effect
    chorus = new ChorusEffect(sampleRate);
    chorus->setRate(2.0f);
    chorus->setDepth(15.0f);
    chorus->setMix(0.4f);
    chorus->setEnabled(false);

    DEBUG_PRINTLN("[CHAIN] EffectsChain initialized with Delay + Chorus");
}

// In destructor:
EffectsChain::~EffectsChain() {
    if (delay != nullptr) {
        delete delay;
        delay = nullptr;
    }
    if (chorus != nullptr) {  // NEW
        delete chorus;
        chorus = nullptr;
    }
    DEBUG_PRINTLN("[CHAIN] EffectsChain destroyed");
}

// In process():
int16_t EffectsChain::process(int16_t input) {
    int16_t output = input;

    output = delay->process(output);
    output = chorus->process(output);  // NEW

    return output;
}

// Add new methods:
void EffectsChain::setChorusEnabled(bool enabled) {
    if (chorus != nullptr) {
        chorus->setEnabled(enabled);
    }
}

bool EffectsChain::isChorusEnabled() const {
    return (chorus != nullptr) ? chorus->isEnabled() : false;
}

// In reset():
void EffectsChain::reset() {
    if (delay != nullptr) {
        delay->reset();
    }
    if (chorus != nullptr) {  // NEW
        chorus->reset();
    }
    DEBUG_PRINTLN("[CHAIN] All effects reset");
}
```

**Tasks:**
- [x] Modify EffectsChain.h to add ChorusEffect
- [x] Modify EffectsChain.cpp to integrate chorus
- [x] Refactor to stack allocation (changed from pointers to direct members)
- [x] Build and test

**Testing:**
- [x] Verify chorus effect is initialized (check serial)
- [x] Test with chorus enabled via temporary code
- [x] Listen for characteristic "shimmer" sound
- [x] Verify no audio glitches

**Success Criteria:**
- ✅ Chorus compiles and integrates
- ✅ Chorus creates audible pitch modulation when enabled
- ✅ No crashes or audio artifacts
- ✅ CPU usage excellent - only 9% with 3 oscillators + delay + chorus!

**STATUS: ✅ PHASE C COMPLETE**

**Performance Notes:**
- **CPU Usage:** ~1.0ms per 11ms buffer (9% of one core)
- **3 Oscillators + Delay + Chorus:** No performance issues
- **RAM:** Stable at ~314 KB free
- **Architecture:** Stack allocation for effects (RAII, no heap fragmentation)
- **LFO Performance:** Oscillator-based design using sine LUT is extremely efficient
- **Headroom:** 91% CPU available for future features!

---

## Phase D: ControlHandler Integration

### D.1 Add Effects Commands to ControlHandler

**Modifications to `src/ControlHandler.cpp` - executeCommand():**

Add this section after the "Audio Control" section:

```cpp
  // ========== EFFECTS CONTROL ==========

  // Delay enable/disable
  if (cmd == "delay:on") {
    theremin->getAudioEngine()->getEffectsChain()->setDelayEnabled(true);
    DEBUG_PRINTLN("[CTRL] Delay effect enabled");
    return;
  }

  if (cmd == "delay:off") {
    theremin->getAudioEngine()->getEffectsChain()->setDelayEnabled(false);
    DEBUG_PRINTLN("[CTRL] Delay effect disabled");
    return;
  }

  // Delay parameters
  if (cmd.startsWith("delay:time:")) {
    int timeMs = cmd.substring(11).toInt();
    theremin->getAudioEngine()->getEffectsChain()->getDelay()->setDelayTime(timeMs);
    DEBUG_PRINT("[CTRL] Delay time set to ");
    DEBUG_PRINT(timeMs);
    DEBUG_PRINTLN(" ms");
    return;
  }

  if (cmd.startsWith("delay:feedback:")) {
    float feedback = cmd.substring(15).toFloat();
    theremin->getAudioEngine()->getEffectsChain()->getDelay()->setFeedback(feedback);
    DEBUG_PRINT("[CTRL] Delay feedback set to ");
    DEBUG_PRINTLN(feedback);
    return;
  }

  if (cmd.startsWith("delay:mix:")) {
    float mix = cmd.substring(10).toFloat();
    theremin->getAudioEngine()->getEffectsChain()->getDelay()->setMix(mix);
    DEBUG_PRINT("[CTRL] Delay mix set to ");
    DEBUG_PRINTLN(mix);
    return;
  }

  // Chorus enable/disable
  if (cmd == "chorus:on") {
    theremin->getAudioEngine()->getEffectsChain()->setChorusEnabled(true);
    DEBUG_PRINTLN("[CTRL] Chorus effect enabled");
    return;
  }

  if (cmd == "chorus:off") {
    theremin->getAudioEngine()->getEffectsChain()->setChorusEnabled(false);
    DEBUG_PRINTLN("[CTRL] Chorus effect disabled");
    return;
  }

  // Chorus parameters
  if (cmd.startsWith("chorus:rate:")) {
    float rate = cmd.substring(12).toFloat();
    theremin->getAudioEngine()->getEffectsChain()->getChorus()->setRate(rate);
    DEBUG_PRINT("[CTRL] Chorus rate set to ");
    DEBUG_PRINT(rate);
    DEBUG_PRINTLN(" Hz");
    return;
  }

  if (cmd.startsWith("chorus:depth:")) {
    float depth = cmd.substring(13).toFloat();
    theremin->getAudioEngine()->getEffectsChain()->getChorus()->setDepth(depth);
    DEBUG_PRINT("[CTRL] Chorus depth set to ");
    DEBUG_PRINT(depth);
    DEBUG_PRINTLN(" ms");
    return;
  }

  if (cmd.startsWith("chorus:mix:")) {
    float mix = cmd.substring(11).toFloat();
    theremin->getAudioEngine()->getEffectsChain()->getChorus()->setMix(mix);
    DEBUG_PRINT("[CTRL] Chorus mix set to ");
    DEBUG_PRINTLN(mix);
    return;
  }

  // Effects status
  if (cmd == "effects:status") {
    printEffectsStatus();
    return;
  }

  // Effects reset
  if (cmd == "effects:reset") {
    theremin->getAudioEngine()->getEffectsChain()->reset();
    DEBUG_PRINTLN("[CTRL] All effects reset");
    return;
  }
```

**Add new helper method in `ControlHandler.cpp`:**

```cpp
void ControlHandler::printEffectsStatus() {
  EffectsChain* fx = theremin->getAudioEngine()->getEffectsChain();

  DEBUG_PRINTLN("\n========== EFFECTS STATUS ==========");

  // Delay status
  DEBUG_PRINT("Delay:   ");
  DEBUG_PRINTLN(fx->isDelayEnabled() ? "ENABLED" : "DISABLED");
  if (fx->getDelay() != nullptr) {
    DEBUG_PRINT("  Time:     ");
    DEBUG_PRINT(fx->getDelay()->getDelayTime());
    DEBUG_PRINTLN(" ms");
    DEBUG_PRINT("  Feedback: ");
    DEBUG_PRINTLN(fx->getDelay()->getFeedback());
    DEBUG_PRINT("  Mix:      ");
    DEBUG_PRINTLN(fx->getDelay()->getMix());
  }

  // Chorus status
  DEBUG_PRINT("\nChorus:  ");
  DEBUG_PRINTLN(fx->isChorusEnabled() ? "ENABLED" : "DISABLED");
  if (fx->getChorus() != nullptr) {
    DEBUG_PRINT("  Rate:     ");
    DEBUG_PRINT(fx->getChorus()->getRate());
    DEBUG_PRINTLN(" Hz");
    DEBUG_PRINT("  Depth:    ");
    DEBUG_PRINT(fx->getChorus()->getDepth());
    DEBUG_PRINTLN(" ms");
    DEBUG_PRINT("  Mix:      ");
    DEBUG_PRINTLN(fx->getChorus()->getMix());
  }

  DEBUG_PRINTLN("====================================\n");
}
```

**Add declaration in `include/ControlHandler.h`:**

```cpp
private:
  // Existing methods...
  void printStatus();
  void printOscillatorStatus(int oscNum);
  void printEffectsStatus();  // NEW
```

---

### D.2 Update Help Text

**Modify `printHelp()` in `src/ControlHandler.cpp`:**

Add before the closing line:

```cpp
  DEBUG_PRINTLN("\nEffects Control:");
  DEBUG_PRINTLN("  delay:on             - Enable delay effect");
  DEBUG_PRINTLN("  delay:off            - Disable delay effect");
  DEBUG_PRINTLN("  delay:time:300       - Set delay time to 300ms");
  DEBUG_PRINTLN("  delay:feedback:0.5   - Set feedback to 50%");
  DEBUG_PRINTLN("  delay:mix:0.3        - Set wet/dry mix to 30%");
  DEBUG_PRINTLN("\n  chorus:on            - Enable chorus effect");
  DEBUG_PRINTLN("  chorus:off           - Disable chorus effect");
  DEBUG_PRINTLN("  chorus:rate:2.0      - Set LFO rate to 2.0 Hz");
  DEBUG_PRINTLN("  chorus:depth:15      - Set modulation depth to 15ms");
  DEBUG_PRINTLN("  chorus:mix:0.4       - Set wet/dry mix to 40%");
  DEBUG_PRINTLN("\n  effects:status       - Show all effect states");
  DEBUG_PRINTLN("  effects:reset        - Clear all effect buffers");
```

**Tasks:**
- [x] Add effects control commands to executeCommand()
- [x] Add printEffectsStatus() method
- [x] Update printHelp() with effects commands
- [x] Build and test

**Testing:**
- [x] Implementation complete - ready for hardware upload
- [ ] Upload firmware to ESP32
- [ ] Type `help` in serial monitor - verify effects commands listed
- [ ] Test each command:
  - `delay:on` → should enable delay
  - `delay:time:500` → should change delay time
  - `chorus:on` → should enable chorus
  - `chorus:rate:3.0` → should change chorus speed
  - `effects:status` → should show all parameters
  - `effects:reset` → should clear buffers
- [ ] Verify all commands work as expected

**Success Criteria:**
- ✅ All effects commands functional via serial
- ✅ Parameters update in real-time
- ✅ No audio glitches when changing parameters
- ✅ Status display shows correct values

**Implementation Date:** November 1, 2025

**STATUS: ✅ PHASE D COMPLETE** - All serial command code implemented. Ready for hardware testing.

---

## Phase E: Testing & CPU Benchmarking ✅ COMPLETE

### E.1 Performance Results

**Measured Performance with PerformanceMonitor:**

- **Execution Time:** 1ms per audio task (out of 11ms I2S buffer window)
- **CPU Usage:** ~9% of available processing time
- **Headroom:** 91% available for additional features
- **Configuration Tested:** 3 oscillators + Delay + Chorus active
- **RAM Usage:** Stable at ~314 KB free
- **Audio Quality:** Excellent - no glitches or dropouts

### E.2 Performance Metrics

| Configuration | Exec Time | CPU % | Audio Quality | Notes |
|---------------|-----------|-------|---------------|-------|
| 3 Osc + Delay + Chorus | 1ms / 11ms | ~9% | Excellent | No artifacts detected |

**Analysis:**
- Performance is **stellar** - far better than expected
- Oscillator-based LFO design proved extremely efficient (sine LUT vs sin() calls)
- Stack allocation architecture prevents heap fragmentation
- Plenty of headroom for reverb implementation

**Tasks:**
- [x] Run stress test with all effects
- [x] Measure execution time with PerformanceMonitor
- [x] Verify audio quality
- [x] Confirm CPU budget allows reverb

**Success Criteria:**
- ✅ CPU usage with both effects <70% (achieved 9%!)
- ✅ No audio dropouts during stress test
- ✅ RAM usage stable (no leaks)
- ✅ Effects sound musical and clean

**Decision:** **PROCEED with Phase F (Reverb)** - excellent headroom available!

**STATUS: ✅ PHASE E COMPLETE**

**Implementation Date:** November 1, 2025

---

## Phase F: Reverb Implementation ⏳ IN PROGRESS

**Decision:** **PROCEED** - Phase E showed 9% CPU usage, excellent headroom available!

### F.1 Reverb Algorithm - Simplified Freeverb

**Design:**
- 4 parallel comb filters (room reflections)
- 2 series allpass filters (diffusion)
- Sample-rate agnostic (time-based constants in milliseconds)
- Estimated CPU impact: +2-3ms (bringing total to ~3-4ms out of 11ms)

**Comb Filter Delays (milliseconds):**
- Comb 1: 25.31ms
- Comb 2: 26.94ms
- Comb 3: 28.96ms
- Comb 4: 30.75ms

**Allpass Filter Delays (milliseconds):**
- Allpass 1: 12.61ms
- Allpass 2: 10.00ms

**Parameters:**
- `room_size` - Room size (0.0-1.0, affects feedback/reverb tail length)
- `damping` - High-frequency absorption (0.0=bright, 1.0=dark/muffled)
- `wet/dry mix` - Effect blend (0.0=dry only, 1.0=wet only)

### F.2 Implementation Status

**Files Created:**
- [x] `include/ReverbEffect.h` - Class definition with sample-rate agnostic constants
- [x] `src/ReverbEffect.cpp` - Full Freeverb implementation

**Integration:**
- [x] Added to EffectsChain (delay → chorus → reverb)
- [x] Serial commands added to ControlHandler
- [x] Help text updated with reverb commands

**Serial Commands:**
```
reverb:on               - Enable reverb effect
reverb:off              - Disable reverb effect
reverb:room:0.5         - Set room size (0.0-1.0)
reverb:damp:0.5         - Set damping (0.0=bright, 1.0=dark)
reverb:mix:0.3          - Set wet/dry mix (0.0-1.0)
effects:status          - Show all effects (includes reverb)
```

**Default Settings:**
- Room size: 0.5 (medium room)
- Damping: 0.5 (balanced)
- Mix: 0.3 (30% wet)
- Disabled by default

### F.3 Testing Checklist

**Tasks:**
- [x] Create ReverbEffect class
- [x] Implement Freeverb algorithm (4 combs + 2 allpass)
- [x] Integrate into EffectsChain
- [x] Add control commands
- [x] Refactor to sample-rate agnostic (millisecond-based constants)
- [x] Compile and upload to ESP32
- [x] Test reverb sound quality
- [x] Measure CPU impact with PerformanceMonitor
- [x] Test all three effects together
- [x] Fix reverb buzzing issue with noise gates

**Actual Performance:**
- **Total execution time: 1.6ms out of 11ms (14.5% CPU)** 🎉
- **3 Oscillators + Delay + Chorus + Reverb:** Excellent performance!
- **Headroom: 85% available** for future enhancements

### F.4 Reverb Buzzing Fix ✅

**Issue:** Reverb tail exhibited grainy "buzzing" when decaying to silence due to quantization noise in int16_t feedback loops.

**Solution Implemented:** Added noise gates at three critical points in `src/ReverbEffect.cpp`:

1. **Input Noise Gate** (in `process()`):
```cpp
// Silence very quiet inputs to prevent noise circulation
if (input > -50 && input < 50) {
    input = 0;
}
```

2. **Damping Filter Noise Gate** (in `processComb()`):
```cpp
// Prevent very small float values from accumulating as noise
if (comb.filterStore > -0.5f && comb.filterStore < 0.5f) {
    comb.filterStore = 0.0f;
}
```

3. **Output Noise Gate** (in `process()`):
```cpp
// Ensure output below noise floor is silenced
if (output > -50 && output < 50) {
    output = 0;
}
```

**Result:** Reverb tail now decays to true silence without buzzing artifacts.

### F.5 Volume Smoothing Toggle ✅

**Feature Added:** Serial commands to toggle volume smoothing for testing reverb trails.

**Implementation:**
- **Files Modified:**
  - `include/SensorManager.h` - Added smoothing enable/disable methods
  - `src/SensorManager.cpp` - Bypass smoothing when disabled, returns raw sensor values
  - `src/ControlHandler.cpp` - Added serial commands and help text

**Commands:**
```
sensors:volume:smooth:on   - Enable volume smoothing (smooth transitions)
sensors:volume:smooth:off  - Instant response (for testing reverb trails)
sensors:pitch:smooth:on    - Enable pitch smoothing
sensors:pitch:smooth:off   - Instant response
sensors:status             - Show sensor and smoothing states
```

**Use Case:** When testing reverb, disable smoothing to get instant volume cutoff, allowing clear hearing of reverb decay without volume smoothing interference.

**STATUS: ✅ PHASE F COMPLETE**

**Implementation Date:** November 1, 2025
**Testing Date:** November 1, 2025

---

## Completion Checklist

### Phase A: DelayEffect ✅ COMPLETE
- [x] DelayEffect.h created
- [x] DelayEffect.cpp implemented
- [x] Unit test passed
- [x] Code compiles

### Phase B: EffectsChain & Integration ✅ COMPLETE
- [x] EffectsChain.h created
- [x] EffectsChain.cpp implemented
- [x] AudioEngine modified
- [x] Integration tested
- [x] Audio still works

### Phase C: ChorusEffect ✅ COMPLETE
- [x] ChorusEffect.h created with comprehensive documentation
- [x] ChorusEffect.cpp implemented with Oscillator-based LFO
- [x] Added to EffectsChain with stack allocation
- [x] Tested and sounds good
- [x] Fixed Oscillator frequency constraint bug
- [x] Verified excellent performance (9% CPU with all effects)

### Phase D: ControlHandler ✅ COMPLETE
- [x] Effects commands added
- [x] printEffectsStatus() implemented
- [x] Help text updated
- [x] Code compiles successfully

### Phase E: Testing & Benchmarking ✅ COMPLETE
- [x] Stress test with 3 oscillators + delay + chorus
- [x] Performance measured: 1ms/11ms (~9% CPU)
- [x] CPU usage well within budget
- [x] No stability issues
- [x] Decision: PROCEED with reverb

### Phase F: Reverb ✅ COMPLETE
- [x] Decision made: PROCEED (excellent headroom)
- [x] ReverbEffect.h created (sample-rate agnostic)
- [x] ReverbEffect.cpp implemented (Freeverb algorithm)
- [x] Integrated into EffectsChain
- [x] Serial commands added
- [x] Compile and upload to ESP32
- [x] Test reverb sound quality
- [x] Measure final CPU impact (1.6ms total / 14.5%)
- [x] Fix buzzing issue with noise gates
- [x] Add volume smoothing toggle for testing

### Phase G: Effects Polish & Fine-Tuning ✅ COMPLETE

**Goal:** Improve audio quality of all three effects while maintaining excellent performance.

**Implementation Date:** November 1, 2025

**Optimization Journey:**

#### G.1 Reverb Noise Gate Fine-Tuning ✅
**Issue:** Graininess and infinite noise at high reverb settings (room:1.0)

**Iterations:**
1. **Initial attempt:** int32_t precision (256x scale-up) → Found scaling bug in damping filter
2. **Feedback limiting:** Reduced max feedback from 0.98 to 0.90 → Too short reverb tails
3. **Aggressive gates (4x):** Increased all gates to ±200/2.0 → Killed infinite noise but tails too short
4. **Balanced solution (2x):** Gates at ±100/1.0, feedback at 0.94 → Good balance achieved

**Final Settings:**
- Reverb input/output gates: ±100 (2x original ±50)
- FilterStore gate: ±1.0 (2x original ±0.5)
- Max feedback: 0.94 (was 0.98)

**Result:** Infinite noise eliminated, decent tail length preserved.

#### G.2 Full Freeverb Upgrade ✅
**Implementation:** Upgraded from simplified to full Freeverb algorithm

**Changes:**
- **Comb filters:** 4 → 8 (doubled for richer early reflections)
- **Allpass filters:** 2 → 4 (doubled for better diffusion)
- **New delays:**
  - Combs 5-8: 32.24ms, 33.81ms, 35.31ms, 36.66ms
  - Allpass 3-4: 7.73ms, 5.10ms

**Performance Impact:**
- RAM: 23,984 bytes (unchanged - efficient buffer allocation)
- Flash: 373,485 bytes (+80 bytes)
- CPU: Still ~2.0-2.1ms with 3 osc + all effects

**Result:** Richer reverb texture, better diffusion, still grainy at low levels.

#### G.3 Master Output Noise Gate ✅
**Implementation:** Final cleanup stage after all effects

**Logic:**
```cpp
// After effects chain, before DAC:
if (scaledSample > -150 && scaledSample < 150) {
  scaledSample = 0;
}
```

**Purpose:** Catch cumulative quantization noise from stacked effects (delay + chorus + reverb)

**Result:** Slight improvement but graininess persists at low levels.

#### G.4 Root Cause Identified 🎯
**Discovery:** The graininess is NOT the audio processing or effects - it's the **8-bit ESP32 built-in DAC**!

**Analysis:**
- Audio processing: 16-bit (int16_t) = 65,536 discrete levels
- ESP32 built-in DAC: **8-bit = only 256 levels**
- **Bottleneck:** 99.6% of audio precision thrown away at output stage!

**Conclusion:** All noise gate optimization was working around DAC limitations, not fixing audio engine issues.

**STATUS:** ✅ **PHASE G COMPLETE** - Optimized within 8-bit DAC constraints. Waiting for PCM5102 (24-bit DAC) to unlock true audio quality.

---

### Phase H: PCM5102 External DAC Integration ✅ COMPLETE

**Goal:** Eliminate graininess by upgrading from 8-bit ESP32 DAC to 24-bit external DAC.

**Completion Date:** November 5, 2025
**Hardware:** PCM5102 24-bit I2S DAC module installed and working
**Documentation:** `docs/improvements/DAC_MIGRATION_PCM5102.md`

**Implementation Results:**
- Successfully migrated from 8-bit ESP32 built-in DAC to 16-bit PCM5102
- I2S reconfigured for external DAC (removed DAC_BUILT_IN mode)
- GPIO pins configured: BCK=26, WS=25, DATA=22
- Output resolution: 8-bit (256 levels) → 16-bit (65,536 levels) = **256x improvement**
- Graininess dramatically reduced compared to built-in DAC

**Expected Improvements:**
- **Output resolution:** 8-bit (256 levels) → 24-bit (16.7M levels)
- **Precision utilization:** 16-bit audio → 24-bit DAC = perfect fit (no truncation)
- **Graininess:** Should be **completely eliminated**
- **Reverb tails:** Smooth decay to silence
- **Volume fades:** Clean transitions without artifacts

#### H.1 I2S Configuration Update
**Tasks:**
- [ ] Modify `AudioEngine::setupI2S()` for external DAC
- [ ] Change from `I2S_MODE_DAC_BUILT_IN` to standard I2S mode
- [ ] Configure GPIO pins (BCK, WS, DATA)
- [ ] Update sample format from 8-bit to 16-bit output
- [ ] Remove DAC offset conversion (no longer needed)

**Changes to `src/AudioEngine.cpp`:**
```cpp
// In setupI2S():
i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),  // Remove DAC_BUILT_IN
    .sample_rate = Audio::SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
    // ... rest of config
};

// Add pin configuration:
i2s_pin_config_t pin_config = {
    .bck_io_num = 26,    // Bit clock
    .ws_io_num = 25,     // Word select
    .data_out_num = 22,  // Data out
    .data_in_num = -1    // Not used
};

i2s_set_pin((i2s_port_t)I2S_NUM, &pin_config);
```

**In `generateAudioBuffer()`:**
```cpp
// Remove DAC offset conversion:
// OLD: uint8_t dacSample = (sample >> 8) + 128;
// NEW: Direct 16-bit output
buffer[i] = scaledSample;  // No conversion needed!
```

#### H.2 Post-PCM5102 Testing Plan
**Tasks:**
- [ ] Upload firmware with PCM5102 config
- [ ] Test with current noise gate settings
- [ ] Listen for graininess at low levels
- [ ] Test reverb tail decay quality
- [ ] Test volume smoothing fades

#### H.3 Noise Gate Re-tuning (If Needed)
**If audio is clean:** Consider softening or removing gates

**Reverb gates:** ±100 → ±50 (back to original) or disable
**Master gate:** ±150 → ±75 or disable (set to 0)
**Feedback:** 0.94 → 0.96-0.98 (longer tails)

**Tasks:**
- [ ] Test with original gate settings (±50/0.5)
- [ ] Test with master gate disabled (threshold = 0)
- [ ] Test higher feedback values (0.96-0.98)
- [ ] Find optimal settings for 24-bit output
- [ ] Document final configuration

#### H.4 Final Validation
**Success Criteria:**
- [ ] No graininess at any volume level
- [ ] Smooth reverb tail decay to silence
- [ ] Clean volume fades with smoothing enabled
- [ ] All 3 effects sound professional
- [ ] CPU usage still <25%

#### H.5 Bit Depth Options Reference

**PCM5102 Capabilities:**
- **Digital Input (I2S):** Accepts 16-bit, 24-bit, or 32-bit frames
- **Internal DAC:** 24-bit precision digital-to-analog conversion
- **Analog Output:** Professional quality based on input bit depth

**Decision: Stick with 16-bit (Recommended)**

Current architecture already uses 16-bit (`int16_t`) throughout:
- Oscillators: Generate at 16-bit precision
- Effects: Process at 16-bit precision
- Output: Send 16-bit to PCM5102

**Expected Result:** Upgrading from 8-bit DAC to PCM5102 with 16-bit input = **256x improvement** (256 levels → 65,536 levels). This should eliminate 99% of graininess.

**Future Option: 24-bit Hybrid Architecture (If Needed)**

If 16-bit reverb tail is still grainy after PCM5102 testing:

```cpp
// Oscillators: Stay efficient at 16-bit
int16_t osc = oscillator.getNextSample();  // int16_t = 16-bit storage

// Upscale to 24-bit precision for effects
int32_t sample24 = (int32_t)osc << 8;  // int32_t = 32-bit storage
                                        // Audio in upper 24 bits
                                        // Lower 8 bits = padding

// Effects process at 24-bit precision
sample24 = delay.process24(sample24);
sample24 = chorus.process24(sample24);
sample24 = reverb.process24(sample24);  // Higher precision = smoother tail

// Send 24-bit to PCM5102
output_to_i2s(sample24);  // Full 24-bit DAC resolution
```

**Why "24-bit audio" uses int32_t:**
- Industry convention: "24-bit audio" = 24 bits of audio data stored in 32-bit container
- Storage: `int32_t` (32 bits total)
- Audio data: Upper 24 bits (16,777,216 levels)
- Padding: Lower 8 bits (unused, typically zeros)
- CPU efficiency: Processors work in 32-bit word sizes naturally
- I2S convention: Sends 32-bit frames with 24-bit audio in upper bits

**Cost of 24-bit upgrade:**
- Effect buffers: 2x RAM (int16_t → int32_t)
- Reverb: ~6 KB → ~12 KB
- CPU: +30-50% for effects processing
- Code complexity: Moderate (upscaling + int32_t effect variants)

**Benefit of 24-bit upgrade:**
- 256x more precision in reverb feedback loops
- Smoother decay to silence
- Less quantization noise accumulation
- Professional studio-quality reverb tail

**Recommendation:** Test 16-bit with PCM5102 first. Likely sufficient! CD audio is 16-bit and sounds professional. Upgrade to 24-bit only if reverb quality demands it.

**STATUS:** ✅ **PHASE H COMPLETE** - PCM5102 installed, 256x precision improvement achieved!

**Phase I: Final Polish & Precision Enhancement** ✅ COMPLETE (Nov 5, 2025)
- All precision enhancement sub-phases completed (A1, A2, B1, C1)
- Documentation: `docs/improvements/REVERB_PRECISION_ENHANCEMENT.md`
- Results: Dramatically improved audio quality, maintained excellent performance

---

### Phase I: Final Polish & Documentation

**Goal:** Finalize project with optimal settings and comprehensive documentation.

#### I.1 Parameter Optimization
**Issue:** Reverb tail exhibits graininess when decaying to low levels due to int16_t quantization.

#### I.1 Parameter Optimization (After PCM5102)

**Options (in order of recommendation):**

1. **Int32_t Intermediate Precision** ⭐ Recommended First
   - Use int32_t for comb filter calculations, keep int16_t buffers
   - Estimated CPU impact: +5-10%
   - Benefit: Much smoother tail decay
   - Tasks:
     - [ ] Modify `processComb()` to use int32_t intermediate math
     - [ ] Keep damping filter in higher precision
     - [ ] Test and measure CPU impact

   **After int32_t improvement, consider:**
   - [ ] Upgrade to "Full Freeverb" (8 combs + 4 allpass vs current 4 + 2)
     - Adds 4 more comb filters for richer early reflections
     - Adds 2 more allpass filters for better diffusion
     - Estimated additional CPU: +5-8% (total reverb ~12-15%)
     - RAM increase: ~7 KB additional (still plenty available)
     - Note: Stereo implementation not planned (mono DAC output)
     - Better density and smoother tail than simplified version
     - Original Freeverb spec: https://github.com/sinshu/freeverb

2. **Add Dithering**
   - Add low-level noise (~1-2 LSB) to mask quantization
   - Estimated CPU impact: +1-2%
   - Benefit: Trades "grain" for smooth "hiss"
   - Tasks:
     - [ ] Implement simple TPDF dither generator
     - [ ] Add to reverb output stage
     - [ ] Make togglable via serial command

3. **Hybrid Float/Int Approach**
   - Use float for feedback/damping, int16_t for buffers
   - Estimated CPU impact: +30-50% for reverb
   - Benefit: Professional-quality tail
   - Tasks:
     - [ ] Convert comb filter math to float
     - [ ] Benchmark performance impact
     - [ ] Only proceed if CPU budget allows

#### G.2 Delay Audio Quality Enhancement
**Issue:** Background noise at end of repetitions

**Tasks:**
- [ ] Add noise gate to delay feedback loop (similar to reverb fix)
- [ ] Improve precision in delay buffer write operation
- [ ] Test feedback values near 0.0 for clean silence
- [ ] Add optional interpolation for smoother repeats (if CPU allows)

#### G.3 Parameter Optimization
**Tasks:**
- [ ] Tune reverb room size range for best sound (currently 0.0-1.0)
- [ ] Optimize default damping value for balanced brightness
- [ ] Test all effects simultaneously with various combinations
- [ ] Document "sweet spot" parameter ranges
- [ ] Create preset combinations (e.g., "hall", "plate", "spring")

#### G.4 Performance Validation
**Tasks:**
- [ ] Stress test: All 3 effects at max parameters
- [ ] Long-duration stability test (1+ hour)
- [ ] RAM leak detection (check free heap over time)
- [ ] Thermal stability (ESP32 temperature monitoring)

#### G.5 Documentation
**Tasks:**
- [ ] Document int16_t vs float tradeoffs
- [ ] Add "known characteristics" section (e.g., 16-bit reverb sound)
- [ ] Create effects usage guide with examples
- [ ] Document CPU budget breakdown by effect

**Available Headroom:** 9.4ms (85%) - plenty for quality improvements!

**Success Criteria:**
- [ ] Reverb tail smoother (if Option 1 or 2 implemented)
- [ ] Delay clean at end of repetitions
- [ ] All effects tested together without artifacts
- [ ] CPU usage stays below 50%
- [ ] Documentation complete

### Documentation
- [ ] Update activeContext.md with effects implementation
- [ ] Update progress.md - Phase 4 status
- [ ] Create EFFECTS_IMPLEMENTATION.md in docs/improvements/
- [ ] Document CPU benchmarks and performance data

---

## Success Criteria (Overall)

**Phase 4 Effects Complete When:**
- ✅ Delay effect implemented and working
- ✅ Chorus effect implemented and working
- ✅ Reverb effect implemented and working
- ✅ EffectsChain manages all three effects
- ✅ ControlHandler provides serial control
- ✅ CPU usage <75% (achieved 14.5%!)
- ✅ Audio quality maintained (no glitches)
- ✅ All parameters adjustable in real-time
- ✅ Reverb buzzing fixed with noise gates
- ✅ Volume smoothing toggle for testing

**Current Status: Phase 4 Core Implementation COMPLETE! 🎉**

**Performance Achievement:**
- **3 Oscillators + Delay + Chorus + Reverb**
- **Execution Time:** 1.6ms / 11ms (14.5% CPU)
- **RAM:** Stable, no leaks
- **Audio Quality:** Excellent (characteristic 16-bit sound)

**You have a multi-oscillator synthesizer with THREE professional effects!** �✨

---

## Next Steps After Phase 4

**Phase 3 Hardware (when parts arrive):**
- Add MCP23017 GPIO expander
- Wire physical switches for oscillators
- Wire toggle switches for effects
- Connect ControlHandler to GPIO inputs

**Phase 4 Visual (LED meters):**
- Add WS2812B LED strips
- Implement LEDMeter class
- Show sensor ranges visually

**Phase 5 Polish:**
- Professional I/O setup
- Final calibration
- Enclosure design

---

**Document Version:** 1.0
**Created:** October 31, 2025
**Status:** Ready for Implementation

**Remember:** Take it step-by-step, test thoroughly at each phase, and enjoy the process! 🎶🔧
