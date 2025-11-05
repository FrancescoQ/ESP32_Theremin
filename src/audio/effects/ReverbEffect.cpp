/*
 * ReverbEffect.cpp
 *
 * Implementation of Freeverb-based reverb effect.
 * Based on the classic Freeverb algorithm by Jezar at Dreampoint.
 * @see https://github.com/sinshu/freeverb/tree/main
 * @see https://ccrma.stanford.edu/~jos//waveguide/Freeverb.html
 */

#include "audio/effects/ReverbEffect.h"
#include "system/Debug.h"
#include <string.h>

// Define static constexpr arrays (required for C++14 and earlier)
constexpr float ReverbEffect::COMB_DELAYS_MS[NUM_COMBS];
constexpr float ReverbEffect::ALLPASS_DELAYS_MS[NUM_ALLPASSES];

ReverbEffect::ReverbEffect(uint32_t sampleRate)
    : sampleRate(sampleRate),
      roomSize(0.5f),
      damping(0.5f),
      wetDryMix(0.3f),
      enabled(false) {

    // Initialize comb filters with tuned delay lengths (in milliseconds)
    for (int i = 0; i < NUM_COMBS; i++) {
        initCombFilter(combs[i], COMB_DELAYS_MS[i]);
    }

    // Initialize allpass filters (in milliseconds)
    for (int i = 0; i < NUM_ALLPASSES; i++) {
        initAllpassFilter(allpasses[i], ALLPASS_DELAYS_MS[i]);
    }

    // Set initial parameters
    updateCombs();

    DEBUG_PRINTLN("[REVERB] Initialized with 8 comb + 4 allpass filters (Full Freeverb)");
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
    // Limited to 0.94 max (balanced between tail length and noise control)
    float feedback = 0.28f + (roomSize * 0.66f);  // Max = 0.28 + 0.66 = 0.94
    feedback = constrain(feedback, 0.0f, 0.94f);  // Prevent infinite noise circulation

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
    // Read from circular buffer at normal scale
    int16_t output = comb.buffer[comb.bufferIndex];

    // Apply damping filter (one-pole lowpass) at NORMAL scale
    // This prevents filterStore from accumulating huge values
    comb.filterStore = (output * comb.damp2) + (comb.filterStore * comb.damp1);

    // Noise gate: Prevent very small float values from accumulating as noise
    // This eliminates buzzing when the reverb tail decays to silence
    if (comb.filterStore > -FILTER_NOISE_GATE_THRESHOLD && comb.filterStore < FILTER_NOISE_GATE_THRESHOLD) {
        comb.filterStore = 0.0f;
    }

    // Scale input UP to int32_t precision for high-precision feedback calculation
    int32_t input32 = (int32_t)input << PRECISION_SHIFT;

    // Compute feedback: scale filterStore UP when multiplying for precision
    int32_t feedback32 = (int32_t)(comb.filterStore * comb.feedback * (1 << PRECISION_SHIFT));
    int32_t newValue32 = input32 + feedback32;

    // Clamp at 32-bit scaled range
    int32_t maxVal = (int32_t)Audio::SAMPLE_MAX << PRECISION_SHIFT;
    int32_t minVal = (int32_t)Audio::SAMPLE_MIN << PRECISION_SHIFT;
    newValue32 = constrain(newValue32, minVal, maxVal);

    // Scale DOWN and store as int16_t
    comb.buffer[comb.bufferIndex] = (int16_t)(newValue32 >> PRECISION_SHIFT);

    // Advance buffer index (circular)
    comb.bufferIndex++;
    if (comb.bufferIndex >= comb.bufferSize) {
        comb.bufferIndex = 0;
    }

    // Return output (already at normal scale)
    return output;
}

int16_t ReverbEffect::processAllpass(AllpassFilter& allpass, int16_t input) {
    // Read from buffer
    int16_t bufferOut = allpass.buffer[allpass.bufferIndex];

    // Allpass calculation: output = -input + bufferOut
    // Store: input + (bufferOut * 0.5)
    int32_t output = -input + bufferOut;
    int32_t store = input + (bufferOut >> 1);  // Divide by 2 (0.5 feedback)

    // Clamp store value
    store = constrain(store, Audio::SAMPLE_MIN, Audio::SAMPLE_MAX);
    allpass.buffer[allpass.bufferIndex] = (int16_t)store;

    // Advance buffer index
    allpass.bufferIndex++;
    if (allpass.bufferIndex >= allpass.bufferSize) {
        allpass.bufferIndex = 0;
    }

    // Clamp output
    output = constrain(output, Audio::SAMPLE_MIN, Audio::SAMPLE_MAX);

    return (int16_t)output;
}

int16_t ReverbEffect::process(int16_t input) {
    // Bypass if disabled
    if (!enabled) {
        return input;
    }

    // Noise gate: Silence very quiet inputs to prevent noise circulation
    // This prevents quantization noise from entering the feedback loops
    if (input > -NOISE_GATE_THRESHOLD && input < NOISE_GATE_THRESHOLD) {
        input = 0;
    }

    // Scale input
    int32_t scaledInput = (int32_t)(input * FIXED_GAIN);

    // Parallel comb filters (sum their outputs)
    int32_t combSum = 0;
    for (int i = 0; i < NUM_COMBS; i++) {
        combSum += processComb(combs[i], scaledInput);
    }

    // Series allpass filters (cascade all 4)
    int16_t allpassOut = processAllpass(allpasses[0], combSum >> 3);  // Divide by 8 to prevent overflow
    allpassOut = processAllpass(allpasses[1], allpassOut);
    allpassOut = processAllpass(allpasses[2], allpassOut);
    allpassOut = processAllpass(allpasses[3], allpassOut);

    // Scale wet signal
    int32_t wet = (int32_t)(allpassOut * SCALE_WET);

    // Mix wet and dry
    int32_t dry = input;
    int32_t output = (dry * (1.0f - wetDryMix)) + (wet * wetDryMix);

    // Clamp output
    output = constrain(output, Audio::SAMPLE_MIN, Audio::SAMPLE_MAX);

    // Noise gate: Ensure output below noise floor is silenced
    // This ensures the reverb tail decays to true silence without buzzing
    if (output > -NOISE_GATE_THRESHOLD && output < NOISE_GATE_THRESHOLD) {
        output = 0;
    }

    return (int16_t)output;
}

void ReverbEffect::setEnabled(bool en) {
    enabled = en;
    DEBUG_PRINT("[REVERB] ");
    DEBUG_PRINTLN(enabled ? "ENABLED" : "DISABLED");
}

void ReverbEffect::setRoomSize(float size) {
    // Constrain to valid range
    size = constrain(size, 0.0f, 1.0f);

    roomSize = size;
    updateCombs();

    DEBUG_PRINT("[REVERB] Room size set to ");
    DEBUG_PRINTLN(roomSize);
}

void ReverbEffect::setDamping(float damp) {
    // Constrain to valid range
    damp = constrain(damp, 0.0f, 1.0f);

    damping = damp;
    updateCombs();

    DEBUG_PRINT("[REVERB] Damping set to ");
    DEBUG_PRINTLN(damping);
}

void ReverbEffect::setMix(float mix) {
    // Constrain to valid range
    mix = constrain(mix, 0.0f, 1.0f);

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
