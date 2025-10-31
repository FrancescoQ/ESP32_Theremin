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
