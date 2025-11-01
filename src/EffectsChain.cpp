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
      chorus(sampleRate),         // Direct initialization on stack
      reverb(sampleRate) {        // Direct initialization on stack

    // Configure delay (object already constructed)
    delay.setFeedback(0.5f);
    delay.setMix(0.3f);
    delay.setEnabled(false);  // Start disabled

    // Configure chorus (object already constructed)
    chorus.setRate(1.0f);
    chorus.setDepth(5.0f);
    chorus.setMix(0.2f);
    chorus.setEnabled(false);

    // Configure reverb (object already constructed)
    reverb.setRoomSize(0.5f);
    reverb.setDamping(0.5f);
    reverb.setMix(0.3f);
    reverb.setEnabled(false);

    DEBUG_PRINTLN("[CHAIN] EffectsChain initialized with Delay + Chorus + Reverb");
}

int16_t EffectsChain::process(int16_t input) {
    int16_t output = input;

    // Apply effects in order
    // (Only process if enabled - effect handles bypass internally)
    output = delay.process(output);
    output = chorus.process(output);
    output = reverb.process(output);  // Reverb at the end of chain

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

void EffectsChain::setReverbEnabled(bool enabled) {
    reverb.setEnabled(enabled);
}

bool EffectsChain::isReverbEnabled() const {
    return reverb.isEnabled();
}

void EffectsChain::reset() {
    delay.reset();
    chorus.reset();
    reverb.reset();

    DEBUG_PRINTLN("[CHAIN] All effects reset");
}
