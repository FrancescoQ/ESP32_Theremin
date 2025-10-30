/*
 * ControlHandler.h
 *
 * Manages oscillator control inputs from serial commands and GPIO.
 * Provides unified interface for changing oscillator parameters.
 */

#pragma once
#include <Arduino.h>
#include "Theremin.h"

class ControlHandler {
public:
  /**
   * Constructor
   * @param theremin Pointer to Theremin instance (for full control access)
   */
  ControlHandler(Theremin* theremin);

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
   * Get waveform name from enum
   */
  const char* getWaveformName(Oscillator::Waveform wf);
};
