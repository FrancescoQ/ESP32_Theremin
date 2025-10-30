/*
 * ControlHandler.h
 *
 * Manages oscillator control inputs from serial commands and GPIO.
 * Provides unified interface for changing oscillator parameters.
 */

#pragma once
#include <Arduino.h>
#include "AudioEngine.h"

class ControlHandler {
public:
  /**
   * Constructor
   * @param audioEngine Pointer to AudioEngine instance
   */
  ControlHandler(AudioEngine* audioEngine);

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
  AudioEngine* audio;

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
};
