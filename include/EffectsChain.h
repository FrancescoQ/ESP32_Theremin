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
