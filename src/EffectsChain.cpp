/*
 * EffectsChain.cpp
 *
 * Implementation of effects processing chain.
 */

#include "EffectsChain.h"
#include "Debug.h"

EffectsChain::EffectsChain(uint32_t sampleRate)
    : sampleRate(sampleRate),
      delay(nullptr) {

    // Create delay effect (300ms default)
    delay = new DelayEffect(300, sampleRate);
    delay->setFeedback(0.5f);
    delay->setMix(0.3f);
    delay->setEnabled(false);  // Start disabled

    DEBUG_PRINTLN("[CHAIN] EffectsChain initialized");
}

EffectsChain::~EffectsChain() {
    if (delay != nullptr) {
        delete delay;
        delay = nullptr;
    }
    DEBUG_PRINTLN("[CHAIN] EffectsChain destroyed");
}

int16_t EffectsChain::process(int16_t input) {
    int16_t output = input;

    // Apply effects in order
    // (Only process if enabled - effect handles bypass internally)
    output = delay->process(output);

    // Future: output = chorus->process(output);
    // Future: output = reverb->process(output);

    return output;
}

void EffectsChain::setDelayEnabled(bool enabled) {
    if (delay != nullptr) {
        delay->setEnabled(enabled);
    }
}

bool EffectsChain::isDelayEnabled() const {
    return (delay != nullptr) ? delay->isEnabled() : false;
}

void EffectsChain::reset() {
    if (delay != nullptr) {
        delay->reset();
    }
    // Future: chorus->reset();
    // Future: reverb->reset();

    DEBUG_PRINTLN("[CHAIN] All effects reset");
}
