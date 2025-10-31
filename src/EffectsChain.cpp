/*
 * EffectsChain.cpp
 *
 * Implementation of effects processing chain.
 */

#include "EffectsChain.h"
#include "Debug.h"

EffectsChain::EffectsChain(uint32_t sampleRate)
    : sampleRate(sampleRate),
      delay(300, sampleRate),    // Direct initialization on stack
      chorus(sampleRate) {        // Direct initialization on stack

    // Configure delay (object already constructed)
    delay.setFeedback(0.5f);
    delay.setMix(0.3f);
    delay.setEnabled(false);  // Start disabled

    // Configure chorus (object already constructed)
    chorus.setRate(2.0f);
    chorus.setDepth(15.0f);
    chorus.setMix(0.4f);
    chorus.setEnabled(false);

    DEBUG_PRINTLN("[CHAIN] EffectsChain initialized with Delay + Chorus");
}

int16_t EffectsChain::process(int16_t input) {
    int16_t output = input;

    // Apply effects in order
    // (Only process if enabled - effect handles bypass internally)
    output = delay.process(output);
    output = chorus.process(output);

    // Future: output = reverb.process(output);

    return output;
}

void EffectsChain::setDelayEnabled(bool enabled) {
    delay.setEnabled(enabled);
}

bool EffectsChain::isDelayEnabled() const {
    return delay.isEnabled();
}

void EffectsChain::setChorusEnabled(bool enabled) {
    chorus.setEnabled(enabled);
}

bool EffectsChain::isChorusEnabled() const {
    return chorus.isEnabled();
}

void EffectsChain::reset() {
    delay.reset();
    chorus.reset();
    // Future: reverb.reset();

    DEBUG_PRINTLN("[CHAIN] All effects reset");
}
