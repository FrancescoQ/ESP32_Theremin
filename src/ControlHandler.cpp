/*
 * ControlHandler.cpp
 *
 * Implementation of control input handling.
 */

#include "ControlHandler.h"
#include "Theremin.h"
#include "Debug.h"

ControlHandler::ControlHandler(Theremin* thereminPtr)
    : theremin(thereminPtr) {
}

void ControlHandler::begin() {
  DEBUG_PRINTLN("[CTRL] Control handler initialized");
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

int ControlHandler::parseWaveform(String name) {
  if (name == "off") {
    return Oscillator::OFF;
  }
  if (name == "square") {
    return Oscillator::SQUARE;
  }
  if (name == "sine") {
    return Oscillator::SINE;
  }
  if (name == "triangle" || name == "tri") {
    return Oscillator::TRIANGLE;
  }
  if (name == "sawtooth" || name == "saw") {
    return Oscillator::SAW;
  }
  return -1;  // Invalid
}

void ControlHandler::printStatus() {
  DEBUG_PRINTLN("\n========== OSCILLATOR STATUS ==========");
  printOscillatorStatus(1);
  printOscillatorStatus(2);
  printOscillatorStatus(3);
  DEBUG_PRINTLN("=======================================\n");
}

void ControlHandler::printOscillatorStatus(int oscNum) {
  DEBUG_PRINT("Oscillator ");
  DEBUG_PRINT(oscNum);
  DEBUG_PRINTLN(":");

  // Get current state from AudioEngine
  Oscillator::Waveform waveform = theremin->getAudioEngine()->getOscillatorWaveform(oscNum);
  int octave = theremin->getAudioEngine()->getOscillatorOctave(oscNum);
  float volume = theremin->getAudioEngine()->getOscillatorVolume(oscNum);

  // Display waveform
  DEBUG_PRINT("  Waveform:     ");
  DEBUG_PRINTLN(getWaveformName(waveform));

  // Display octave shift
  DEBUG_PRINT("  Octave Shift: ");
  if (octave > 0) {
    DEBUG_PRINT("+");
  }
  DEBUG_PRINTLN(octave);

  // Display volume as percentage
  DEBUG_PRINT("  Volume:       ");
  DEBUG_PRINT((int)(volume * 100));
  DEBUG_PRINTLN("%");
}

// Get waveform name.
const char* ControlHandler::getWaveformName(Oscillator::Waveform wf) {
  switch (wf) {
    case Oscillator::OFF: return "OFF";
    case Oscillator::SQUARE: return "SQUARE";
    case Oscillator::SINE: return "SINE";
    case Oscillator::TRIANGLE: return "TRIANGLE";
    case Oscillator::SAW: return "SAWTOOTH";
    default: return "UNKNOWN";
  }
}

void ControlHandler::printHelp() {
  DEBUG_PRINTLN("\n========== OSCILLATOR CONTROL COMMANDS ==========");
  DEBUG_PRINTLN("Waveform:");
  DEBUG_PRINTLN("  osc1:off         - Turn off oscillator 1");
  DEBUG_PRINTLN("  osc1:square      - Set oscillator 1 to square wave");
  DEBUG_PRINTLN("  osc1:sine        - Set oscillator 1 to sine wave");
  DEBUG_PRINTLN("  osc1:triangle    - Set oscillator 1 to triangle wave");
  DEBUG_PRINTLN("  osc1:sawtooth    - Set oscillator 1 to sawtooth wave");
  DEBUG_PRINTLN("\nOctave Shift:");
  DEBUG_PRINTLN("  osc1:octave:-1   - Shift oscillator 1 down one octave");
  DEBUG_PRINTLN("  osc1:octave:0    - Reset oscillator 1 to base octave");
  DEBUG_PRINTLN("  osc1:octave:1    - Shift oscillator 1 up one octave");
  DEBUG_PRINTLN("\nVolume:");
  DEBUG_PRINTLN("  osc1:vol:0.0     - Set oscillator 1 to 0% volume (silent)");
  DEBUG_PRINTLN("  osc1:vol:0.5     - Set oscillator 1 to 50% volume");
  DEBUG_PRINTLN("  osc1:vol:1.0     - Set oscillator 1 to 100% volume");
  DEBUG_PRINTLN("\nStatus:");
  DEBUG_PRINTLN("  status           - Show status of all oscillators");
  DEBUG_PRINTLN("  status:osc1      - Show status of oscillator 1");
  DEBUG_PRINTLN("\nBatch Commands:");
  DEBUG_PRINTLN("  osc1:sine;osc1:octave:1;osc1:vol:0.8");
  DEBUG_PRINTLN("  - Execute multiple commands separated by ';'");
  DEBUG_PRINTLN("\nSensor Control:");
  DEBUG_PRINTLN("  sensors:pitch:on     - Enable pitch sensor");
  DEBUG_PRINTLN("  sensors:pitch:off    - Disable pitch sensor");
  DEBUG_PRINTLN("  sensors:volume:on    - Enable volume sensor");
  DEBUG_PRINTLN("  sensors:volume:off   - Disable volume sensor");
  DEBUG_PRINTLN("  sensors:enable       - Enable both sensors (alias)");
  DEBUG_PRINTLN("  sensors:disable      - Disable both sensors (alias)");
  DEBUG_PRINTLN("  sensors:status       - Show sensor enable states");
  DEBUG_PRINTLN("\nAudio Control:");
  DEBUG_PRINTLN("  audio:freq:440       - Set frequency to 440 Hz");
  DEBUG_PRINTLN("  audio:amp:75         - Set amplitude to 75%");
  DEBUG_PRINTLN("  audio:status         - Show current audio values");
  DEBUG_PRINTLN("\nNote: Replace 'osc1' with 'osc2' or 'osc3' for other oscillators");
  DEBUG_PRINTLN("      Abbreviations: 'tri'=triangle, 'saw'=sawtooth, 'oct'=octave, 'vol'=volume");
  DEBUG_PRINTLN("      When sensors disabled, manual audio: commands persist");
  DEBUG_PRINTLN("      When sensors enabled, they override manual settings");
  DEBUG_PRINTLN("=================================================\n");
}

// Parse and execute a single command string.
// This contains the list of all supported commands, including the batch
// processing of multiple commands in one line.
void ControlHandler::executeCommand(String cmd) {
  cmd.toLowerCase();  // Case-insensitive

  // Handle batch commands (separated by semicolons)
  if (cmd.indexOf(';') != -1) {
    int start = 0;
    int semicolon;
    while ((semicolon = cmd.indexOf(';', start)) != -1) {
      String subCmd = cmd.substring(start, semicolon);
      subCmd.trim();
      if (subCmd.length() > 0) {
        executeCommand(subCmd);  // Recursive call
      }
      start = semicolon + 1;
    }
    // Execute last command
    String lastCmd = cmd.substring(start);
    lastCmd.trim();
    if (lastCmd.length() > 0) {
      executeCommand(lastCmd);
    }
    return;
  }

  // Handle help.
  if (cmd == "help" || cmd == "?") {
    printHelp();
    return;
  }

  // Handle global and oscillator status.
  if (cmd == "status") {
    printStatus();
    return;
  }

  if (cmd.startsWith("status:osc")) {
    int oscNum = cmd.charAt(10) - '0';
    if (oscNum >= 1 && oscNum <= 3) {
      printOscillatorStatus(oscNum);
    } else {
      DEBUG_PRINTLN("[CTRL] ERROR: Oscillator number must be 1-3");
    }
    return;
  }

  // Sensor control commands
  if (cmd == "sensors:pitch:on") {
    theremin->getSensorManager()->setPitchEnabled(true);
    DEBUG_PRINTLN("[CTRL] Pitch sensor enabled");
    return;
  }

  if (cmd == "sensors:pitch:off") {
    theremin->getSensorManager()->setPitchEnabled(false);
    DEBUG_PRINTLN("[CTRL] Pitch sensor disabled");
    return;
  }

  if (cmd == "sensors:volume:on") {
    theremin->getSensorManager()->setVolumeEnabled(true);
    DEBUG_PRINTLN("[CTRL] Volume sensor enabled");
    return;
  }

  if (cmd == "sensors:volume:off") {
    theremin->getSensorManager()->setVolumeEnabled(false);
    DEBUG_PRINTLN("[CTRL] Volume sensor disabled");
    return;
  }

  // Sensor control aliases to change both sensors at the same time
  // (use batch commands)
  if (cmd == "sensors:enable") {
    executeCommand("sensors:pitch:on;sensors:volume:on");
    return;
  }

  if (cmd == "sensors:disable") {
    executeCommand("sensors:pitch:off;sensors:volume:off");
    return;
  }

  // Sensor status
  if (cmd == "sensors:status") {
    DEBUG_PRINTLN("\n========== SENSOR STATUS ==========");
    DEBUG_PRINT("Pitch sensor:  ");
    DEBUG_PRINTLN(theremin->getSensorManager()->isPitchEnabled() ? "ENABLED" : "DISABLED");
    DEBUG_PRINT("Volume sensor: ");
    DEBUG_PRINTLN(theremin->getSensorManager()->isVolumeEnabled() ? "ENABLED" : "DISABLED");
    DEBUG_PRINTLN("===================================\n");
    return;
  }

  // Audio control commands
  if (cmd.startsWith("audio:freq:")) {
    int freq = cmd.substring(11).toInt();
    theremin->getAudioEngine()->setFrequency(freq);
    DEBUG_PRINT("[CTRL] Manual frequency set to ");
    DEBUG_PRINT(freq);
    DEBUG_PRINTLN(" Hz");
    return;
  }

  if (cmd.startsWith("audio:amp:")) {
    int amp = cmd.substring(10).toInt();
    theremin->getAudioEngine()->setAmplitude(amp);
    DEBUG_PRINT("[CTRL] Manual amplitude set to ");
    DEBUG_PRINT(amp);
    DEBUG_PRINTLN("%");
    return;
  }

  // Audio status
  if (cmd == "audio:status") {
    DEBUG_PRINTLN("\n========== AUDIO STATUS ==========");
    DEBUG_PRINT("Frequency: ");
    DEBUG_PRINT(theremin->getAudioEngine()->getFrequency());
    DEBUG_PRINTLN(" Hz");
    DEBUG_PRINT("Amplitude: ");
    DEBUG_PRINT(theremin->getAudioEngine()->getAmplitude());
    DEBUG_PRINTLN("%");
    DEBUG_PRINTLN("==================================\n");
    return;
  }

  // Oscillator commands (only if command starts with 'osc')
  if (cmd.startsWith("osc")) {
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
        theremin->getAudioEngine()->setOscillatorWaveform(oscNum, (Oscillator::Waveform)waveform);
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
      theremin->getAudioEngine()->setOscillatorOctave(oscNum, octave);
    } else if (paramName == "volume" || paramName == "vol") {
      float volume = value.toFloat();
      theremin->getAudioEngine()->setOscillatorVolume(oscNum, volume);
    } else {
      DEBUG_PRINT("[CTRL] ERROR: Unknown parameter: ");
      DEBUG_PRINTLN(paramName);
    }
    return;
  }

  // If we reach here, command is unknown
  DEBUG_PRINT("[CTRL] ERROR: Unknown command: ");
  DEBUG_PRINTLN(cmd);
  DEBUG_PRINTLN("[CTRL] Type 'help' for list of commands");
}
