/*
 * SerialControls.h
 *
 * Manages oscillator control inputs from serial commands.
 * Provides interface for changing oscillator parameters via serial.
 */

#pragma once
#include <Arduino.h>
#include "audio/Oscillator.h"

// Forward declaration to avoid circular dependency:
// Theremin.h includes SerialControls.h (to contain SerialControls as a member)
// SerialControls only needs a pointer to Theremin, so we declare it here
// instead of including Theremin.h. Full definition included in SerialControls.cpp
class Theremin;

class SerialControls {
public:
  /**
   * Constructor
   * @param theremin Pointer to Theremin instance (for full control access)
   */
  SerialControls(Theremin* theremin);

  /**
   * Initialize control handler
   */
  void begin();

  /**
   * Update control handler (read serial, GPIO, etc.)
   * Call from main loop
   */
  void update();

private:
  Theremin* theremin;

  /**
   * Check for and process serial commands
   */
  void handleSerialCommands();

  /**
   * Parse and execute a single command string
   * @param cmd Command string (e.g., "osc1:sine")
   */
  void executeCommand(String cmd);

  /**
   * Parse waveform name to enum
   * @param name Waveform name ("off", "square", "sine", "triangle", "saw")
   * @return Waveform enum or -1 if invalid
   */
  int parseWaveform(String name);

  /**
   * Print help text
   */
  void printHelp();

  /**
   * Print status of all oscillators
   */
  void printStatus();

  /**
   * Print status of specific oscillator
   */
  void printOscillatorStatus(int oscNum);

  /**
   * Print status of all effects
   */
  void printEffectsStatus();

  /**
   * Get waveform name from enum
   */
  const char* getWaveformName(Oscillator::Waveform wf);
};
