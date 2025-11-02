/*
 * GPIOControls.cpp
 *
 * Implementation of physical control handling via MCP23017.
 */

#include "controls/GPIOControls.h"
#include "system/Theremin.h"
#include "system/PinConfig.h"
#include "system/Debug.h"

GPIOControls::GPIOControls(Theremin* thereminPtr)
    : theremin(thereminPtr), initialized(false), controlsEnabled(true), firstUpdate(true) {
  // Initialize state tracking
  osc1State.waveform = Oscillator::OFF;
  osc1State.octave = 0;
  osc1State.lastChangeTime = 0;

  osc2State.waveform = Oscillator::OFF;
  osc2State.octave = 0;
  osc2State.lastChangeTime = 0;

  osc3State.waveform = Oscillator::OFF;
  osc3State.octave = 0;
  osc3State.lastChangeTime = 0;
}

bool GPIOControls::begin() {
  // Initialize MCP23017 at address 0x20
  if (!mcp.begin_I2C(PIN_SWITCH_EXPANDER_ADDR)) {
    DEBUG_PRINTLN("[GPIO] Failed to initialize MCP23017");
    return false;
  }

  // Configure all 16 pins as inputs with pullups
  for (uint8_t pin = 0; pin < 16; pin++) {
    mcp.pinMode(pin, INPUT_PULLUP);
  }

  DEBUG_PRINTLN("[GPIO] MCP23017 initialized");
  DEBUG_PRINTLN("[GPIO] Oscillator switches configured:");
  DEBUG_PRINTLN("[GPIO]   OSC1: Waveform pins 6,5,14 | Octave pins 7,15");
  DEBUG_PRINTLN("[GPIO]   OSC2: Waveform pins 4,11,3 | Octave pins 12,13");
  DEBUG_PRINTLN("[GPIO]   OSC3: Waveform pins 1,9,0  | Octave pins 10,2");

  initialized = true;

  // Read initial positions and apply
  DEBUG_PRINTLN("[GPIO] Reading initial switch positions...");
  update();

  return true;
}

void GPIOControls::update() {
  if (!initialized || !controlsEnabled) {
    return;
  }

  // Update each oscillator
  updateOscillator(1, osc1State,
                   PIN_OSC1_WAVE_A, PIN_OSC1_WAVE_B, PIN_OSC1_WAVE_C,
                   PIN_OSC1_OCT_UP, PIN_OSC1_OCT_DOWN);

  updateOscillator(2, osc2State,
                   PIN_OSC2_WAVE_A, PIN_OSC2_WAVE_B, PIN_OSC2_WAVE_C,
                   PIN_OSC2_OCT_UP, PIN_OSC2_OCT_DOWN);

  updateOscillator(3, osc3State,
                   PIN_OSC3_WAVE_A, PIN_OSC3_WAVE_B, PIN_OS3_WAVE_C,
                   PIN_OSC3_OCT_UP, PIN_OSC3_OCT_DOWN);

  // Clear first update flag after initial sync
  if (firstUpdate) {
    firstUpdate = false;
  }
}

void GPIOControls::updateOscillator(int oscNum, OscillatorState& state,
                                    uint8_t pinA, uint8_t pinB, uint8_t pinC,
                                    uint8_t pinUp, uint8_t pinDown) {
  unsigned long now = millis();

  // Read current switch positions
  Oscillator::Waveform currentWaveform = readWaveform(pinA, pinB, pinC);
  int8_t currentOctave = readOctave(pinUp, pinDown);

  // Check for waveform change (force update on first call)
  if (firstUpdate || currentWaveform != state.waveform) {
    if (firstUpdate || (now - state.lastChangeTime > DEBOUNCE_MS)) {
      state.waveform = currentWaveform;
      state.lastChangeTime = now;

      theremin->getAudioEngine()->setOscillatorWaveform(oscNum, currentWaveform);

      DEBUG_PRINT("[GPIO] OSC");
      DEBUG_PRINT(oscNum);
      DEBUG_PRINT(" waveform: ");
      DEBUG_PRINTLN(getWaveformName(currentWaveform));
    }
  }

  // Check for octave change (force update on first call)
  if (firstUpdate || currentOctave != state.octave) {
    if (firstUpdate || (now - state.lastChangeTime > DEBOUNCE_MS)) {
      state.octave = currentOctave;
      state.lastChangeTime = now;

      theremin->getAudioEngine()->setOscillatorOctave(oscNum, currentOctave);

      DEBUG_PRINT("[GPIO] OSC");
      DEBUG_PRINT(oscNum);
      DEBUG_PRINT(" octave: ");
      if (currentOctave > 0) {
        DEBUG_PRINT("+");
      }
      DEBUG_PRINTLN(currentOctave);
    }
  }
}

Oscillator::Waveform GPIOControls::readWaveform(uint8_t pinA, uint8_t pinB, uint8_t pinC) {
  // Read pins (active LOW with pullup)
  bool sineActive = (mcp.digitalRead(pinA) == LOW);
  bool triActive = (mcp.digitalRead(pinB) == LOW);
  bool squareActive = (mcp.digitalRead(pinC) == LOW);

  // If all HIGH (none active) → OFF
  if (!sineActive && !triActive && !squareActive) {
    return Oscillator::OFF;
  }

  // Return whichever is active (priority: sine > square > triangle)
  if (sineActive) {
    return Oscillator::SINE;
  }
  if (triActive) {
    return Oscillator::SQUARE;
  }
  if (squareActive) {
    return Oscillator::TRIANGLE;
  }

  // Shouldn't reach here, but safe default
  return Oscillator::OFF;
}

int8_t GPIOControls::readOctave(uint8_t pinUp, uint8_t pinDown) {
  // Read pins (active LOW with pullup)
  bool upActive = (mcp.digitalRead(pinUp) == LOW);
  bool downActive = (mcp.digitalRead(pinDown) == LOW);

  // Both LOW → center position (0)
  if (upActive && downActive) {
    return 0;
  }

  // One active → that direction
  if (upActive && !downActive) {
    return +1;
  }
  if (downActive && !upActive) {
    return -1;
  }

  // Both active (invalid) → default to 0
  DEBUG_PRINTLN("[GPIO] WARNING: Both octave switches active!");
  return 0;
}

void GPIOControls::setEnabled(bool enabled) {
  controlsEnabled = enabled;
  DEBUG_PRINT("[GPIO] Physical controls ");
  DEBUG_PRINTLN(enabled ? "enabled" : "disabled");
}

const char* GPIOControls::getWaveformName(Oscillator::Waveform wf) {
  switch (wf) {
    case Oscillator::OFF:
      return "OFF";
    case Oscillator::SQUARE:
      return "SQUARE";
    case Oscillator::SINE:
      return "SINE";
    case Oscillator::TRIANGLE:
      return "TRIANGLE";
    case Oscillator::SAW:
      return "SAWTOOTH";
    default:
      return "UNKNOWN";
  }
}
