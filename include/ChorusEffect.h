/*
 * ChorusEffect.h
 *
 * Chorus effect using modulated delay with Oscillator-based LFO.
 * Creates thick, shimmering sound by pitch-shifting with sinusoidal modulation.
 *
 * Design: Uses Oscillator class as LFO for performance (sine LUT vs sin() calls).
 *
 * ═══════════════════════════════════════════════════════════════════════════
 * EFFECT MANUAL - CHORUS
 * ═══════════════════════════════════════════════════════════════════════════
 *
 * WHAT IT DOES:
 * The chorus effect simulates multiple sound sources playing together by
 * creating subtle pitch and timing variations. It makes a single sound source
 * appear wider, thicker, and more spacious - like a choir versus a solo voice.
 *
 * HOW IT WORKS:
 * - Uses a delay line with constantly varying delay time
 * - An LFO (Low Frequency Oscillator) modulates the delay time
 * - As delay time changes, pitch shifts up/down (Doppler effect)
 * - Multiple slightly detuned copies create the "ensemble" effect
 * - Uses Oscillator class as LFO (performance: sine LUT vs sin() calls)
 *
 * PARAMETERS:
 *
 * 1. RATE (0.1-10.0 Hz)
 *    - Speed of the LFO modulation (how fast the pitch wobbles)
 *    - Slow rates (0.5-2 Hz): Subtle, organic chorus
 *    - Medium rates (2-4 Hz): Classic chorus, noticeable shimmer
 *    - Fast rates (5-10 Hz): Vibrato/tremolo effect, warble
 *    Example: setRate(2.0) for classic chorus speed
 *
 * 2. DEPTH (1-50ms)
 *    - Amount of pitch variation (delay time swing)
 *    - Shallow depth (5-10ms): Subtle thickening, tight chorus
 *    - Medium depth (15-25ms): Classic chorus, clear detuning
 *    - Deep depth (30-50ms): Dramatic pitch wobble, Leslie speaker-like
 *    Example: setDepth(15) for moderate chorus effect
 *
 * 3. MIX (0.0-1.0)
 *    - Balance between dry (original) and wet (chorused) signal
 *    - 0.0: 100% dry, no chorus heard
 *    - 0.3-0.4: Subtle enhancement, adds width
 *    - 0.5: Equal mix, classic chorus balance
 *    - 0.7-1.0: Heavy chorus, swirly and spacious
 *    Example: setMix(0.4) for moderate chorus presence
 *
 * TYPICAL PRESETS:
 *
 * Subtle Thickener (clean enhancement):
 *   setRate(1.5);
 *   setDepth(8);
 *   setMix(0.3);
 *
 * Classic Chorus (80s synth style):
 *   setRate(2.5);
 *   setDepth(20);
 *   setMix(0.5);
 *
 * Deep Chorus (dramatic, swirly):
 *   setRate(3.0);
 *   setDepth(35);
 *   setMix(0.6);
 *
 * Vibrato (pitch wobble, no dry):
 *   setRate(5.0);
 *   setDepth(10);
 *   setMix(1.0);
 *
 * Leslie Speaker Simulation:
 *   setRate(1.0);
 *   setDepth(40);
 *   setMix(0.7);
 *
 * TECHNICAL DETAILS:
 * - Buffer size: Fixed 50ms + margin (~1200 samples at 22050 Hz)
 * - Interpolation: Linear (smooth pitch changes)
 * - LFO waveform: Sine (from Oscillator's lookup table)
 * - Delay modulation: depth ± (lfo * depth)
 *   Example: 15ms depth → 0-30ms delay range
 *
 * MEMORY USAGE:
 * Fixed buffer: ~2.4 KB RAM (50ms at 22050 Hz)
 * LFO overhead: Oscillator object (~60 bytes)
 *
 * CPU USAGE:
 * Low-Medium:
 * - LFO: Sine LUT lookup (very fast)
 * - Delay: Interpolated buffer read (moderate)
 * - Mix: Simple math (fast)
 *
 * MUSICAL USES:
 * - Synth pads: Add width and movement
 * - Lead lines: Thicken and enhance presence
 * - Ambient textures: Create evolving soundscapes
 * - Clean tones: Add organic variation
 *
 * TIPS:
 * - Slower rates sound more natural
 * - Higher depth = more obvious pitch variation
 * - Pair with delay for lush, spacious sounds
 * - Try moderate mix (0.3-0.5) for subtlety
 * - Use deep chorus + slow rate for dreamy pads
 *
 * ═══════════════════════════════════════════════════════════════════════════
 */

#pragma once
#include <Arduino.h>
#include <memory>
#include "Oscillator.h"  // Reuse Oscillator as LFO!
#include "AudioConstants.h"

/*
 * MEMORY MANAGEMENT EVOLUTION:
 *
 * Version 1 (Old C++ style - manual management):
 *   int16_t* delayBuffer = new int16_t[bufferSize];
 *   delete[] delayBuffer;  // Easy to forget, leads to memory leaks!
 *
 * Version 2 (Modern C++ - RAII with std::unique_ptr):
 *   std::unique_ptr<int16_t[]> delayBuffer;
 *   delayBuffer.reset(new int16_t[bufferSize]);  // Automatic cleanup when object destroyed
 *
 * WHY std::unique_ptr FOR CHORUS EFFECT:
 *   - Buffer size is FIXED (50ms, never changes after construction)
 *   - Automatic memory management (no manual new/delete)
 *   - Exception-safe (if allocation fails, automatic cleanup)
 *   - Zero overhead (just a wrapper around raw pointer)
 *   - Perfect for fixed-size allocations
 *
 * COMPARISON:
 *   Raw new[]/delete[]: Manual, error-prone, zero overhead
 *   std::unique_ptr:    Automatic, safe, zero overhead ✓ (BEST for fixed-size)
 *   std::vector:        Automatic, safe, ~12 bytes overhead, overkill for fixed buffers
 */

class ChorusEffect {
public:
    /**
     * Constructor
     * @param sampleRate Audio sample rate
     */
    ChorusEffect(uint32_t sampleRate = Audio::SAMPLE_RATE);

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
    std::unique_ptr<int16_t[]> delayBuffer;  // Fixed-size buffer (auto-managed)
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
