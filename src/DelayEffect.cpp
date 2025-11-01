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
      writeIndex(0) {

    // Calculate and allocate buffer using std::vector
    size_t bufferSize = calculateBufferSize(delayTimeMs);
    delayBuffer.resize(bufferSize);

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
    // std::vector automatically frees memory - no manual cleanup needed!
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
    newSample = constrain(newSample, Audio::SAMPLE_MIN, Audio::SAMPLE_MAX);

    delayBuffer[writeIndex] = (int16_t)newSample;

    // Advance write pointer (circular)
    writeIndex++;
    if (writeIndex >= delayBuffer.size()) {
        writeIndex = 0;
    }

    // Mix dry and wet signals
    // output = (dry * (1-mix)) + (wet * mix)
    int32_t dry = input;
    int32_t wet = delayedSample;
    int32_t output = (dry * (1.0f - wetDryMix)) + (wet * wetDryMix);

    // Clamp output
    output = constrain(output, Audio::SAMPLE_MIN, Audio::SAMPLE_MAX);

    return (int16_t)output;
}

void DelayEffect::setEnabled(bool en) {
    enabled = en;
    DEBUG_PRINT("[DELAY] ");
    DEBUG_PRINTLN(enabled ? "ENABLED" : "DISABLED");
}

void DelayEffect::setDelayTime(uint32_t timeMs) {
    // Constrain to reasonable range
    timeMs = constrain(timeMs, 10, 2000);

    delayTimeMs = timeMs;

    // Resize buffer if needed (std::vector handles reallocation automatically)
    size_t newSize = calculateBufferSize(timeMs);
    if (newSize != delayBuffer.size()) {
        delayBuffer.resize(newSize);
        reset();

        DEBUG_PRINT("[DELAY] Time changed to ");
        DEBUG_PRINT(delayTimeMs);
        DEBUG_PRINT("ms, new buffer: ");
        DEBUG_PRINT((newSize * sizeof(int16_t)) / 1024);
        DEBUG_PRINTLN(" KB");
    }
}

void DelayEffect::setFeedback(float fb) {
    // Constrain to safe range (prevent runaway feedback)
    fb = constrain(fb, 0.0f, 0.95f);

    feedback = fb;

    DEBUG_PRINT("[DELAY] Feedback set to ");
    DEBUG_PRINTLN(feedback);
}

void DelayEffect::setMix(float mix) {
    // Constrain to 0.0-1.0
    mix = constrain(mix, 0.0f, 1.0f);

    wetDryMix = mix;

    DEBUG_PRINT("[DELAY] Mix set to ");
    DEBUG_PRINTLN(wetDryMix);
}

void DelayEffect::reset() {
    if (!delayBuffer.empty()) {
        std::fill(delayBuffer.begin(), delayBuffer.end(), 0);
        writeIndex = 0;
        DEBUG_PRINTLN("[DELAY] Buffer cleared");
    }
}
