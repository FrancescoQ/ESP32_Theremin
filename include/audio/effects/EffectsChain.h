/*
 * EffectsChain.h
 *
 * Manages chain of audio effects applied to mixed oscillator output.
 */

#pragma once
#include <Arduino.h>
#include "audio/AudioConstants.h"
#include "audio/effects/DelayEffect.h"
#include "audio/effects/ChorusEffect.h"
#include "audio/effects/ReverbEffect.h"

class EffectsChain {
public:
    /**
     * Constructor
     * @param sampleRate Audio sample rate
     */
    EffectsChain(uint32_t sampleRate = Audio::SAMPLE_RATE);

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
    void setChorusEnabled(bool enabled);
    void setReverbEnabled(bool enabled);

    /**
     * Get effect instances (for parameter control)
     */
    DelayEffect* getDelay() { return &delay; }
    ChorusEffect* getChorus() { return &chorus; }
    ReverbEffect* getReverb() { return &reverb; }

    /**
     * Reset all effect buffers
     */
    void reset();

    /**
     * Get effect enable states
     */
    bool isDelayEnabled() const;
    bool isChorusEnabled() const;
    bool isReverbEnabled() const;

private:
    uint32_t sampleRate;

    DelayEffect delay;
    ChorusEffect chorus;
    ReverbEffect reverb;
};
