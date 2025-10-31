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
