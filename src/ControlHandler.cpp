/*
 * ControlHandler.cpp
 *
 * Implementation of control input handling.
 */

#include "ControlHandler.h"
#include "Debug.h"

ControlHandler::ControlHandler(AudioEngine* audioEngine)
    : audio(audioEngine) {
}

void ControlHandler::begin() {
  DEBUG_PRINTLN("[CTRL] Control handler initialized");
  DEBUG_PRINTLN("[CTRL] Serial commands enabled");
  DEBUG_PRINTLN("[CTRL] Command format: oscN:parameter:value");
  DEBUG_PRINTLN("[CTRL] Examples:");
  DEBUG_PRINTLN("[CTRL]   osc1:sine      - Set oscillator 1 to sine wave");
  DEBUG_PRINTLN("[CTRL]   osc2:octave:-1 - Shift oscillator 2 down one octave");
  DEBUG_PRINTLN("[CTRL]   osc3:vol:0.5   - Set oscillator 3 volume to 50%");
  DEBUG_PRINTLN("[CTRL]   osc1:off       - Turn off oscillator 1");
}

void ControlHandler::update() {
  handleSerialCommands();
  // GPIO reading will be added in Phase D
}

void ControlHandler::handleSerialCommands() {
  // Check if serial data available
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();  // Remove whitespace

    if (command.length() > 0) {
      DEBUG_PRINT("[CTRL] Received command: ");
      DEBUG_PRINTLN(command);
      executeCommand(command);
    }
  }
}

void ControlHandler::executeCommand(String cmd) {
  // Parse command format: oscN:parameter:value
  // Examples: "osc1:sine", "osc2:octave:-1", "osc3:vol:0.5"

  cmd.toLowerCase();  // Case-insensitive

  // Extract oscillator number
  if (!cmd.startsWith("osc")) {
    DEBUG_PRINTLN("[CTRL] ERROR: Command must start with 'osc'");
    return;
  }

  int oscNum = cmd.charAt(3) - '0';  // Get number after 'osc'
  if (oscNum < 1 || oscNum > 3) {
    DEBUG_PRINTLN("[CTRL] ERROR: Oscillator number must be 1-3");
    return;
  }

  // Find first colon
  int colonPos = cmd.indexOf(':', 4);
  if (colonPos == -1) {
    DEBUG_PRINTLN("[CTRL] ERROR: Missing ':' separator");
    return;
  }

  // Extract parameter (everything after first colon)
  String param = cmd.substring(colonPos + 1);

  // Check if waveform command (no second colon)
  if (param.indexOf(':') == -1) {
    // Waveform command (e.g., "osc1:sine" or "osc1:off")
    int waveform = parseWaveform(param);
    if (waveform >= 0) {
      audio->setOscillatorWaveform(oscNum, (Oscillator::Waveform)waveform);
    } else {
      DEBUG_PRINT("[CTRL] ERROR: Unknown waveform: ");
      DEBUG_PRINTLN(param);
    }
    return;
  }

  // Parameter with value (e.g., "osc1:octave:-1" or "osc1:vol:0.5")
  int secondColon = param.indexOf(':');
  String paramName = param.substring(0, secondColon);
  String value = param.substring(secondColon + 1);

  if (paramName == "octave" || paramName == "oct") {
    int octave = value.toInt();
    audio->setOscillatorOctave(oscNum, octave);
  } else if (paramName == "volume" || paramName == "vol") {
    float volume = value.toFloat();
    audio->setOscillatorVolume(oscNum, volume);
  } else {
    DEBUG_PRINT("[CTRL] ERROR: Unknown parameter: ");
    DEBUG_PRINTLN(paramName);
  }
}

int ControlHandler::parseWaveform(String name) {
  if (name == "off") return Oscillator::OFF;
  if (name == "square") return Oscillator::SQUARE;
  if (name == "sine") return Oscillator::SINE;
  if (name == "triangle" || name == "tri") return Oscillator::TRIANGLE;
  if (name == "sawtooth" || name == "saw") return Oscillator::SAW;
  return -1;  // Invalid
}
