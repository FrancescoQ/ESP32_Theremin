# Effects Implementation Plan - Phase 4

**Status:** Planning Complete - Ready for Implementation
**Date Created:** October 31, 2025
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

## Phase C: ChorusEffect Implementation (with Oscillator-Based LFO)

**Design Decision:** The chorus effect uses the existing `Oscillator` class as its LFO instead of implementing a separate phase accumulator. This provides:
- **Performance benefit**: Uses optimized sine LUT instead of sin() calls (~100x faster)
- **Code reuse**: Leverages proven phase accumulator logic
- **Architectural elegance**: LFO is conceptually an oscillator
- **Future flexibility**: Easy to experiment with different LFO waveforms

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
- [ ] Add `getNextSampleNormalized()` declaration to Oscillator.h
- [ ] Implement `getNextSampleNormalized()` in Oscillator.cpp
- [ ] Build and verify compilation

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
- [ ] Create `include/ChorusEffect.h` with Oscillator-based LFO

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
- [ ] Create `src/ChorusEffect.cpp` with Oscillator-based LFO
- [ ] Verify interpolation logic
- [ ] Build and test

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
- [ ] Modify EffectsChain.h to add ChorusEffect
- [ ] Modify EffectsChain.cpp to integrate chorus
- [ ] Build and test

**Testing:**
- [ ] Verify chorus effect is initialized (check serial)
- [ ] Test with chorus enabled via temporary code
- [ ] Listen for characteristic "shimmer" sound
- [ ] Verify no audio glitches

**Success Criteria:**
- ✅ Chorus compiles and integrates
- ✅ Chorus creates audible pitch modulation when enabled
- ✅ No crashes or audio artifacts
- ✅ CPU usage acceptable (benchmark in Phase E)

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
- [ ] Add effects control commands to executeCommand()
- [ ] Add printEffectsStatus() method
- [ ] Update printHelp() with effects commands
- [ ] Build and test

**Testing:**
- [ ] Upload firmware
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

---

## Phase E: Testing & CPU Benchmarking

### E.1 Create Test Scenarios

**Test Plan:**

1. **Baseline (No Effects)**
   - [ ] Run theremin with all effects disabled
   - [ ] Note CPU usage from PerformanceMonitor
   - [ ] Verify smooth audio output

2. **Delay Only**
   - [ ] Enable delay: `delay:on`
   - [ ] Test various settings:
     - Short delay (100ms): `delay:time:100`
     - Medium delay (300ms): `delay:time:300`
     - Long delay (800ms): `delay:time:800`
     - High feedback (0.8): `delay:feedback:0.8`
   - [ ] Note CPU impact
   - [ ] Check for audio dropouts

3. **Chorus Only**
   - [ ] Enable chorus: `chorus:on`
   - [ ] Test various settings:
     - Slow rate (1Hz): `chorus:rate:1.0`
     - Fast rate (4Hz): `chorus:rate:4.0`
     - Deep modulation (30ms): `chorus:depth:30`
   - [ ] Note CPU impact
   - [ ] Listen for characteristic sound

4. **Both Effects**
   - [ ] Enable both: `delay:on` + `chorus:on`
   - [ ] Test with 3 oscillators active
   - [ ] Note total CPU usage
   - [ ] Check for glitches or dropouts

5. **Stress Test**
   - [ ] Enable all 3 oscillators (different waveforms)
   - [ ] Enable both effects
   - [ ] Play continuously for 5 minutes
   - [ ] Monitor for instability

### E.2 Performance Metrics to Collect

**Create a results table:**

| Configuration | CPU % | Free RAM | Latency | Audio Quality | Notes |
|---------------|-------|----------|---------|---------------|-------|
| Baseline (no FX) | ? | ? | ? | ? | |
| Delay only | ? | ? | ? | ? | |
| Chorus only | ? | ? | ? | ? | |
| Both FX | ? | ? | ? | ? | |
| 3 Osc + Both FX | ? | ? | ? | ? | |

**Tasks:**
- [ ] Run all test scenarios
- [ ] Fill in performance table
- [ ] Document any issues found
- [ ] Determine if CPU budget allows reverb (Phase F)

**Success Criteria:**
- ✅ CPU usage with both effects <70%
- ✅ No audio dropouts during stress test
- ✅ RAM usage stable (no leaks)
- ✅ Latency still <20ms
- ✅ Effects sound musical and clean

---

## Phase F: Optional Reverb (If CPU Permits)

**Decision Point:**

If Phase E shows CPU < 65% with 3 oscillators + delay + chorus:
→ **PROCEED with reverb implementation**

If Phase E shows CPU > 70%:
→ **SKIP reverb**, document as "Phase 7 future upgrade"

### F.1 Reverb Implementation (If Proceeding)

**Note:** Reverb is CPU-intensive. Simplified Freeverb algorithm:
- 4 comb filters (instead of 8)
- 2 allpass filters (instead of 4)
- Estimated CPU: 20-30%

**Files to create:**
- `include/ReverbEffect.h`
- `src/ReverbEffect.cpp`

**Integration:**
- Add to EffectsChain (same pattern as delay/chorus)
- Add control commands to ControlHandler

**Test extensively before considering "done"!**

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

### Phase C: ChorusEffect
- [ ] ChorusEffect.h created
- [ ] ChorusEffect.cpp implemented
- [ ] Added to EffectsChain
- [ ] Tested and sounds good

### Phase D: ControlHandler
- [ ] Effects commands added
- [ ] printEffectsStatus() implemented
- [ ] Help text updated
- [ ] All commands tested

### Phase E: Testing
- [ ] All test scenarios run
- [ ] Performance table filled
- [ ] CPU usage within budget
- [ ] No stability issues

### Phase F: Reverb (Optional)
- [ ] Decision made (proceed or skip)
- [ ] If proceeding: reverb implemented and tested
- [ ] Final CPU check

### Documentation
- [ ] Update activeContext.md with effects implementation
- [ ] Update progress.md - Phase 4 status
- [ ] Create EFFECTS_IMPLEMENTATION.md in docs/improvements/
- [ ] Document CPU benchmarks

---

## Success Criteria (Overall)

**Phase 4 Effects Complete When:**
- ✅ Delay effect implemented and working
- ✅ Chorus effect implemented and working
- ✅ EffectsChain manages both effects
- ✅ ControlHandler provides serial control
- ✅ CPU usage <75% with 3 osc + delay + chorus
- ✅ Audio quality maintained (no glitches)
- ✅ All parameters adjustable in real-time
- ✅ Documentation updated

**At this point:** You have a multi-oscillator synthesizer with professional effects! 🎉

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
