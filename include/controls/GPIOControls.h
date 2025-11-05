/*
 * GPIOControls.h
 *
 * Manages physical oscillator controls via MCP23017 I2C GPIO expander.
 * Handles waveform and octave switches for 3 oscillators with debouncing.
 */

#pragma once
#include <Adafruit_MCP23X17.h>
#include "audio/Oscillator.h"
#include "system/DisplayManager.h"

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
  GPIOControls(Theremin* theremin, DisplayManager* displayMgr = nullptr);

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

  /**
   * Check if modifier button is currently held (long press active)
   * @return true if modifier mode is active
   */
  bool isModifierActive() const { return modifierActive; }

  /**
   * Check if button was short-pressed since last check
   * Consumes the flag (returns true only once per press)
   * @return true if short press occurred
   */
  bool wasShortPressed();

  /**
   * Check if button was double-clicked since last check
   * Consumes the flag (returns true only once per double-click)
   * @return true if double-click occurred
   */
  bool wasDoubleClicked();

private:
  Theremin* theremin;
  DisplayManager* displayManager;
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

  // Secondary control state tracking
  int8_t lastSmoothingPreset;        // Track last smoothing preset to avoid redundant calls
  unsigned long lastSmoothingChangeTime;

  // Debounce timing
  static constexpr unsigned long DEBOUNCE_MS = 50;

  // Multi-function button state machine
  enum ButtonState {
    IDLE,                // Button not pressed
    PRESSED,             // Button just pressed (debouncing)
    LONG_PRESS_ACTIVE,   // Long press threshold reached (modifier active)
    RELEASED             // Button released (process action)
  };

  ButtonState buttonState;
  unsigned long buttonPressTime;
  bool modifierActive;      // True while long press is active
  bool shortPressFlag;      // Set on short press, cleared by wasShortPressed()

  // Double-click detection
  unsigned long firstPressReleaseTime;
  bool waitingForSecondClick;
  bool doubleClickFlag;

  static constexpr unsigned long LONG_PRESS_THRESHOLD_MS = 600;
  static constexpr unsigned long DOUBLE_CLICK_WINDOW_MS = 400;

  /**
   * Update multi-function button state machine
   * Handles debouncing and long/short press detection
   */
  void updateButton();

  /**
   * Handle secondary controls (effects, smoothing, etc.)
   * Called when modifier button is held and switches are used
   */
  void updateSecondaryControls();

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
