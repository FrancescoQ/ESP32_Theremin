/*
 * ReverbEffect.cpp
 *
 * Implementation of Freeverb-based reverb effect.
 * Based on the classic Freeverb algorithm by Jezar at Dreampoint.
 */

#include "ReverbEffect.h"
#include "Debug.h"
#include <string.h>

ReverbEffect::ReverbEffect(uint32_t sampleRate)
    : sampleRate(sampleRate),
      roomSize(0.5f),
      damping(0.5f),
      wetDryMix(0.3f),
      enabled(false) {

    // Initialize comb filters with tuned delay lengths (in milliseconds)
    initCombFilter(combs[0], COMB_DELAY_1_MS);
    initCombFilter(combs[1], COMB_DELAY_2_MS);
    initCombFilter(combs[2], COMB_DELAY_3_MS);
    initCombFilter(combs[3], COMB_DELAY_4_MS);

    // Initialize allpass filters (in milliseconds)
    initAllpassFilter(allpasses[0], ALLPASS_DELAY_1_MS);
    initAllpassFilter(allpasses[1], ALLPASS_DELAY_2_MS);

    // Set initial parameters
    updateCombs();

    DEBUG_PRINTLN("[REVERB] Initialized with 4 comb + 2 allpass filters");
}

ReverbEffect::~ReverbEffect() {
    // Free comb filter buffers
    for (int i = 0; i < NUM_COMBS; i++) {
        if (combs[i].buffer != nullptr) {
            delete[] combs[i].buffer;
            combs[i].buffer = nullptr;
        }
    }

    // Free allpass filter buffers
    for (int i = 0; i < NUM_ALLPASSES; i++) {
        if (allpasses[i].buffer != nullptr) {
            delete[] allpasses[i].buffer;
            allpasses[i].buffer = nullptr;
        }
    }

    DEBUG_PRINTLN("[REVERB] Destroyed");
}

int ReverbEffect::msToSamples(float ms) {
    // Convert milliseconds to samples based on current sample rate
    return (int)(ms * sampleRate / 1000.0f);
}

void ReverbEffect::initCombFilter(CombFilter& comb, float delayMs) {
    comb.bufferSize = msToSamples(delayMs);
    comb.buffer = new int16_t[comb.bufferSize];
    memset(comb.buffer, 0, comb.bufferSize * sizeof(int16_t));
    comb.bufferIndex = 0;
    comb.filterStore = 0;
    comb.feedback = 0.5f;  // Will be updated by updateCombs()
    comb.damp1 = 0.5f;
    comb.damp2 = 0.5f;
}

void ReverbEffect::initAllpassFilter(AllpassFilter& allpass, float delayMs) {
    allpass.bufferSize = msToSamples(delayMs);
    allpass.buffer = new int16_t[allpass.bufferSize];
    memset(allpass.buffer, 0, allpass.bufferSize * sizeof(int16_t));
    allpass.bufferIndex = 0;
}

void ReverbEffect::updateCombs() {
    // Calculate feedback based on room size
    // Larger rooms = more feedback = longer reverb tail
    float feedback = 0.28f + (roomSize * 0.7f);
    if (feedback > 0.98f) feedback = 0.98f;  // Prevent instability

    // Calculate damping coefficients
    float damp = damping * SCALE_DAMPING;
    float damp1 = damp;
    float damp2 = 1.0f - damp;

    // Update all comb filters
    for (int i = 0; i < NUM_COMBS; i++) {
        combs[i].feedback = feedback;
        combs[i].damp1 = damp1;
        combs[i].damp2 = damp2;
    }
}

int16_t ReverbEffect::processComb(CombFilter& comb, int16_t input) {
    // Read from circular buffer
    int16_t output = comb.buffer[comb.bufferIndex];

    // Apply damping filter (one-pole lowpass)
    comb.filterStore = (output * comb.damp2) + (comb.filterStore * comb.damp1);

    // Write new value with feedback
    int32_t newValue = input + (int32_t)(comb.filterStore * comb.feedback);

    // Clamp to prevent overflow
    if (newValue > 32767) newValue = 32767;
    if (newValue < -32768) newValue = -32768;

    comb.buffer[comb.bufferIndex] = (int16_t)newValue;

    // Advance buffer index (circular)
    comb.bufferIndex++;
    if (comb.bufferIndex >= comb.bufferSize) {
        comb.bufferIndex = 0;
    }

    return output;
}

int16_t ReverbEffect::processAllpass(AllpassFilter& allpass, int16_t input) {
    // Read from buffer
    int16_t bufferOut = allpass.buffer[allpass.bufferIndex];

    // Allpass calculation: output = -input + bufferOut
    // Store: input + (bufferOut * 0.5)
    int32_t output = -input + bufferOut;
    int32_t store = input + (bufferOut >> 1);  // Divide by 2 (0.5 feedback)

    // Clamp
    if (store > 32767) store = 32767;
    if (store < -32768) store = -32768;

    allpass.buffer[allpass.bufferIndex] = (int16_t)store;

    // Advance buffer index
    allpass.bufferIndex++;
    if (allpass.bufferIndex >= allpass.bufferSize) {
        allpass.bufferIndex = 0;
    }

    // Clamp output
    if (output > 32767) output = 32767;
    if (output < -32768) output = -32768;

    return (int16_t)output;
}

int16_t ReverbEffect::process(int16_t input) {
    // Bypass if disabled
    if (!enabled) {
        return input;
    }

    // Scale input
    int32_t scaledInput = (int32_t)(input * FIXED_GAIN);

    // Parallel comb filters (sum their outputs)
    int32_t combSum = 0;
    for (int i = 0; i < NUM_COMBS; i++) {
        combSum += processComb(combs[i], scaledInput);
    }

    // Series allpass filters (cascade)
    int16_t allpassOut = processAllpass(allpasses[0], combSum >> 2);  // Divide by 4 to prevent overflow
    allpassOut = processAllpass(allpasses[1], allpassOut);

    // Scale wet signal
    int32_t wet = (int32_t)(allpassOut * SCALE_WET);

    // Mix wet and dry
    int32_t dry = input;
    int32_t output = (dry * (1.0f - wetDryMix)) + (wet * wetDryMix);

    // Clamp output
    if (output > 32767) output = 32767;
    if (output < -32768) output = -32768;

    return (int16_t)output;
}

void ReverbEffect::setEnabled(bool en) {
    enabled = en;
    DEBUG_PRINT("[REVERB] ");
    DEBUG_PRINTLN(enabled ? "ENABLED" : "DISABLED");
}

void ReverbEffect::setRoomSize(float size) {
    // Constrain to valid range
    if (size < 0.0f) size = 0.0f;
    if (size > 1.0f) size = 1.0f;

    roomSize = size;
    updateCombs();

    DEBUG_PRINT("[REVERB] Room size set to ");
    DEBUG_PRINTLN(roomSize);
}

void ReverbEffect::setDamping(float damp) {
    // Constrain to valid range
    if (damp < 0.0f) damp = 0.0f;
    if (damp > 1.0f) damp = 1.0f;

    damping = damp;
    updateCombs();

    DEBUG_PRINT("[REVERB] Damping set to ");
    DEBUG_PRINTLN(damping);
}

void ReverbEffect::setMix(float mix) {
    // Constrain to valid range
    if (mix < 0.0f) mix = 0.0f;
    if (mix > 1.0f) mix = 1.0f;

    wetDryMix = mix;

    DEBUG_PRINT("[REVERB] Mix set to ");
    DEBUG_PRINTLN(wetDryMix);
}

void ReverbEffect::reset() {
    // Clear all comb filter buffers
    for (int i = 0; i < NUM_COMBS; i++) {
        if (combs[i].buffer != nullptr) {
            memset(combs[i].buffer, 0, combs[i].bufferSize * sizeof(int16_t));
            combs[i].bufferIndex = 0;
            combs[i].filterStore = 0;
        }
    }

    // Clear all allpass filter buffers
    for (int i = 0; i < NUM_ALLPASSES; i++) {
        if (allpasses[i].buffer != nullptr) {
            memset(allpasses[i].buffer, 0, allpasses[i].bufferSize * sizeof(int16_t));
            allpasses[i].bufferIndex = 0;
        }
    }

    DEBUG_PRINTLN("[REVERB] Buffers cleared");
}
