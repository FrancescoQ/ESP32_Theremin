/*
 * GPIOControls.h
 *
 * Manages physical oscillator controls via MCP23017 I2C GPIO expander.
 * Handles waveform and octave switches for 3 oscillators with debouncing.
 */

#pragma once
#include <Adafruit_MCP23X17.h>
#include "Oscillator.h"

// Forward declaration to avoid circular dependency:
// Theremin.h includes ControlHandler.h which includes GPIOControls.h
// GPIOControls only needs a pointer to Theremin, so we declare it here
// Full definition included in GPIOControls.cpp
class Theremin;

class GPIOControls {
public:
  /**
   * Constructor
   * @param theremin Pointer to Theremin instance (for oscillator control)
   */
  GPIOControls(Theremin* theremin);

  /**
   * Initialize MCP23017 and configure all pins as inputs
   * @return true if successful, false if MCP23017 not found
   */
  bool begin();

  /**
   * Read all switches and update oscillator settings
   * Call from ControlHandler::update()
   */
  void update();

  /**
   * Check if GPIO controls are operational
   * @return true if MCP23017 initialized successfully
   */
  bool isInitialized() const { return initialized; }

  /**
   * Enable or disable GPIO controls
   * Useful for web interface override
   * @param enabled True to enable, false to disable
   */
  void setEnabled(bool enabled);

  /**
   * Check if GPIO controls are enabled
   * @return true if enabled
   */
  bool isEnabled() const { return controlsEnabled; }

private:
  Theremin* theremin;
  Adafruit_MCP23X17 mcp;
  bool initialized;
  bool controlsEnabled;  // Master enable/disable for web interface override
  bool firstUpdate;      // Force initial sync with switch positions

  // State tracking for debouncing
  struct OscillatorState {
    Oscillator::Waveform waveform;
    int8_t octave;
    unsigned long lastChangeTime;
  };

  OscillatorState osc1State;
  OscillatorState osc2State;
  OscillatorState osc3State;

  // Debounce timing
  static constexpr unsigned long DEBOUNCE_MS = 50;

  /**
   * Read waveform from 3-pin switch
   * @param pinA Sine wave pin
   * @param pinB Triangle wave pin
   * @param pinC Square wave pin
   * @return Waveform (OFF if all HIGH)
   */
  Oscillator::Waveform readWaveform(uint8_t pinA, uint8_t pinB, uint8_t pinC);

  /**
   * Read octave from 2-pin switch
   * @param pinUp +1 octave pin
   * @param pinDown -1 octave pin
   * @return Octave shift (-1, 0, +1)
   */
  int8_t readOctave(uint8_t pinUp, uint8_t pinDown);

  /**
   * Update single oscillator if state changed
   * @param oscNum Oscillator number (1-3)
   * @param state State tracking struct
   * @param pinA, pinB, pinC Waveform pins
   * @param pinUp, pinDown Octave pins
   */
  void updateOscillator(int oscNum, OscillatorState& state,
                        uint8_t pinA, uint8_t pinB, uint8_t pinC,
                        uint8_t pinUp, uint8_t pinDown);

  /**
   * Get waveform name for debug output
   */
  const char* getWaveformName(Oscillator::Waveform wf);
};
