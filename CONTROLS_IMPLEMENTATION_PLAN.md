# Controls Implementation Plan
**ESP32 Theremin - Runtime Oscillator Control System**

**Date:** October 29, 2025
**Goal:** Implement runtime control of oscillator parameters (waveform, octave, volume)
**Approach:** Phased implementation with serial testing before hardware integration

---

## Overview

This plan implements a control system that allows runtime modification of oscillator parameters. The implementation is split into 5 phases, each building on the previous one, allowing for incremental testing and debugging.

## Current Status

**✅ Phase A: Complete** - AudioEngine control methods implemented and tested
**✅ Bonus Features: Complete** - Melody player, system test, and startup sound
**✅ Phase B: Complete** - Serial command parser (ControlHandler class)
**✅ Refinements: Complete** - Serial initialization fix, code style improvements
**✅ Phase C: Complete** - Extended serial commands (help, status, batch) + Status introspection with getters
**✅ Phase C2: Complete** - Audio & Sensor Control Commands
**✅ Phase D: Complete** - GPIO controls with MCP23017 via GPIOControls class
**✅ Architecture Refactor: Complete** - ControlHandler → SerialControls, GPIO elevated to sibling
**✅ Phase E: Complete** - Polish & integration with startup bug fix and enable/disable features

### Key Architectural Principles

1. **Thread Safety:** AudioEngine runs on Core 1 (FreeRTOS task), controls read on Core 0 (main loop)
2. **Mutex Protection:** All parameter changes must be protected by `paramMutex`
3. **Separation of Concerns:** AudioEngine provides API, ControlHandler manages input sources
4. **Test-First:** Serial commands enable testing without hardware
5. **Single Source of Truth:** Both serial and GPIO trigger the same AudioEngine methods

### Thread Safety Explanation

Your audio runs in a **separate FreeRTOS task** on Core 1:
```cpp
// Core 1 - Running continuously
void audioTaskLoop() {
    while (taskRunning) {
        generateAudioBuffer();  // Reading oscillator parameters
    }
}
```

Meanwhile, your main loop runs on Core 0:
```cpp
// Core 0 - Your main loop
void loop() {
    theremin.update();  // Might want to change oscillator parameters
}
```

**The Problem:** Both cores could access the same memory at the same time = data corruption!

**The Solution:** Mutex (mutual exclusion) acts like a "talking stick":
```cpp
// Core 0 asks: "Can I change parameters?"
xSemaphoreTake(paramMutex, portMAX_DELAY);  // Wait for stick

// Now Core 0 has exclusive access
oscillator1.setFrequency(440.0);  // SAFE to change

// Core 0 returns the stick
xSemaphoreGive(paramMutex);  // Core 1 can use it now
```

In `generateAudioBuffer()`, Core 1 also requests the mutex before reading parameters, ensuring no conflicts.

---

## Phase A: AudioEngine Control Methods
**Goal:** Add thread-safe API to AudioEngine for changing oscillator parameters
**Testing:** Call methods directly from `setup()` to verify they work
**Files Modified:** `include/AudioEngine.h`, `src/AudioEngine.cpp`

### Implementation Steps

#### Step A1: Add Method Declarations to AudioEngine.h

Add these public methods after `setAmplitude()`:

```cpp
/**
 * Set waveform for specific oscillator
 * @param oscNum Oscillator number (1-3)
 * @param wf Waveform type (OFF, SQUARE, SINE, TRIANGLE, SAW)
 */
void setOscillatorWaveform(int oscNum, Oscillator::Waveform wf);

/**
 * Set octave shift for specific oscillator
 * @param oscNum Oscillator number (1-3)
 * @param octave Octave shift (-1, 0, +1)
 */
void setOscillatorOctave(int oscNum, int octave);

/**
 * Set volume for specific oscillator
 * @param oscNum Oscillator number (1-3)
 * @param volume Volume level (0.0 = silent, 1.0 = full)
 */
void setOscillatorVolume(int oscNum, float volume);
```

#### Step A2: Implement Methods in AudioEngine.cpp

Add these implementations at the end of the file:

```cpp
void AudioEngine::setOscillatorWaveform(int oscNum, Oscillator::Waveform wf) {
  // Validate oscillator number
  if (oscNum < 1 || oscNum > 3) {
    DEBUG_PRINT("[AUDIO] Invalid oscillator number: ");
    DEBUG_PRINTLN(oscNum);
    return;
  }

  // Thread-safe parameter update
  if (paramMutex != NULL && xSemaphoreTake(paramMutex, portMAX_DELAY) == pdTRUE) {
    switch (oscNum) {
      case 1:
        oscillator1.setWaveform(wf);
        break;
      case 2:
        oscillator2.setWaveform(wf);
        break;
      case 3:
        oscillator3.setWaveform(wf);
        break;
    }

    DEBUG_PRINT("[AUDIO] Oscillator ");
    DEBUG_PRINT(oscNum);
    DEBUG_PRINT(" waveform set to ");
    DEBUG_PRINTLN((int)wf);

    xSemaphoreGive(paramMutex);
  }
}

void AudioEngine::setOscillatorOctave(int oscNum, int octave) {
  // Validate oscillator number
  if (oscNum < 1 || oscNum > 3) {
    DEBUG_PRINT("[AUDIO] Invalid oscillator number: ");
    DEBUG_PRINTLN(oscNum);
    return;
  }

  // Validate octave range
  if (octave < -1 || octave > 1) {
    DEBUG_PRINT("[AUDIO] Invalid octave shift: ");
    DEBUG_PRINTLN(octave);
    return;
  }

  // Thread-safe parameter update
  if (paramMutex != NULL && xSemaphoreTake(paramMutex, portMAX_DELAY) == pdTRUE) {
    switch (oscNum) {
      case 1:
        oscillator1.setOctaveShift(octave);
        break;
      case 2:
        oscillator2.setOctaveShift(octave);
        break;
      case 3:
        oscillator3.setOctaveShift(octave);
        break;
    }

    DEBUG_PRINT("[AUDIO] Oscillator ");
    DEBUG_PRINT(oscNum);
    DEBUG_PRINT(" octave shift set to ");
    DEBUG_PRINTLN(octave);

    xSemaphoreGive(paramMutex);
  }
}

void AudioEngine::setOscillatorVolume(int oscNum, float volume) {
  // Validate oscillator number
  if (oscNum < 1 || oscNum > 3) {
    DEBUG_PRINT("[AUDIO] Invalid oscillator number: ");
    DEBUG_PRINTLN(oscNum);
    return;
  }

  // Thread-safe parameter update
  if (paramMutex != NULL && xSemaphoreTake(paramMutex, portMAX_DELAY) == pdTRUE) {
    switch (oscNum) {
      case 1:
        oscillator1.setVolume(volume);
        break;
      case 2:
        oscillator2.setVolume(volume);
        break;
      case 3:
        oscillator3.setVolume(volume);
        break;
    }

    DEBUG_PRINT("[AUDIO] Oscillator ");
    DEBUG_PRINT(oscNum);
    DEBUG_PRINT(" volume set to ");
    DEBUG_PRINTLN(volume);

    xSemaphoreGive(paramMutex);
  }
}
```

#### Step A3: Test from main.cpp

Add test code to `setup()` after `theremin.begin()`:

```cpp
// Test Phase A: Direct API calls
DEBUG_PRINTLN("\n[TEST] Testing oscillator control API...");
delay(2000);  // Play default sound for 2 seconds

// Change oscillator 1 to sawtooth
DEBUG_PRINTLN("[TEST] Changing OSC1 to sawtooth...");
theremin.getAudioEngine()->setOscillatorWaveform(1, Oscillator::SAW);
delay(2000);

// Change oscillator 1 octave down
DEBUG_PRINTLN("[TEST] Shifting OSC1 down one octave...");
theremin.getAudioEngine()->setOscillatorOctave(1, -1);
delay(2000);

// Change oscillator 1 volume to 50%
DEBUG_PRINTLN("[TEST] Setting OSC1 volume to 50%...");
theremin.getAudioEngine()->setOscillatorVolume(1, 0.5);
delay(2000);

DEBUG_PRINTLN("[TEST] Phase A test complete!\n");
```

**Note:** You'll need to add a getter to Theremin class:
```cpp
// In Theremin.h (public section)
AudioEngine* getAudioEngine() { return &audio; }
```

### Success Criteria for Phase A

- [x] Code compiles without errors
- [x] Audio starts with default oscillator settings
- [x] Oscillator control methods work correctly
- [x] Thread-safe parameter updates via mutex
- [x] Serial output shows all debug messages
- [x] No crashes or audio glitches

### Phase A: COMPLETED ✅

**Date Completed:** October 29, 2025

**Files Modified:**
- `include/AudioEngine.h` - Added control method declarations
- `src/AudioEngine.cpp` - Implemented control methods (~80 lines)
- `include/Theremin.h` - Added `getAudioEngine()` getter
- `src/main.cpp` - Added test sequence (replaced with startup sound)

**Methods Implemented:**
1. `setOscillatorWaveform(oscNum, waveform)` - Thread-safe waveform control
2. `setOscillatorOctave(oscNum, octave)` - Thread-safe octave shift control
3. `setOscillatorVolume(oscNum, volume)` - Thread-safe volume control

**Build Impact:**
- RAM: 7.3% (23,960 bytes) - No increase
- Flash: 25.9% (338,949 bytes) - +796 bytes from baseline
- Compilation time: ~3.1 seconds

---

## Bonus Features: Melody Player & System Test

**Implemented:** October 29, 2025
**Status:** ✅ Complete

Beyond Phase A, several enhancement features were implemented to improve the theremin's capabilities and user experience.

### 1. Musical Note Constants (54 Notes)

Added comprehensive note definitions to `AudioEngine.h`:
- **Octave 3:** C3 (131 Hz) through B3 (247 Hz)
- **Octave 4:** C4 (262 Hz) through B4 (494 Hz) - Middle octave, includes A4 = 440 Hz
- **Octave 5:** C5 (523 Hz) through B5 (988 Hz)
- **NOTE_REST:** For silences/pauses in melodies

### 2. playMelody() Method

**Signature:**
```cpp
void playMelody(const int notes[], const int durations[], int length,
                int oscNum = 1, Oscillator::Waveform waveform = Oscillator::SINE,
                float staccato = 1.0);
```

**Features:**
- Plays any sequence of notes on any oscillator with any waveform
- Saves and restores previous audio state (non-destructive)
- Supports NOTE_REST for pauses
- **Staccato parameter** for automatic note articulation:
  - `1.0` = Legato (smooth, connected)
  - `0.8` = Standard staccato (crisp separation)
  - `0.5` = Staccatissimo (very short notes)
- Automatic array length calculation using `sizeof()` to prevent counting errors

**Implementation Details:**
- ~60 lines of code in AudioEngine.cpp
- Uses mutex for thread-safe state save/restore
- Properly saves actual waveform state via `getWaveform()` (bug fix)

### 3. systemTest() Method

Automated test sequence demonstrating all control capabilities:
1. Sine wave at base octave, 100% volume (1.5s)
2. Triangle waveform change (1.5s)
3. Octave shift up (1.5s)
4. Volume reduction to 50% (1.5s)
5. Restore defaults (1s)

**Purpose:**
- Hardware validation
- Audible confirmation of control functionality
- Demonstration of capabilities

### 4. playStartupSound() Method

Plays Final Fantasy VII Victory Fanfare on startup:
- 10 notes with realistic timing
- Square wave for classic 8-bit feel
- 0.8 staccato for crisp articulation
- ~2 seconds duration

**Implementation:**
```cpp
const int ff7_melody[] = {
    NOTE_C5, NOTE_C5, NOTE_C5, NOTE_C5,
    NOTE_GS4, NOTE_AS4, NOTE_C5, NOTE_REST, NOTE_AS4, NOTE_C5
};
const int ff7_durations[] = {
    150, 150, 150, 450,
    450, 450, 150, 150, 150, 600
};
constexpr int ff7_length = sizeof(ff7_melody) / sizeof(ff7_melody[0]);
playMelody(ff7_melody, ff7_durations, ff7_length, 1, Oscillator::SQUARE, 0.8);
```

### 5. Oscillator.getWaveform() Getter

**Bug Fix:** Added to properly save/restore oscillator state in `playMelody()`

**Before (buggy):**
```cpp
// WRONG: Assumed active = SINE
Oscillator::Waveform saved = osc.isActive() ? Oscillator::SINE : Oscillator::OFF;
```

**After (correct):**
```cpp
// CORRECT: Get actual waveform
Oscillator::Waveform saved = osc.getWaveform();
```

### Usage Examples

**Play a simple melody:**
```cpp
const int notes[] = {NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5};
const int durations[] = {250, 250, 250, 500};
audio.playMelody(notes, durations, 4);
```

**With custom oscillator and waveform:**
```cpp
audio.playMelody(notes, durations, 4, 2, Oscillator::SAW);
```

**With staccato articulation:**
```cpp
audio.playMelody(notes, durations, 4, 1, Oscillator::SQUARE, 0.8);
```

### Future Melody Ideas

**Suggested additions for settings notifications:**
- Super Mario: 1-Up sound, coin sound, level complete
- Zelda: Secret found, item get, treasure chest
- Pokemon: Pokemon Center healing
- Use for preset changes, OTA complete, error notifications

### Build Impact Summary

**Total additions:**
- ~200 lines of code
- 54 note constants (negligible Flash)
- RAM: 7.3% (no increase - notes are const)
- Flash: 25.9% (+796 bytes total)
- All features compile cleanly with no warnings

---

## Phase B: Serial Command Parser (Testing Infrastructure)
**Goal:** Create ControlHandler class with serial command parsing
**Testing:** Control oscillators via Serial Monitor
**Files Created:** `include/ControlHandler.h`, `src/ControlHandler.cpp`
**Files Modified:** `src/main.cpp`

### Implementation Steps

#### Step B1: Create ControlHandler.h

Create `include/ControlHandler.h`:

```cpp
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
```

#### Step B2: Create ControlHandler.cpp

Create `src/ControlHandler.cpp`:

```cpp
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
```

#### Step B3: Integrate into main.cpp

Modify `src/main.cpp`:

```cpp
// Add include at top
#include "ControlHandler.h"

// Add after theremin instance (around line 20)
ControlHandler controls(nullptr);  // Will be initialized in setup()

void setup() {
  // ... existing setup code ...

  // After theremin.begin()

  // Initialize control handler (pass audio engine pointer)
  controls = ControlHandler(theremin.getAudioEngine());
  controls.begin();

  // Remove Phase A test code (direct API calls)
}

void loop() {
  // Add before theremin.update()
  controls.update();  // Handle serial commands

  // ... rest of loop ...
}
```

**Note:** Remember to add `getAudioEngine()` getter to Theremin class (see Phase A).

### Success Criteria for Phase B

- [x] Code compiles without errors
- [x] Control handler initializes and prints help text
- [x] Can send `osc1:sine` → oscillator 1 changes to sine wave
- [x] Can send `osc2:square` → oscillator 2 changes to square wave
- [x] Can send `osc1:octave:1` → oscillator 1 shifts up one octave
- [x] Can send `osc3:vol:0.3` → oscillator 3 volume reduces
- [x] Can send `osc2:off` → oscillator 2 turns off
- [x] Invalid commands print error messages
- [x] Audio responds smoothly to commands without glitches

### Phase B: COMPLETED ✅

**Date Completed:** October 30, 2025

**Files Created:**
- `include/ControlHandler.h` - Class interface (~50 lines)
- `src/ControlHandler.cpp` - Command parser implementation (~115 lines)

**Files Modified:**
- `src/main.cpp` - Added ControlHandler integration (4 changes)

**Features Implemented:**
1. Serial command parser with format: `oscN:parameter:value`
2. Waveform commands: `osc1:sine`, `osc1:square`, `osc1:triangle`, `osc1:sawtooth`, `osc1:off`
3. Octave commands: `osc1:octave:-1`, `osc1:octave:0`, `osc1:octave:1`
4. Volume commands: `osc1:vol:0.0` to `osc1:vol:1.0`
5. Support for abbreviated commands: `tri` for triangle, `saw` for sawtooth, `oct` for octave, `vol` for volume
6. Case-insensitive command parsing
7. Error handling with helpful error messages
8. Non-blocking serial reading in main loop

**Build Impact:**
- RAM: 7.3% (23,960 bytes) - No increase from Phase A
- Flash: 26.6% (348,389 bytes) - +9,440 bytes for control system
- Compilation time: ~3.4 seconds

**Additional Refinements:**
- **Serial Initialization Fix:** Changed from DEBUG_INIT() to direct Serial.begin() to ensure Serial is always available for commands, regardless of DEBUG_MODE setting
- **Code Style:** Added braces to all single-line if statements in parseWaveform() for consistency and maintainability

**Command Examples:**
```
osc1:sine          # Set oscillator 1 to sine wave
osc2:square        # Set oscillator 2 to square wave
osc1:tri           # Set oscillator 1 to triangle (abbreviated)
osc1:octave:-1     # Shift oscillator 1 down one octave
osc2:oct:1         # Shift oscillator 2 up one octave (abbreviated)
osc3:vol:0.5       # Set oscillator 3 to 50% volume
osc1:off           # Turn off oscillator 1
```

**Testing Notes:**
- All commands execute without audio glitches
- Thread-safe parameter updates via AudioEngine API
- Serial commands work alongside theremin sensor control
- Invalid commands provide clear error messages

---

## Phase C: Extended Serial Commands
**Goal:** Add more commands, better error handling, status queries
**Testing:** Full command set via Serial Monitor
**Files Modified:** `include/ControlHandler.h`, `src/ControlHandler.cpp`

### Implementation Steps

#### Step C1: Add Status Query Commands

Add to `ControlHandler.cpp` in `executeCommand()` before parameter parsing:

```cpp
// Handle special commands
if (cmd == "help" || cmd == "?") {
  printHelp();
  return;
}

if (cmd == "status") {
  printStatus();
  return;
}

if (cmd.startsWith("status:osc")) {
  int oscNum = cmd.charAt(10) - '0';
  if (oscNum >= 1 && oscNum <= 3) {
    printOscillatorStatus(oscNum);
  }
  return;
}
```

#### Step C2: Add Helper Methods

Add method declarations to `ControlHandler.h` (private section):

```cpp
private:
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
```

Add implementations to `ControlHandler.cpp`:

```cpp
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
  DEBUG_PRINTLN("\nNote: Replace 'osc1' with 'osc2' or 'osc3' for other oscillators");
  DEBUG_PRINTLN("=================================================\n");
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
  DEBUG_PRINTLN("  [Status will be implemented when AudioEngine provides getters]");
  DEBUG_PRINTLN("  Current implementation: Check debug output from set commands");
}

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
```

**Note:** Full status reporting requires adding getter methods to AudioEngine. For now, we rely on debug output from setter methods.

#### Step C3: Add Batch Commands (Optional Enhancement)

Add support for setting multiple parameters at once:

```cpp
// In executeCommand(), add before waveform parsing:
if (cmd.indexOf(';') != -1) {
  // Batch command (e.g., "osc1:sine;osc1:octave:1;osc1:vol:0.8")
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
```

### Success Criteria for Phase C

- [x] `help` command prints complete command reference
- [x] `status` command shows all oscillator states
- [x] `status:osc1` shows single oscillator state
- [x] Batch commands work (e.g., `osc1:sine;osc1:vol:0.5`)
- [x] All commands from Phase B still work
- [x] Error messages are clear and helpful

### Phase C: COMPLETED ✅

**Date Completed:** October 30, 2025

**Files Modified:**
- `include/ControlHandler.h` - Added helper method declarations (~20 lines)
- `src/ControlHandler.cpp` - Implemented special commands and helpers (~80 lines)

**Features Implemented:**
1. **Help Command** - `help` or `?` prints complete command reference
   - Organized by category: Waveform, Octave, Volume, Status, Batch
   - Shows abbreviations and usage examples
   - Formatted help text with clear sections

2. **Status Commands** - Query current oscillator state
   - `status` - Shows all 3 oscillators
   - `status:osc1/2/3` - Shows specific oscillator
   - Note: Full introspection pending AudioEngine getters (future enhancement)
   - Currently shows placeholder text directing to debug output

3. **Batch Commands** - Execute multiple commands in one line
   - Format: `cmd1;cmd2;cmd3` separated by semicolons
   - Example: `osc1:sine;osc1:octave:1;osc1:vol:0.8`
   - Recursive implementation handles any number of commands
   - Whitespace trimming for clean parsing

4. **Helper Methods**
   - `printHelp()` - Comprehensive command reference
   - `printStatus()` - All oscillators status
   - `printOscillatorStatus(int)` - Single oscillator status
   - `getWaveformName(Waveform)` - Enum to string conversion

**Build Impact:**
- RAM: 7.3% (23,960 bytes) - No increase from Phase B
- Flash: 26.8% (350,693 bytes) - +2,304 bytes for extended commands
- Compilation time: ~3.5 seconds

**Command Examples:**
```
help                                          # Show all commands
?                                             # Short form of help
status                                        # Show all oscillator states
status:osc1                                   # Show oscillator 1 state
osc1:sine;osc1:octave:1;osc1:vol:0.8         # Batch: sine + octave up + 80% volume
osc2:square;osc2:octave:-1;osc3:triangle     # Multiple oscillators at once
```

**Testing Notes:**
- All commands execute cleanly without audio glitches
- Batch commands process sequentially via recursive calls
- Status commands work but show placeholder (getters needed for full introspection)
- Help text is clear and comprehensive
- All Phase B commands remain functional

**Status Introspection Enhancement (October 30, 2025):**
- Added AudioEngine getter methods: `getOscillatorWaveform()`, `getOscillatorOctave()`, `getOscillatorVolume()`
- Added Oscillator getter methods: `getOctaveShift()`, `getVolume()` (inline)
- Updated `printOscillatorStatus()` to display real-time oscillator configuration
- Thread-safe getters using mutex protection (consistent with setters)
- Build impact: +432 bytes Flash (minimal), no RAM increase
- Now shows actual oscillator state instead of placeholder text

**Status Output Example:**
```
========== OSCILLATOR STATUS ==========
Oscillator 1:
  Waveform:     TRIANGLE
  Octave Shift: 0
  Volume:       100%

Oscillator 2:
  Waveform:     SINE
  Octave Shift: +1
  Volume:       100%

Oscillator 3:
  Waveform:     TRIANGLE
  Octave Shift: -1
  Volume:       100%
```

---

## Phase C2: Audio & Sensor Control Commands
**Goal:** Add commands to control audio parameters and sensor enable/disable
**Testing:** Manual control via serial, sensor override testing
**Files Modified:** `include/SensorManager.h`, `src/SensorManager.cpp`, `include/ControlHandler.h`, `src/ControlHandler.cpp`, `include/Theremin.h`, `src/Theremin.cpp`, `src/main.cpp`

### Overview

Phase C2 adds advanced control capabilities for directly setting audio parameters and enabling/disabling sensor input. This allows:
- **Direct audio control:** Set fixed frequency/amplitude, bypassing sensors
- **Sensor control:** Enable/disable pitch and volume sensors independently
- **Testing flexibility:** Test fixed audio parameters or individual sensors
- **Performance mode:** Combine sensor control with manual parameter setting

### Architecture

**State Management - Stored in SensorManager:**
```cpp
class SensorManager {
private:
    bool pitchEnabled;   // Default: true
    bool volumeEnabled;  // Default: true
};
```

**Control Flow:**
```
User: "sensors:pitch:off"
    ↓
ControlHandler::executeCommand()
    ↓
theremin->getSensorManager()->setPitchEnabled(false)
    ↓
Later in Theremin::update():
    ↓
if (sensors.isPitchEnabled()) {
    // Apply pitch sensor value
}
// If disabled, frequency unchanged (manual commands persist)
```

**Key Principle:** When a sensor is disabled, Theremin simply doesn't update that audio parameter. Manual `audio:` commands persist until sensor is re-enabled.

### Implementation Steps

#### Step C2.1: Add Enable Flags to SensorManager

**Modify `include/SensorManager.h`** (add to public section):

```cpp
public:
  /**
   * Enable or disable pitch sensor
   * @param enabled True to enable, false to disable
   */
  void setPitchEnabled(bool enabled);

  /**
   * Enable or disable volume sensor
   * @param enabled True to enable, false to disable
   */
  void setVolumeEnabled(bool enabled);

  /**
   * Check if pitch sensor is enabled
   * @return True if enabled
   */
  bool isPitchEnabled() const { return pitchEnabled; }

  /**
   * Check if volume sensor is enabled
   * @return True if enabled
   */
  bool isVolumeEnabled() const { return volumeEnabled; }

private:
  bool pitchEnabled;   // Pitch sensor enable state
  bool volumeEnabled;  // Volume sensor enable state
```

**Modify `src/SensorManager.cpp`** (add to constructor and implement methods):

```cpp
// In constructor, initialize to true (sensors enabled by default)
SensorManager::SensorManager(PerformanceMonitor* perfMon)
    : performanceMonitor(perfMon),
      pitchEnabled(true),
      volumeEnabled(true) {
  // ... existing initialization ...
}

void SensorManager::setPitchEnabled(bool enabled) {
  pitchEnabled = enabled;
  DEBUG_PRINT("[SENSOR] Pitch sensor ");
  DEBUG_PRINTLN(enabled ? "enabled" : "disabled");
}

void SensorManager::setVolumeEnabled(bool enabled) {
  volumeEnabled = enabled;
  DEBUG_PRINT("[SENSOR] Volume sensor ");
  DEBUG_PRINTLN(enabled ? "enabled" : "disabled");
}
```

#### Step C2.2: Add Theremin Getter

**Modify `include/Theremin.h`** (add to public section):

```cpp
public:
  /**
   * Get pointer to SensorManager instance (for control access)
   * @return Pointer to SensorManager
   */
  SensorManager* getSensorManager() { return &sensors; }
```

#### Step C2.3: Update Theremin Logic

**Modify `src/Theremin.cpp`** in `update()` method:

```cpp
void Theremin::update() {
  // Read sensors (always read hardware)
  sensors.updateReadings();

  // Only apply pitch sensor if enabled
  if (sensors.isPitchEnabled()) {
    int pitchDist = sensors.getPitchDistance();
    float freq = mapFloat((float)pitchDist,
                          (float)PITCH_MIN_DISTANCE, (float)PITCH_MAX_DISTANCE,
                          (float)AudioEngine::MIN_FREQUENCY, (float)AudioEngine::MAX_FREQUENCY);
    audio.setFrequency((int)freq);
  }

  // Only apply volume sensor if enabled
  if (sensors.isVolumeEnabled()) {
    int volumeDist = sensors.getVolumeDistance();
    int amplitude = map(volumeDist,
                        VOLUME_FAR_DISTANCE, VOLUME_NEAR_DISTANCE,
                        MAX_AMPLITUDE_PERCENT, MIN_AMPLITUDE_PERCENT);
    amplitude = constrain(amplitude, MIN_AMPLITUDE_PERCENT, MAX_AMPLITUDE_PERCENT);
    audio.setAmplitude(amplitude);
  }

  // ... rest of update() ...
}
```

#### Step C2.4: Refactor ControlHandler Constructor

**Modify `include/ControlHandler.h`**:

```cpp
class ControlHandler {
public:
  /**
   * Constructor
   * @param theremin Pointer to Theremin instance (for full control access)
   */
  ControlHandler(Theremin* theremin);

private:
  Theremin* theremin;  // Full access to sensors and audio
```

**Modify `src/ControlHandler.cpp`** constructor:

```cpp
ControlHandler::ControlHandler(Theremin* thereminPtr)
    : theremin(thereminPtr) {
}
```

**Update all `audio->` calls to `theremin->getAudioEngine()->`** in ControlHandler.cpp.

#### Step C2.5: Add Sensor Commands

**Add to `ControlHandler::executeCommand()`** (after help/status handling):

```cpp
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

// Sensor control aliases (use batch commands)
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
  DEBUG_PRINTLN("\\n========== SENSOR STATUS ==========");
  DEBUG_PRINT("Pitch sensor:  ");
  DEBUG_PRINTLN(theremin->getSensorManager()->isPitchEnabled() ? "ENABLED" : "DISABLED");
  DEBUG_PRINT("Volume sensor: ");
  DEBUG_PRINTLN(theremin->getSensorManager()->isVolumeEnabled() ? "ENABLED" : "DISABLED");
  DEBUG_PRINTLN("===================================\\n");
  return;
}
```

#### Step C2.6: Add Audio Commands

**Add to `ControlHandler::executeCommand()`** (after sensor commands):

```cpp
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
  DEBUG_PRINTLN("\\n========== AUDIO STATUS ==========");
  DEBUG_PRINT("Frequency: ");
  DEBUG_PRINT(theremin->getAudioEngine()->getFrequency());
  DEBUG_PRINTLN(" Hz");
  DEBUG_PRINT("Amplitude: ");
  DEBUG_PRINT(theremin->getAudioEngine()->getAmplitude());
  DEBUG_PRINTLN("%");
  DEBUG_PRINTLN("==================================\\n");
  return;
}
```

#### Step C2.7: Update Help Text

**Modify `printHelp()`** in ControlHandler.cpp:

```cpp
void ControlHandler::printHelp() {
  DEBUG_PRINTLN("\\n========== OSCILLATOR CONTROL COMMANDS ==========");
  // ... existing oscillator commands ...

  DEBUG_PRINTLN("\\nSensor Control:");
  DEBUG_PRINTLN("  sensors:pitch:on     - Enable pitch sensor");
  DEBUG_PRINTLN("  sensors:pitch:off    - Disable pitch sensor");
  DEBUG_PRINTLN("  sensors:volume:on    - Enable volume sensor");
  DEBUG_PRINTLN("  sensors:volume:off   - Disable volume sensor");
  DEBUG_PRINTLN("  sensors:enable       - Enable both sensors (alias)");
  DEBUG_PRINTLN("  sensors:disable      - Disable both sensors (alias)");
  DEBUG_PRINTLN("  sensors:status       - Show sensor enable states");

  DEBUG_PRINTLN("\\nAudio Control:");
  DEBUG_PRINTLN("  audio:freq:440       - Set frequency to 440 Hz");
  DEBUG_PRINTLN("  audio:amp:75         - Set amplitude to 75%");
  DEBUG_PRINTLN("  audio:status         - Show current audio values");

  DEBUG_PRINTLN("\\nNote: When sensors disabled, manual audio: commands persist");
  DEBUG_PRINTLN("      When sensors enabled, they override manual settings");
  DEBUG_PRINTLN("=================================================\\n");
}
```

#### Step C2.8: Update main.cpp

**Modify `src/main.cpp`** constructor call:

```cpp
// Change from:
// ControlHandler controls(theremin.getAudioEngine());

// To:
ControlHandler controls(&theremin);
```

### Use Cases

**1. Test fixed frequency with sensor-controlled volume:**
```
sensors:pitch:off
audio:freq:440
# Now playing A4 (440Hz) at sensor-controlled volume
```

**2. Test different amplitudes at sensor-controlled pitch:**
```
sensors:volume:off
audio:amp:50
# Fixed 50% volume, pitch follows hand position
```

**3. Complete manual control (no sensors):**
```
sensors:disable
audio:freq:523
audio:amp:75
# C5 at 75% volume, fully manual
```

**4. Return to normal theremin operation:**
```
sensors:enable
# Back to full sensor control
```

**5. Combine with oscillator commands:**
```
sensors:pitch:off;audio:freq:440;osc1:sine;osc1:vol:0.8
# Fixed 440Hz sine at 80% volume with sensor-controlled amplitude
```

### Success Criteria for Phase C2

- [ ] Sensor enable flags stored in SensorManager
- [ ] `sensors:pitch:on/off` commands work
- [ ] `sensors:volume:on/off` commands work
- [ ] `sensors:enable/disable` aliases work
- [ ] `audio:freq:X` sets frequency manually
- [ ] `audio:amp:X` sets amplitude manually
- [ ] `sensors:status` shows enable states
- [ ] `audio:status` shows current values
- [ ] Manual audio commands persist when sensors disabled
- [ ] Sensors override manual settings when re-enabled
- [ ] Theremin::update() respects sensor flags
- [ ] Help text includes new commands
- [ ] All Phase C commands still work
- [ ] No audio glitches during transitions

### Command Reference

```bash
# Individual sensor control
sensors:pitch:on         # Enable pitch sensor
sensors:pitch:off        # Disable pitch sensor
sensors:volume:on        # Enable volume sensor
sensors:volume:off       # Disable volume sensor

# Convenience aliases (batch commands)
sensors:enable           # Enable both sensors
sensors:disable          # Disable both sensors

# Manual audio control
audio:freq:440           # Set frequency to 440 Hz
audio:amp:75             # Set amplitude to 75%

# Status
sensors:status           # Show sensor enable states
audio:status             # Show current audio values

# All oscillator commands still work
osc1:sine                # Oscillator commands
osc1:octave:1
help
status
```

### Build Impact Estimate

- **Code:** ~120-150 lines added total
- **Flash:** +1.5-2 KB estimated
- **RAM:** 2 bytes (bool flags in SensorManager)
- Minimal impact, leverages existing infrastructure

---

## Phase D: GPIO Reading (Hardware Integration)
**Goal:** Add support for physical switches and buttons
**Testing:** Hardware switches trigger same commands as serial
**Files Modified:** `include/ControlHandler.h`, `src/ControlHandler.cpp`, `include/PinConfig.h`

### Implementation Steps

#### Step D1: Define GPIO Pins in PinConfig.h

Add to `include/PinConfig.h` in the appropriate section:

```cpp
//=============================================================================
// CONTROL PINS - Temporary Direct GPIO (before MCP23017)
//=============================================================================
// Oscillator 1 Controls
#define PIN_OSC1_WAVE_BIT0    4   // Waveform selection bit 0
#define PIN_OSC1_WAVE_BIT1    5   // Waveform selection bit 1
#define PIN_OSC1_OCT_BIT0     13  // Octave selection bit 0
#define PIN_OSC1_OCT_BIT1     14  // Octave selection bit 1

// Future: Add OSC2 and OSC3 pins as needed
// For now, start with OSC1 only for testing

// Control pin configuration
#define CONTROL_DEBOUNCE_MS   50  // Debounce delay in milliseconds
```

#### Step D2: Add GPIO Reading to ControlHandler

Add to `ControlHandler.h` (private section):

```cpp
private:
  // GPIO state tracking
  struct SwitchState {
    int waveformBits;    // Current waveform switch state
    int octaveBits;      // Current octave switch state
    unsigned long lastChangeTime;  // For debouncing
  };

  SwitchState osc1State;
  // SwitchState osc2State;  // Future
  // SwitchState osc3State;  // Future

  /**
   * Initialize GPIO pins
   */
  void setupGPIO();

  /**
   * Read and process GPIO inputs
   */
  void handleGPIO();

  /**
   * Read 2-bit switch value from GPIO pins
   */
  int readSwitchBits(int pin0, int pin1);

  /**
   * Map 2-bit value to waveform
   */
  Oscillator::Waveform mapToWaveform(int bits);

  /**
   * Map 2-bit value to octave shift
   */
  int mapToOctave(int bits);
```

Add to `ControlHandler.cpp`:

```cpp
#include "PinConfig.h"

void ControlHandler::begin() {
  setupGPIO();

  // ... existing serial command help text ...
}

void ControlHandler::setupGPIO() {
  // Configure oscillator 1 control pins as inputs with pullup
  pinMode(PIN_OSC1_WAVE_BIT0, INPUT_PULLUP);
  pinMode(PIN_OSC1_WAVE_BIT1, INPUT_PULLUP);
  pinMode(PIN_OSC1_OCT_BIT0, INPUT_PULLUP);
  pinMode(PIN_OSC1_OCT_BIT1, INPUT_PULLUP);

  // Initialize state
  osc1State.waveformBits = -1;  // Force initial read
  osc1State.octaveBits = -1;
  osc1State.lastChangeTime = 0;

  DEBUG_PRINTLN("[CTRL] GPIO pins configured");
  DEBUG_PRINTLN("[CTRL] Oscillator 1 switches enabled:");
  DEBUG_PRINT("[CTRL]   Waveform: GPIO ");
  DEBUG_PRINT(PIN_OSC1_WAVE_BIT0);
  DEBUG_PRINT(", ");
  DEBUG_PRINTLN(PIN_OSC1_WAVE_BIT1);
  DEBUG_PRINT("[CTRL]   Octave: GPIO ");
  DEBUG_PRINT(PIN_OSC1_OCT_BIT0);
  DEBUG_PRINT(", ");
  DEBUG_PRINTLN(PIN_OSC1_OCT_BIT1);
}

void ControlHandler::update() {
  handleSerialCommands();
  handleGPIO();
}

void ControlHandler::handleGPIO() {
  unsigned long now = millis();

  // Read oscillator 1 switches
  int waveBits = readSwitchBits(PIN_OSC1_WAVE_BIT0, PIN_OSC1_WAVE_BIT1);
  int octBits = readSwitchBits(PIN_OSC1_OCT_BIT0, PIN_OSC1_OCT_BIT1);

  // Check for waveform change
  if (waveBits != osc1State.waveformBits) {
    if (now - osc1State.lastChangeTime > CONTROL_DEBOUNCE_MS) {
      osc1State.waveformBits = waveBits;
      osc1State.lastChangeTime = now;

      Oscillator::Waveform wf = mapToWaveform(waveBits);
      DEBUG_PRINT("[CTRL] OSC1 waveform switch changed: bits=");
      DEBUG_PRINT(waveBits);
      DEBUG_PRINT(" -> ");
      DEBUG_PRINTLN(getWaveformName(wf));

      audio->setOscillatorWaveform(1, wf);
    }
  }

  // Check for octave change
  if (octBits != osc1State.octaveBits) {
    if (now - osc1State.lastChangeTime > CONTROL_DEBOUNCE_MS) {
      osc1State.octaveBits = octBits;
      osc1State.lastChangeTime = now;

      int octave = mapToOctave(octBits);
      DEBUG_PRINT("[CTRL] OSC1 octave switch changed: bits=");
      DEBUG_PRINT(octBits);
      DEBUG_PRINT(" -> ");
      DEBUG_PRINTLN(octave);

      audio->setOscillatorOctave(1, octave);
    }
  }
}

int ControlHandler::readSwitchBits(int pin0, int pin1) {
  // Read pins (active LOW with pullup)
  int bit0 = digitalRead(pin0) == LOW ? 1 : 0;
  int bit1 = digitalRead(pin1) == LOW ? 1 : 0;

  // Combine into 2-bit value (0-3)
  return (bit1 << 1) | bit0;
}

Oscillator::Waveform ControlHandler::mapToWaveform(int bits) {
  // Map 2-bit switch position to waveform
  // Adjust this mapping based on your physical switch arrangement
  switch (bits) {
    case 0: return Oscillator::OFF;
    case 1: return Oscillator::SINE;
    case 2: return Oscillator::TRIANGLE;
    case 3: return Oscillator::SQUARE;
    default: return Oscillator::SINE;
  }
}

int ControlHandler::mapToOctave(int bits) {
  // Map 2-bit switch position to octave shift
  // Adjust this mapping based on your physical switch arrangement
  switch (bits) {
    case 0: return -1;  // Down one octave
    case 1: return 0;   // Base octave
    case 2: return 1;   // Up one octave
    case 3: return 0;   // Reserved / same as base
    default: return 0;
  }
}
```

#### Step D3: Hardware Testing Setup

**Switch Wiring (for testing):**
```
2-Position Rotary Switch or Toggle Switches:
- Connect each switch pin to GND when active
- Pins have internal pullup, so:
  - Switch OPEN = HIGH = 0
  - Switch CLOSED (to GND) = LOW = 1

Example for waveform switch (4 positions, 2 bits):
  Position 1: Both open    = 00 = OFF
  Position 2: Bit0 closed  = 01 = SINE
  Position 3: Bit1 closed  = 10 = TRIANGLE
  Position 4: Both closed  = 11 = SQUARE
```

**Testing Steps:**
1. Wire switches to GPIO 4, 5, 13, 14
2. Upload firmware
3. Move switches and observe:
   - Serial output shows bit values
   - Audio changes in real-time
   - Same as serial commands from Phase B/C

### Phase D: COMPLETED ✅ (MCP23017 Implementation)

**Date Completed:** November 2, 2025

**Actual Implementation:** Instead of direct GPIO pins, Phase D was implemented using MCP23017 I2C GPIO expander with a dedicated GPIOControls class.

**Files Created:**
- `include/GPIOControls.h` - Dedicated GPIO control class (~95 lines)
- `src/GPIOControls.cpp` - MCP23017 integration (~180 lines)

**Files Modified:**
- `include/PinConfig.h` - Added MCP23017 pin mappings
- `include/ControlHandler.h` - Added GPIOControls member (later refactored)
- `src/ControlHandler.cpp` - Integrated GPIO controls (later refactored)

**Key Implementation Details:**

1. **MCP23017 Integration**
   - I2C address: 0x20
   - 16 GPIO pins for 3 oscillators
   - INPUT_PULLUP configuration (active LOW)
   - Graceful degradation if MCP23017 not found

2. **Pin Mappings** (3-pin waveform + 2-pin octave per oscillator)
   ```cpp
   // OSC1: Waveform pins 6,5,14 | Octave pins 7,15
   // OSC2: Waveform pins 4,11,3 | Octave pins 12,13
   // OSC3: Waveform pins 1,9,0  | Octave pins 10,2
   ```

3. **Switch Reading Logic**
   - 3-pin waveform switch: SINE/TRIANGLE/SQUARE (all HIGH = OFF)
   - 2-pin octave switch: +1/-1 (both LOW = 0)
   - Debouncing: 50ms delay prevents switch bounce
   - State tracking per oscillator

4. **Architecture Pattern**
   - Stack allocation (RAII pattern) - consistent with effects
   - Forward declarations to resolve circular dependencies
   - Clean separation: GPIOControls reads hardware, ControlHandler coordinates

**Build Impact:**
- RAM: 7.4% (24,104 bytes) - +144 bytes for GPIO state tracking
- Flash: 29.3% (383,565 bytes) - ~30KB for GPIO controls + MCP23017 library
- Compilation time: ~8.5 seconds (full rebuild)

**Success Criteria:**
- [x] MCP23017 initializes correctly via I2C
- [x] All 16 GPIO pins configured as INPUT_PULLUP
- [x] Moving waveform switches changes oscillator waveforms
- [x] Moving octave switches changes oscillator octaves
- [x] Debouncing prevents switch bounce glitches
- [x] Serial commands work alongside GPIO controls
- [x] Debug output shows switch state changes
- [x] Audio responds smoothly to switch changes
- [x] System works without MCP23017 (serial fallback)

**Testing Notes:**
- Hardware tested with MCP23017 on breadboard
- All 3 oscillators controllable via physical switches
- "Last wins" behavior: Serial or GPIO, whichever is used last takes control
- No audio glitches during rapid switch changes
- Startup sync issue discovered and fixed in Phase E

**Architecture Decision:**
The MCP23017 approach provides:
- More available pins (16 vs limited ESP32 GPIO)
- I2C reduces wire count
- Easy to add more controls in future
- Isolated from ESP32 critical pins

---

## Phase E: Polish & Integration
**Goal:** Clean up code, add production features, finalize documentation
**Testing:** Complete system test with serial and hardware
**Files Modified:** All control-related files

### Implementation Steps

#### Step E1: Startup Control Reading

Modify `ControlHandler::begin()` to read initial switch positions:

```cpp
void ControlHandler::begin() {
  setupGPIO();

  DEBUG_PRINTLN("[CTRL] Control handler initialized");

  // Read initial switch positions and apply
  DEBUG_PRINTLN("[CTRL] Reading initial control positions...");
  handleGPIO();  // This will set oscillators based on current switch positions

  // ... existing serial command help text ...
}
```

This ensures oscillators match physical switch positions at startup, overriding constructor defaults.

#### Step E2: Add Volume Control (Optional)

If you want physical volume control, add potentiometer reading:

```cpp
// In PinConfig.h
#define PIN_OSC1_VOL_POT      34  // Analog input (GPIO 34-39 are ADC-only)

// In ControlHandler.cpp
void ControlHandler::handleGPIO() {
  // ... existing switch reading ...

  // Read volume potentiometer (0-4095 ADC range)
  int volReading = analogRead(PIN_OSC1_VOL_POT);
  float volume = (float)volReading / 4095.0f;  // Convert to 0.0-1.0

  // Apply with hysteresis to prevent jitter
  static float lastVolume = -1.0;
  if (abs(volume - lastVolume) > 0.02) {  // 2% threshold
    audio->setOscillatorVolume(1, volume);
    lastVolume = volume;
  }
}
```

#### Step E3: Performance Optimization

Add throttling to GPIO reads (they don't need to run every loop):

```cpp
void ControlHandler::update() {
  handleSerialCommands();

  // Throttle GPIO reading to every 10ms (plenty fast for human input)
  static unsigned long lastGPIOCheck = 0;
  unsigned long now = millis();
  if (now - lastGPIOCheck >= 10) {
    handleGPIO();
    lastGPIOCheck = now;
  }
}
```

#### Step E4: Error Recovery

Add safeguards for invalid GPIO readings:

```cpp
Oscillator::Waveform ControlHandler::mapToWaveform(int bits) {
  if (bits < 0 || bits > 3) {
    DEBUG_PRINT("[CTRL] WARNING: Invalid waveform bits: ");
    DEBUG_PRINTLN(bits);
    return Oscillator::SINE;  // Safe default
  }

  // ... existing mapping ...
}
```

#### Step E5: Documentation

Update comments and add header documentation:

```cpp
/*
 * ControlHandler.cpp
 *
 * Manages oscillator control from multiple input sources:
 * - Serial commands for testing and debugging
 * - GPIO switches for performance control
 *
 * All inputs are debounced and validated before being sent to AudioEngine
 * via thread-safe API calls protected by mutex.
 *
 * Command Format:
 *   oscN:waveform       - Set waveform (off, square, sine, triangle, saw)
 *   oscN:octave:shift   - Set octave shift (-1, 0, 1)
 *   oscN:vol:level      - Set volume (0.0-1.0)
 *
 * GPIO Format:
 *   2-bit switches for waveform and octave selection
 *   Analog potentiometer for volume (optional)
 *   Active-LOW logic with internal pullups
 */
```

### Success Criteria for Phase E

- [ ] Oscillators match switch positions at startup
- [ ] Volume control works smoothly (if implemented)
- [ ] GPIO reading is throttled (efficient)
- [ ] Invalid inputs handled gracefully
- [ ] Code is well-documented
- [ ] All previous phase features still work
- [ ] System is ready for production use

---

## Testing Checklist

### After Each Phase

- [ ] Code compiles without warnings
- [ ] No RAM/Flash overflow
- [ ] Audio continues playing smoothly
- [ ] No audio glitches during parameter changes
- [ ] Serial debug output is clear
- [ ] Previous features still work

### Complete System Test

1. **Power-On Test:**
   - [ ] System boots successfully
   - [ ] Audio starts with correct initial settings
   - [ ] Controls match physical switch positions

2. **Serial Command Test:**
   - [ ] `help` shows all commands
   - [ ] `status` shows current state
   - [ ] Can change waveform via serial
   - [ ] Can change octave via serial
   - [ ] Can change volume via serial

3. **GPIO Control Test:**
   - [ ] Moving switches changes audio in real-time
   - [ ] No switch bounce artifacts
   - [ ] Debouncing works correctly
   - [ ] Multiple rapid changes handled smoothly

4. **Integration Test:**
   - [ ] Serial and GPIO can both control oscillators
   - [ ] Theremin sensors still work normally
   - [ ] OTA still works
   - [ ] Performance monitoring still works
   - [ ] No crashes or hangs

---

## Future Enhancements

After completing all phases, consider:

1. **Preset System:** Save/load oscillator configurations to EEPROM
2. **MIDI Control:** Accept MIDI CC messages for parameter changes
3. **Display Integration:** Show current settings on OLED
4. **MCP23017 Migration:** Move GPIO to I2C expander for more controls
5. **Effects Controls:** Add switches for delay/chorus/reverb
6. **LED Feedback:** Visual indication of current settings

---

## Troubleshooting

### Audio Glitches When Changing Parameters
- Check mutex is being used correctly
- Verify paramMutex timeout isn't too short
- Ensure changes happen between audio buffers, not during

### Serial Commands Not Working
- Check Serial baud rate (115200)
- Verify command format (case-insensitive)
- Look for error messages in Serial output

### GPIO Not Responding
- Verify pin numbers in PinConfig.h
- Check physical wiring (GND connections)
- Test pins with simple digitalWrite/digitalRead
- Check debounce timing

### Oscillators Not Changing
- Add debug output in AudioEngine set methods
- Verify mutex is not deadlocking
- Check oscillator number (1-3 range)
- Ensure AudioEngine pointer is valid

---

## Notes

- Delete this file after implementation is complete
- Document final implementation in `docs/improvements/`
- Update memory bank with new architecture patterns
- Add control system to `docs/architecture/ARCHITECTURE.md`

**Implementation Date:** October 29, 2025
**Target Completion:** Incrementally, one phase at a time
