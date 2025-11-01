/*
 * ChorusEffect.cpp
 *
 * Implementation of chorus effect with Oscillator-based LFO.
 */

#include "ChorusEffect.h"
#include "Debug.h"
#include <string.h>

// Fixed base delay for chorus effect (center of modulation)
static const float BASE_DELAY_MS = 10.0f;

ChorusEffect::ChorusEffect(uint32_t sampleRate)
    : sampleRate(sampleRate),
      lfoDepthMs(7.0f),      // Default 7ms modulation depth (±7ms around base)
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
    delayBuffer.reset(new int16_t[bufferSize]);  // C++11 compatible

    reset();

    DEBUG_PRINT("[CHORUS] Initialized with Oscillator-based LFO: buffer size ");
    DEBUG_PRINT(bufferSize);
    DEBUG_PRINT(" samples (");
    DEBUG_PRINT((bufferSize * sizeof(int16_t)) / 1024);
    DEBUG_PRINTLN(" KB)");

    DEBUG_PRINT("[CHORUS] LFO configured: Freq=");
    DEBUG_PRINT(lfo.getEffectiveFrequency());
    DEBUG_PRINT(" Hz, Depth=");
    DEBUG_PRINT(lfoDepthMs);
    DEBUG_PRINT(" ms, Mix=");
    DEBUG_PRINT(wetDryMix);
    DEBUG_PRINT(", SampleRate=");
    DEBUG_PRINTLN(sampleRate);
}

ChorusEffect::~ChorusEffect() {
    // std::unique_ptr automatically frees memory - no manual cleanup needed!
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
    // Base delay + LFO modulation (industry standard formula)
    // With base=10ms, depth=7ms: delay varies from 3ms to 17ms
    float delayTimeMs = BASE_DELAY_MS + (lfoValue * lfoDepthMs);
    float delayInSamples = (delayTimeMs / 1000.0f) * sampleRate;

    // Read delayed sample with interpolation
    int16_t delayedSample = readDelayBuffer(delayInSamples);

    // Mix dry and wet
    int32_t dry = input;
    int32_t wet = delayedSample;
    int32_t output = (dry * (1.0f - wetDryMix)) + (wet * wetDryMix);

    // Clamp
    output = constrain(output, Audio::SAMPLE_MIN, Audio::SAMPLE_MAX);

    // Advance write pointer
    writeIndex = (writeIndex + 1) % bufferSize;

    return (int16_t)output;
}

void ChorusEffect::setEnabled(bool en) {
    enabled = en;
    DEBUG_PRINT("[CHORUS] ");
    DEBUG_PRINTLN(enabled ? "ENABLED" : "DISABLED");

    if (enabled) {
        DEBUG_PRINT("[CHORUS] Active settings - Rate: ");
        DEBUG_PRINT(lfo.getEffectiveFrequency());
        DEBUG_PRINT(" Hz, Depth: ");
        DEBUG_PRINT(lfoDepthMs);
        DEBUG_PRINT(" ms, Mix: ");
        DEBUG_PRINTLN(wetDryMix);

        DEBUG_PRINT("[CHORUS] LFO internal state - Waveform: ");
        DEBUG_PRINT(lfo.getWaveform());
        DEBUG_PRINT(", OctaveShift: ");
        DEBUG_PRINT(lfo.getOctaveShift());
        DEBUG_PRINT(", Volume: ");
        DEBUG_PRINTLN(lfo.getVolume());
    }
}

void ChorusEffect::setRate(float hz) {
    // Constrain to reasonable range
    hz = constrain(hz, 0.1f, 10.0f);

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
    ms = constrain(ms, 1.0f, 50.0f);

    lfoDepthMs = ms;

    DEBUG_PRINT("[CHORUS] Depth set to ");
    DEBUG_PRINT(lfoDepthMs);
    DEBUG_PRINTLN(" ms");
}

void ChorusEffect::setMix(float mix) {
    mix = constrain(mix, 0.0f, 1.0f);

    wetDryMix = mix;

    DEBUG_PRINT("[CHORUS] Mix set to ");
    DEBUG_PRINTLN(wetDryMix);
}

void ChorusEffect::reset() {
    if (delayBuffer) {
        memset(delayBuffer.get(), 0, bufferSize * sizeof(int16_t));
        writeIndex = 0;
        // Note: No need to reset LFO phase - continuous modulation is fine
        DEBUG_PRINTLN("[CHORUS] Buffer cleared");
    }
}
