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
