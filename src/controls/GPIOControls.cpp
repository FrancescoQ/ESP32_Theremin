/*
 * GPIOControls.cpp
 *
 * Implementation of physical control handling via MCP23017.
 */

#include "controls/GPIOControls.h"
#include "system/Theremin.h"
#include "system/PinConfig.h"
#include "system/Debug.h"
#include "system/NotificationManager.h"

GPIOControls::GPIOControls(Theremin* thereminPtr, DisplayManager* displayMgr)
    : theremin(thereminPtr), initialized(false), controlsEnabled(true), firstUpdate(true),
      displayManager(displayMgr),
      notificationManager(nullptr),
      buttonState(IDLE), buttonPressTime(0), modifierActive(false), modifierWasActive(false),
      shortPressFlag(false),
      firstPressReleaseTime(0), waitingForSecondClick(false), doubleClickFlag(false),
      snapshotSmoothingPreset(0), snapshotFreqRangePreset(0), snapshotMixPreset(0),
      snapshotReverbPreset(Oscillator::OFF), snapshotDelayPreset(Oscillator::OFF),
      snapshotChorusPreset(Oscillator::OFF),
      snapshotOsc1Waveform(Oscillator::OFF), snapshotOsc1Octave(0),
      snapshotOsc2Waveform(Oscillator::OFF), snapshotOsc2Octave(0),
      snapshotOsc3Waveform(Oscillator::OFF), snapshotOsc3Octave(0) {

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

  // Register modifier button indicator overlay
  if (displayManager) {
    displayManager->registerOverlay([this](Adafruit_SSD1306& oled) {
      if (this->modifierActive) {
        // Draw filled circle in top-right area (before page indicator)
        oled.fillCircle(DisplayManager::SCREEN_WIDTH - 30, 3, 3, SSD1306_WHITE);
      }
    });
  }
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

  // Assign here instead the construction or initialization list, to be sure
  // Theremin is completely initialized with display and everything is needed.
  notificationManager = theremin->getNotificationManager();

  DEBUG_PRINTLN("[GPIO] MCP23017 initialized");
  DEBUG_PRINTLN("[GPIO] Oscillator switches configured:");
  DEBUG_PRINTLN("[GPIO]   OSC1: Waveform pins 6,5,14 | Octave pins 7,15");
  DEBUG_PRINTLN("[GPIO]   OSC2: Waveform pins 4,11,3 | Octave pins 12,13");
  DEBUG_PRINTLN("[GPIO]   OSC3: Waveform pins 1,9,0  | Octave pins 10,2");

  // Pin for jack detection: LOW when jack inserted.
  pinMode(PIN_OUTPUT_JACK, INPUT_PULLUP);

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

  // Always update button state first
  updateButton();

  // Check for button actions (page navigation)
  if (displayManager) {
    if (wasDoubleClicked()) {
      // Double-click = previous page
      displayManager->previousPage();
    } else if (wasShortPressed()) {
      // Single click = next page
      displayManager->nextPage();
    }
  }

  // Detect mode transitions and take snapshots
  if (modifierActive && !modifierWasActive) {
    // Just entered Mode 2 - snapshot all secondary control positions
    DEBUG_PRINTLN("[GPIO] Entering Mode 2 - snapshotting secondary controls");
    snapshotSmoothingPreset = readOctave(PIN_OSC1_OCT_UP, PIN_OSC1_OCT_DOWN);
    snapshotFreqRangePreset = readOctave(PIN_OSC2_OCT_UP, PIN_OSC2_OCT_DOWN);
    snapshotMixPreset = readOctave(PIN_OSC3_OCT_UP, PIN_OSC3_OCT_DOWN);
    snapshotReverbPreset = readWaveform(PIN_OSC1_WAVE_A, PIN_OSC1_WAVE_B, PIN_OSC1_WAVE_C);
    snapshotDelayPreset = readWaveform(PIN_OSC2_WAVE_A, PIN_OSC2_WAVE_B, PIN_OSC2_WAVE_C);
    snapshotChorusPreset = readWaveform(PIN_OSC3_WAVE_A, PIN_OSC3_WAVE_B, PIN_OS3_WAVE_C);
  } else if (!modifierActive && modifierWasActive) {
    // Just exited Mode 2 - snapshot all primary control positions
    DEBUG_PRINTLN("[GPIO] Exiting Mode 2 - snapshotting primary controls");
    snapshotOsc1Waveform = readWaveform(PIN_OSC1_WAVE_A, PIN_OSC1_WAVE_B, PIN_OSC1_WAVE_C);
    snapshotOsc1Octave = readOctave(PIN_OSC1_OCT_UP, PIN_OSC1_OCT_DOWN);
    snapshotOsc2Waveform = readWaveform(PIN_OSC2_WAVE_A, PIN_OSC2_WAVE_B, PIN_OSC2_WAVE_C);
    snapshotOsc2Octave = readOctave(PIN_OSC2_OCT_UP, PIN_OSC2_OCT_DOWN);
    snapshotOsc3Waveform = readWaveform(PIN_OSC3_WAVE_A, PIN_OSC3_WAVE_B, PIN_OS3_WAVE_C);
    snapshotOsc3Octave = readOctave(PIN_OSC3_OCT_UP, PIN_OSC3_OCT_DOWN);
  }

  // Update tracking variable for next iteration
  modifierWasActive = modifierActive;

  // Branch based on modifier button state
  if (isModifierActive()) {
    // Modifier held: switches control secondary functions (effects, smoothing, etc.)
    updateSecondaryControls();
  } else {
    // Normal mode: switches control oscillators
    updateOscillator(1, osc1State,
                     PIN_OSC1_WAVE_A, PIN_OSC1_WAVE_B, PIN_OSC1_WAVE_C,
                     PIN_OSC1_OCT_UP, PIN_OSC1_OCT_DOWN);

    updateOscillator(2, osc2State,
                     PIN_OSC2_WAVE_A, PIN_OSC2_WAVE_B, PIN_OSC2_WAVE_C,
                     PIN_OSC2_OCT_UP, PIN_OSC2_OCT_DOWN);

    updateOscillator(3, osc3State,
                     PIN_OSC3_WAVE_A, PIN_OSC3_WAVE_B, PIN_OS3_WAVE_C,
                     PIN_OSC3_OCT_UP, PIN_OSC3_OCT_DOWN);
  }

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

  // Get snapshot pointers for this oscillator
  Oscillator::Waveform* snapshotWaveform;
  int8_t* snapshotOctave;

  switch (oscNum) {
    case 1:
      snapshotWaveform = &snapshotOsc1Waveform;
      snapshotOctave = &snapshotOsc1Octave;
      break;
    case 2:
      snapshotWaveform = &snapshotOsc2Waveform;
      snapshotOctave = &snapshotOsc2Octave;
      break;
    case 3:
      snapshotWaveform = &snapshotOsc3Waveform;
      snapshotOctave = &snapshotOsc3Octave;
      break;
    default:
      return; // Invalid oscillator
  }

  // Check for waveform change FROM SNAPSHOT (force update on first call)
  if (firstUpdate || currentWaveform != *snapshotWaveform) {
    if (firstUpdate || (now - state.lastChangeTime > DEBOUNCE_MS)) {
      // Update snapshot
      *snapshotWaveform = currentWaveform;
      state.waveform = currentWaveform;
      state.lastChangeTime = now;

      theremin->getAudioEngine()->setOscillatorWaveform(oscNum, currentWaveform);

      // Convert waveform to short name
      const char* waveformName;
      switch (currentWaveform) {
        case Oscillator::OFF:
          waveformName = "OFF";
          break;
        case Oscillator::SINE:
          waveformName = "SIN";
          break;
        case Oscillator::SQUARE:
          waveformName = "SQR";
          break;
        case Oscillator::TRIANGLE:
          waveformName = "TRI";
          break;
        case Oscillator::SAW:
          waveformName = "SAW";
          break;
        default:
          waveformName = "???";
          break;
      }

      // Format: "OSC1:SIN"
      String message = "OSC" + String(oscNum) + ":" + String(waveformName);
      showNotification(message);

      DEBUG_PRINT("[GPIO] OSC");
      DEBUG_PRINT(oscNum);
      DEBUG_PRINT(" waveform: ");
      DEBUG_PRINTLN(getWaveformName(currentWaveform));
    }
  }

  // Check for octave change FROM SNAPSHOT
  if (firstUpdate || currentOctave != *snapshotOctave) {
    if (firstUpdate || (now - state.lastChangeTime > DEBOUNCE_MS)) {
      // Update snapshot
      *snapshotOctave = currentOctave;
      state.octave = currentOctave;
      state.lastChangeTime = now;

      theremin->getAudioEngine()->setOscillatorOctave(oscNum, currentOctave);

      // Convert octave to short name
      const char* octaveString;
      switch (currentOctave) {
        case 0:
          octaveString = "0";
          break;
        case -1:
          octaveString = "-1";
          break;
        case +1:
          octaveString = "+1";
          break;
        default:
          octaveString = "???";
          break;
      }

      // Format: "OSC1:+1"
      String message = "OSC" + String(oscNum) + ":" + String(octaveString);
      showNotification(message);

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

void GPIOControls::updateButton() {
  // Read button state (active LOW with pullup on MCP23017 pin 8)
  bool buttonPressed = (mcp.digitalRead(PIN_MULTI_BUTTON) == LOW);
  unsigned long now = millis();

  // Check for double-click timeout while waiting for second click
  if (waitingForSecondClick && (now - firstPressReleaseTime > DOUBLE_CLICK_WINDOW_MS)) {
    // Timeout expired - convert to single click
    waitingForSecondClick = false;
    shortPressFlag = true;
    DEBUG_PRINTLN("[GPIO] Double-click timeout - processing as single click");
  }

  switch (buttonState) {
    case IDLE:
      if (buttonPressed) {
        // Button just pressed
        buttonState = PRESSED;
        buttonPressTime = now;
        DEBUG_PRINTLN("[GPIO] Button pressed");
      }
      break;

    case PRESSED:
      if (!buttonPressed) {
        // Button released - check if it was held long enough for debounce
        if (now - buttonPressTime > DEBOUNCE_MS) {
          // Valid short press detected

          if (waitingForSecondClick) {
            // Second click within window - it's a double-click!
            doubleClickFlag = true;
            waitingForSecondClick = false;
            buttonState = IDLE;
            DEBUG_PRINTLN("[GPIO] Double-click detected");
          } else {
            // First click - start waiting for second click
            waitingForSecondClick = true;
            firstPressReleaseTime = now;
            buttonState = IDLE;
            DEBUG_PRINTLN("[GPIO] First click - waiting for second");
          }
        } else {
          // Released too quickly - ignore (bounce)
          buttonState = IDLE;
        }
      } else if (now - buttonPressTime >= LONG_PRESS_THRESHOLD_MS) {
        // Long press threshold reached - entering modifier mode
        // Cancel any waiting double-click
        waitingForSecondClick = false;
        buttonState = LONG_PRESS_ACTIVE;
        modifierActive = true;
        DEBUG_PRINTLN("[GPIO] Long press active - modifier mode ON");
      }
      // Otherwise stay in PRESSED state, waiting
      break;

    case LONG_PRESS_ACTIVE:
      if (!buttonPressed) {
        // Long press released - exiting modifier mode
        buttonState = IDLE;
        modifierActive = false;
        DEBUG_PRINTLN("[GPIO] Long press released - modifier mode OFF");
      }
      else if (now - buttonPressTime >= VERY_LONG_PRESS_THRESHOLD_MS) {
        // VERY long press - trigger system reboot
        performSystemReboot();
        // Note: ESP.restart() never returns, so no state change needed
      }
      // While held, stay in this state
      break;

    case RELEASED:
      // This state is only briefly used, should transition to IDLE
      buttonState = IDLE;
      break;
  }
}

bool GPIOControls::wasShortPressed() {
  if (shortPressFlag) {
    shortPressFlag = false;  // Consume the flag
    return true;
  }
  return false;
}

bool GPIOControls::wasDoubleClicked() {
  if (doubleClickFlag) {
    doubleClickFlag = false;  // Consume the flag
    return true;
  }
  return false;
}

void GPIOControls::updateSecondaryControls() {
  osc1PitchSecondaryControl();
  osc2PitchSecondaryControl();
  osc3PitchSecondaryControl();
  osc1WaveformSecondaryControl();
  osc2WaveformSecondaryControl();
  osc3WaveformSecondaryControl();
}

// OSC1 Pitch secondary control: Smoothing Presets
void GPIOControls::osc1PitchSecondaryControl() {
  unsigned long now = millis();

  // OSC1 Octave Switch → Smoothing Preset Control
  // Read OSC1 octave switch position (-1, 0, +1)
  int8_t octaveValue = readOctave(PIN_OSC1_OCT_UP, PIN_OSC1_OCT_DOWN);

  // Track last state for debouncing (static local variables)
  static unsigned long lastSmoothingChangeTime = 0;

  // Check if smoothing preset changed FROM SNAPSHOT (not from last applied)
  if (octaveValue != snapshotSmoothingPreset) {
    if (now - lastSmoothingChangeTime > DEBOUNCE_MS) {
      // Update snapshot to new position
      snapshotSmoothingPreset = octaveValue;
      lastSmoothingChangeTime = now;

      // Map octave position to smoothing preset
      // -1 (down) → SMOOTH_NONE (0)
      //  0 (center) → SMOOTH_NORMAL (1)
      // +1 (up) → SMOOTH_EXTRA (2)
      Theremin::SmoothingPreset preset = static_cast<Theremin::SmoothingPreset>(octaveValue + 1);

      // Apply to both pitch and volume
      theremin->setPitchSmoothingPreset(preset);
      theremin->setVolumeSmoothingPreset(preset);

      // Debug output
      const char* presetName;
      switch (preset) {
        case Theremin::SMOOTH_NONE:
          presetName = "NONE (instant response)";
          showNotification("SMT:OFF");
          break;
        case Theremin::SMOOTH_NORMAL:
          presetName = "NORMAL (balanced)";
          showNotification("SMT:NRM");
          break;
        case Theremin::SMOOTH_EXTRA:
          presetName = "EXTRA (maximum smoothness)";
          showNotification("SMT:MAX");
          break;
        default:
          presetName = "UNKNOWN";
          showNotification("SMT:???");
          break;
      }

      DEBUG_PRINT("[GPIO] Smoothing preset changed: ");
      DEBUG_PRINTLN(presetName);
    }
  }
}

// OSC2 Pitch secondary control: Frequency Range Presets
void GPIOControls::osc2PitchSecondaryControl() {
  unsigned long now = millis();

  // OSC2 Octave Switch → Frequency Range Presets
  int8_t octaveValue = readOctave(PIN_OSC2_OCT_UP, PIN_OSC2_OCT_DOWN);

  // Track last state for debouncing
  static unsigned long lastRangeChangeTime = 0;

  if (octaveValue != snapshotFreqRangePreset) {
    if (now - lastRangeChangeTime > DEBOUNCE_MS) {
      snapshotFreqRangePreset = octaveValue;
      lastRangeChangeTime = now;

      // Map octave position to frequency range preset
      // -1 (down) → NARROW (1 octave)
      //  0 (center) → NORMAL (2 octaves)
      // +1 (up) → WIDE (3 octaves)
      Theremin::FrequencyRangePreset preset = static_cast<Theremin::FrequencyRangePreset>(octaveValue + 1);

      theremin->setFrequencyRangePreset(preset);

      // Debug output
      const char* presetName;
      switch (preset) {
        case Theremin::RANGE_NARROW:
          presetName = "NARROW (1 octave, 250mm)";
          showNotification("RNG:NRW");
          break;
        case Theremin::RANGE_NORMAL:
          presetName = "NORMAL (2 octaves, 350mm)";
          showNotification("RNG:NRM");
          break;
        case Theremin::RANGE_WIDE:
          presetName = "WIDE (3 octaves, 450mm)";
          showNotification("RNG:EXT");
          break;
        default:
          presetName = "UNKNOWN";
          showNotification("RNG:???");
          break;
      }

      DEBUG_PRINT("[GPIO] Frequency range changed: ");
      DEBUG_PRINTLN(presetName);
    }
  }
}

// OSC3 Pitch secondary control: Oscillator Mix Presets
void GPIOControls::osc3PitchSecondaryControl() {
  unsigned long now = millis();

  // OSC3 Octave Switch → Oscillator Mix Presets
  int8_t octaveValue = readOctave(PIN_OSC3_OCT_UP, PIN_OSC3_OCT_DOWN);

  // Track last state for debouncing
  static unsigned long lastMixChangeTime = 0;

  if (octaveValue != snapshotMixPreset) {
    if (now - lastMixChangeTime > DEBOUNCE_MS) {
      snapshotMixPreset = octaveValue;
      lastMixChangeTime = now;

      // Map octave position to oscillator mix preset
      const char* presetName;
      switch (octaveValue) {
        case -1:  // Down position - Equal mix (thickest)
          theremin->getAudioEngine()->setOscillatorVolume(1, 1.0f);
          theremin->getAudioEngine()->setOscillatorVolume(2, 1.0f);
          theremin->getAudioEngine()->setOscillatorVolume(3, 1.0f);
          presetName = "EQUAL (1.0, 1.0, 1.0)";
          showNotification("MIX:EQ");
          break;

        case 0:   // Center - Primary focus (balanced)
          theremin->getAudioEngine()->setOscillatorVolume(1, 1.0f);
          theremin->getAudioEngine()->setOscillatorVolume(2, 0.7f);
          theremin->getAudioEngine()->setOscillatorVolume(3, 0.5f);
          presetName = "PRIMARY (1.0, 0.7, 0.5)";
          showNotification("MIX:BAL");
          break;

        case +1:  // Up position - Gradient (focused)
          theremin->getAudioEngine()->setOscillatorVolume(1, 1.0f);
          theremin->getAudioEngine()->setOscillatorVolume(2, 0.5f);
          theremin->getAudioEngine()->setOscillatorVolume(3, 0.3f);
          presetName = "GRADIENT (1.0, 0.6, 0.3)";
          showNotification("MIX:WID");
          break;

        default:
          presetName = "UNKNOWN";
          showNotification("MIX:???");
          break;
      }

      DEBUG_PRINT("[GPIO] Oscillator mix: ");
      DEBUG_PRINTLN(presetName);
    }
  }
}

// Oscillator 1 waveform secondary control: Reverb effect presets
void GPIOControls::osc1WaveformSecondaryControl() {
  unsigned long now = millis();
  Oscillator::Waveform waveformValue = readWaveform(PIN_OSC1_WAVE_A, PIN_OSC1_WAVE_B, PIN_OSC1_WAVE_C);

  // Track last state for debouncing (static local variables)
  static unsigned long lastOsc1ChangeTime = 0;

  // Check if preset changed FROM SNAPSHOT
  if (waveformValue != snapshotReverbPreset) {
    if (now - lastOsc1ChangeTime > DEBOUNCE_MS) {
      snapshotReverbPreset = waveformValue;
      lastOsc1ChangeTime = now;

      ReverbEffect::Preset preset;
      const char* presetName;
      // We use a switch to map since enums are not ordered like we want for
      // the presets, the physical switch shows sine as first, than square and
      // than triangle. Saw is not used in the physiical switch.
      switch (waveformValue) {
        case Oscillator::Waveform::OFF:
          preset = ReverbEffect::REVERB_OFF;
          presetName = "OFF";
          showNotification("REV:OFF");
          break;
        case Oscillator::Waveform::SINE:
          preset = ReverbEffect::REVERB_SMALL;
          presetName = "SMALL";
          showNotification("REV:SML");
          break;
        case Oscillator::Waveform::SQUARE:
          preset = ReverbEffect::REVERB_NORMAL;
          presetName = "NORMAL";
          showNotification("REV:NRM");
          break;
        case Oscillator::Waveform::TRIANGLE:
          preset = ReverbEffect::REVERB_MAX;
          presetName = "MAX";
          showNotification("REV:MAX");
          break;
        default:
          preset = ReverbEffect::REVERB_OFF;
          presetName = "UNKNOWN";
          showNotification("REV:???");
          break;  // Safe fallback
      }

      // Apply preset
      theremin->getAudioEngine()->getEffectsChain()->getReverb()->setPreset(preset);

      DEBUG_PRINT("[GPIO] Reverb preset changed: ");
      DEBUG_PRINTLN(presetName);
    }
  }
}

// Oscillator 2 waveform secondary control: Delay effect presets
void GPIOControls::osc2WaveformSecondaryControl() {
  unsigned long now = millis();
  Oscillator::Waveform waveformValue = readWaveform(PIN_OSC2_WAVE_A, PIN_OSC2_WAVE_B, PIN_OSC2_WAVE_C);

  // Track last state for debouncing (static local variables)
  static unsigned long lastOsc2ChangeTime = 0;

  // Check if preset changed FROM SNAPSHOT
  if (waveformValue != snapshotDelayPreset) {
    if (now - lastOsc2ChangeTime > DEBOUNCE_MS) {
      snapshotDelayPreset = waveformValue;
      lastOsc2ChangeTime = now;

      DelayEffect::Preset preset;
      const char* presetName;
      // We use a switch to map since enums are not ordered like we want for
      // the presets, the physical switch shows sine as first, than square and
      // than triangle. Saw is not used in the physiical switch.
      switch (waveformValue) {
        case Oscillator::Waveform::OFF:
          preset = DelayEffect::DELAY_OFF;
          presetName = "OFF";
          showNotification("DLY:OFF");
          break;
        case Oscillator::Waveform::SINE:
          preset = DelayEffect::DELAY_SHORT;
          presetName = "SMALL";
          showNotification("DLY:SML");
          break;
        case Oscillator::Waveform::SQUARE:
          preset = DelayEffect::DELAY_MEDIUM;
          presetName = "NORMAL";
          showNotification("DLY:NRM");
          break;
        case Oscillator::Waveform::TRIANGLE:
          preset = DelayEffect::DELAY_LONG;
          presetName = "MAX";
          showNotification("DLY:MAX");
          break;
        default:
          preset = DelayEffect::DELAY_OFF;
          presetName = "UNKNOWN";
          showNotification("DLY:???");
          break;  // Safe fallback
      }

      // Apply preset
      theremin->getAudioEngine()->getEffectsChain()->getDelay()->setPreset(preset);

      DEBUG_PRINT("[GPIO] Delay preset changed: ");
      DEBUG_PRINTLN(presetName);
    }
  }
}

// Oscillator 3 waveform secondary control: Chorus effect presets
void GPIOControls::osc3WaveformSecondaryControl() {
  unsigned long now = millis();
  Oscillator::Waveform waveformValue = readWaveform(PIN_OSC3_WAVE_A, PIN_OSC3_WAVE_B, PIN_OS3_WAVE_C);

  // Track last state for debouncing (static local variables)
  static unsigned long lastOsc3ChangeTime = 0;

  // Check if preset changed FROM SNAPSHOT
  if (waveformValue != snapshotChorusPreset) {
    if (now - lastOsc3ChangeTime > DEBOUNCE_MS) {
      snapshotChorusPreset = waveformValue;
      lastOsc3ChangeTime = now;

      ChorusEffect::Preset preset;
      const char* presetName;
      // We use a switch to map since enums are not ordered like we want for
      // the presets, the physical switch shows sine as first, than square and
      // than triangle. Saw is not used in the physiical switch.
      switch (waveformValue) {
        case Oscillator::Waveform::OFF:
          preset = ChorusEffect::CHORUS_OFF;
          presetName = "OFF";
          showNotification("CHR:OFF");
          break;
        case Oscillator::Waveform::SINE:
          preset = ChorusEffect::CHORUS_MIN;
          presetName = "SMALL";
          showNotification("CHR:SML");
          break;
        case Oscillator::Waveform::SQUARE:
          preset = ChorusEffect::CHORUS_MEDIUM;
          presetName = "NORMAL";
          showNotification("CHR:NRM");
          break;
        case Oscillator::Waveform::TRIANGLE:
          preset = ChorusEffect::CHORUS_MAX;
          presetName = "MAX";
          showNotification("CHR:MAX");
          break;
        default:
          preset = ChorusEffect::CHORUS_OFF;
          presetName = "UNKNOWN";
          showNotification("CHR:???");
          break;  // Safe fallback
      }

      // Apply preset
      theremin->getAudioEngine()->getEffectsChain()->getChorus()->setPreset(preset);

      DEBUG_PRINT("[GPIO] Chorus preset changed: ");
      DEBUG_PRINTLN(presetName);
    }
  }
}

void GPIOControls::showNotification(const String& message, uint16_t durationMs) {
  if (notificationManager) {
    notificationManager->show(message, durationMs);
  }
}

void GPIOControls::performSystemReboot() {
  DEBUG_PRINTLN("[GPIO] VERY LONG PRESS DETECTED - REBOOTING SYSTEM...");

  // Show notification if available
  showNotification("REBOOTING...", 2000);

  // Small delay to let notification show
  delay(2000);

  // Restart the ESP32
  ESP.restart();

  // Code never reaches here
}
