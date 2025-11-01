/*
 * DelayEffect.h
 *
 * Digital delay effect with feedback and wet/dry mix control.
 * Uses circular buffer for delay line.
 *
 * ═══════════════════════════════════════════════════════════════════════════
 * EFFECT MANUAL - DELAY
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * WHAT IT DOES:
 * The delay effect creates echoes by storing incoming audio in a buffer and
 * playing it back after a specified time. The delayed signal can be fed back
 * into the buffer to create multiple repeating echoes.
 *
 * HOW IT WORKS:
 * - Circular buffer stores incoming audio samples
 * - Delay time determines how far back in the buffer we read
 * - Feedback controls how much of the delayed signal is fed back
 * - Mix controls the balance between original (dry) and delayed (wet) signals
 *
 * PARAMETERS:
 *
 * 1. DELAY TIME (10-2000ms)
 *    - The time between the original sound and its echo
 *    - Short delays (50-150ms): Slapback echo, doubling effect
 *    - Medium delays (200-400ms): Classic echo, rhythmic repeats
 *    - Long delays (500-2000ms): Ambient soundscapes, dub effects
 *    Example: setDelayTime(300) for a 300ms echo
 *
 * 2. FEEDBACK (0.0-0.95)
 *    - How much of the delayed signal feeds back into the buffer
 *    - 0.0: Single echo (no repeats)
 *    - 0.3-0.5: Few gentle repeats
 *    - 0.6-0.8: Multiple distinct echoes
 *    - 0.9-0.95: Very long decay, ambient trails
 *    WARNING: Keep below 1.0 to prevent runaway feedback!
 *    Example: setFeedback(0.6) for moderate repeats
 *
 * 3. MIX (0.0-1.0)
 *    - Balance between dry (original) and wet (delayed) signal
 *    - 0.0: 100% dry, no delay heard
 *    - 0.3: Subtle delay enhancement
 *    - 0.5: Equal mix of dry and wet
 *    - 1.0: 100% wet, only delayed signal
 *    Example: setMix(0.3) for subtle echo
 *
 * TYPICAL PRESETS:
 *
 * Slapback Echo (rockabilly style):
 *   setDelayTime(100);
 *   setFeedback(0.3);
 *   setMix(0.4);
 *
 * Rhythmic Echo:
 *   setDelayTime(375);  // Matches 160 BPM
 *   setFeedback(0.6);
 *   setMix(0.4);
 *
 * Ambient Wash:
 *   setDelayTime(800);
 *   setFeedback(0.85);
 *   setMix(0.5);
 *
 * MEMORY USAGE:
 * Buffer size = (delay_time_ms / 1000) * sample_rate * 2 bytes
 * Example: 300ms delay at 22050 Hz = ~13 KB RAM
 *
 * CPU USAGE:
 * Very low - simple buffer read/write operations
 *
 * ═══════════════════════════════════════════════════════════════════════════
 */

#pragma once
#include <Arduino.h>
#include "AudioConstants.h"

class DelayEffect {
public:
    /**
     * Constructor
     * @param delayTimeMs Delay time in milliseconds
     * @param sampleRate Audio sample rate (e.g., 22050 Hz)
     */
    DelayEffect(uint32_t delayTimeMs, uint32_t sampleRate = Audio::SAMPLE_RATE);

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
