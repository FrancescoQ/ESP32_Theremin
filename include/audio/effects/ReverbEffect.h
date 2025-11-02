/*
 * ReverbEffect.h
 *
 * Freeverb-based reverb effect with comb and allpass filters.
 * Creates realistic room ambience and spatial depth.
 *
 * Algorithm: Simplified Freeverb
 * - 4 parallel comb filters (instead of full 8)
 * - 2 series allpass filters (instead of full 4)
 * - Optimized for ESP32 performance
 */

#pragma once
#include <Arduino.h>
#include "audio/AudioConstants.h"

class ReverbEffect {
public:
    /**
     * Constructor
     * @param sampleRate Audio sample rate
     */
    ReverbEffect(uint32_t sampleRate = Audio::SAMPLE_RATE);

    /**
     * Destructor - frees all buffers
     */
    ~ReverbEffect();

    /**
     * Process single audio sample through reverb
     */
    int16_t process(int16_t input);

    /**
     * Enable/disable effect
     */
    void setEnabled(bool enabled);
    bool isEnabled() const { return enabled; }

    /**
     * Set room size
     * @param size Room size (0.0 = small, 1.0 = large)
     */
    void setRoomSize(float size);

    /**
     * Set damping (high-frequency absorption)
     * @param damp Damping amount (0.0 = bright, 1.0 = dark/muffled)
     */
    void setDamping(float damp);

    /**
     * Set wet/dry mix
     * @param mix Mix amount (0.0 = dry only, 1.0 = wet only)
     */
    void setMix(float mix);

    /**
     * Clear all delay buffers
     */
    void reset();

    /**
     * Get current settings
     */
    float getRoomSize() const { return roomSize; }
    float getDamping() const { return damping; }
    float getMix() const { return wetDryMix; }

private:
    // Comb filter structure
    struct CombFilter {
        int16_t* buffer;
        size_t bufferSize;
        size_t bufferIndex;
        float feedback;
        float filterStore;  // For damping filter
        float damp1;        // Damping coefficient 1
        float damp2;        // Damping coefficient 2
    };

    // Allpass filter structure
    struct AllpassFilter {
        int16_t* buffer;
        size_t bufferSize;
        size_t bufferIndex;
    };

    static const int NUM_COMBS = 8;        // Full Freeverb: 8 comb filters
    static const int NUM_ALLPASSES = 4;    // Full Freeverb: 4 allpass filters

    // Freeverb tuning constants (in milliseconds - sample-rate agnostic)
    // Original Freeverb delay times converted from 44.1kHz sample rate
    static constexpr float COMB_DELAYS_MS[NUM_COMBS] = {
        25.31f, 26.94f, 28.96f, 30.75f,   // Original 4 combs
        32.24f, 33.81f, 35.31f, 36.66f    // Additional 4 combs for Full Freeverb
    };
    static constexpr float ALLPASS_DELAYS_MS[NUM_ALLPASSES] = {
        12.61f, 10.00f,  // Original 2 allpass
        7.73f, 5.10f     // Additional 2 allpass for Full Freeverb
    };

    static constexpr float FIXED_GAIN = 0.015f;      // Input gain
    static constexpr float SCALE_WET = 3.0f;         // Wet signal scaling
    static constexpr float SCALE_DAMPING = 0.4f;     // Damping scaling

    // Noise gate thresholds (2x increase for balance between tail length and noise control)
    static constexpr int16_t NOISE_GATE_THRESHOLD = 100;       // Input/output noise gate
    static constexpr float FILTER_NOISE_GATE_THRESHOLD = 1.0f; // Comb filter noise gate

    // Precision enhancement: Use int32_t intermediate math for smoother reverb tail
    // Shift left 8 bits = 256x precision (24-bit effective: 16-bit + 8-bit fractional)
    static constexpr int PRECISION_SHIFT = 8;

    uint32_t sampleRate;
    float roomSize;
    float damping;
    float wetDryMix;
    bool enabled;

    CombFilter combs[NUM_COMBS];
    AllpassFilter allpasses[NUM_ALLPASSES];

    /**
     * Initialize a comb filter
     */
    void initCombFilter(CombFilter& comb, float delayMs);

    /**
     * Initialize an allpass filter
     */
    void initAllpassFilter(AllpassFilter& allpass, float delayMs);

    /**
     * Process sample through comb filter
     */
    int16_t processComb(CombFilter& comb, int16_t input);

    /**
     * Process sample through allpass filter
     */
    int16_t processAllpass(AllpassFilter& allpass, int16_t input);

    /**
     * Update comb filter parameters based on room size and damping
     */
    void updateCombs();

    /**
     * Convert milliseconds to samples based on sample rate
     */
    int msToSamples(float ms);
};
