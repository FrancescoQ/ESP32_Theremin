# System Patterns - ESP32 Theremin

## Project File Organization (November 2, 2025)

The project uses a hierarchical folder structure organized by subsystem for better maintainability:

```
include/
├── audio/
│   ├── AudioConstants.h      # Audio configuration constants
│   ├── AudioEngine.h          # Main audio synthesis engine
│   ├── Oscillator.h           # Waveform generator
│   └── effects/
│       ├── EffectsChain.h     # Effects coordinator
│       ├── DelayEffect.h      # Digital delay
│       ├── ChorusEffect.h     # Chorus/vibrato
│       └── ReverbEffect.h     # Reverb (Freeverb)
├── controls/
│   ├── SensorManager.h        # VL53L0X sensor interface
│   ├── GPIOControls.h         # Physical switch controls (MCP23017)
│   ├── SerialControls.h       # Serial command interface
│   └── GPIOMonitor.h          # I2C device monitor
└── system/
    ├── Theremin.h             # Main coordinator class
    ├── OTAManager.h           # Over-the-air updates
    ├── PerformanceMonitor.h   # CPU/RAM monitoring
    ├── Debug.h                # Debug macros
    └── PinConfig.h            # Hardware pin definitions

src/
├── audio/
│   ├── AudioEngine.cpp
│   ├── Oscillator.cpp
│   └── effects/
│       ├── EffectsChain.cpp
│       ├── DelayEffect.cpp
│       ├── ChorusEffect.cpp
│       └── ReverbEffect.cpp
├── controls/
│   ├── SensorManager.cpp
│   ├── GPIOControls.cpp
│   ├── SerialControls.cpp
│   └── GPIOMonitor.cpp
├── system/
│   ├── Theremin.cpp
│   ├── OTAManager.cpp
│   └── PerformanceMonitor.cpp
└── main.cpp                  # Application entry point
```

**Include Path Convention:**
- All includes use subsystem-relative paths from the `include/` root
- Example: `#include "audio/AudioEngine.h"`
- Example: `#include "audio/effects/DelayEffect.h"`
- Example: `#include "controls/SensorManager.h"`
- Example: `#include "system/Theremin.h"`

**Benefits:**
- Clear separation by responsibility (audio/controls/system)
- Effects get dedicated subdirectory (already 4 files)
- Easy to locate related components
- Matches conceptual architecture
- Future-proof for expansion

## System Architecture

### High-Level Architecture - Modular Design
```
                ┌─────────────────────────────────────┐
                │   ESP32 Main Loop (setup/loop)      │
                │      Modular Class Structure         │
                └─────────────────────────────────────┘
                              │
                    ┌─────────┴─────────┐
                    │  Core Components  │
                    │  (always enabled) │
                    └─────────┬─────────┘
                              │
            ┌─────────────────┴─────────────────┐
            │                                   │
       ┌────▼────┐                        ┌─────▼─────┐
       │SIMULATION│                       │ HARDWARE  │
       │  MODE    │                       │   MODE    │
       └────┬─────┘                       └─────┬─────┘
            │                                   │
   ┌────────▼────────┐              ┌───────────▼──────────┐
   │ Potentiometers  │              │   VL53L0X Sensors    │
   │ GPIO34/35 (ADC) │              │   I2C @ 0x29/0x30    │
   │ SensorManager   │              │   SensorManager      │
   └────────┬────────┘              └───────────┬──────────┘
            │                                   │
            └──────────┬────────────────────────┘
                       │
          ┌────────────▼────────────┐
          │      AudioEngine        │
          │   Audio Synthesis       │
          └────────────┬────────────┘
                       │
                  ┌────▼────┐
                  │   PWM   │
                  │ GPIO25  │
                  └────┬────┘
                       │
                  ┌────▼────┐
                  │ Buzzer  │
                  └─────────┘

   ┌──────────────────────────────────────┐
   │   Optional: OTAManager               │
   │   (#ifdef ENABLE_OTA)                │
   │                                      │
   │   ┌────────────┐                     │
   │   │ WiFi AP    │                     │
   │   │ 192.168.4.1│                     │
   │   └─────┬──────┘                     │
   │         │                            │
   │   ┌─────▼──────┐                     │
   │   │ ElegantOTA │                     │
   │   │ Web Server │                     │
   │   └────────────┘                     │
   └──────────────────────────────────────┘
```

### Simulation Architecture (Wokwi)
```
┌────────────────────────────────────────────┐
│         ESP32 Virtual Circuit              │
│  ┌──────────┐              ┌──────────┐   │
│  │  Pitch   │              │  Volume  │   │
│  │   POT    │              │   POT    │   │
│  │ (Left)   │              │ (Right)  │   │
│  └────┬─────┘              └─────┬────┘   │
│       │                          │         │
│    GPIO34                     GPIO35       │
│       │                          │         │
│  ┌────▼──────────────────────────▼────┐   │
│  │      ESP32 (ADC1 channels)        │   │
│  │  12-bit ADC (0-4095) → mm        │   │
│  └────────────────┬──────────────────┘   │
│                   │GPIO25                 │
│              ┌────▼────┐                  │
│              │ 220Ω R  │                  │
│              └────┬────┘                  │
│              ┌────▼────┐                  │
│              │ Buzzer  │                  │
│              └─────────┘                  │
└────────────────────────────────────────────┘
```

### Hardware Architecture (Physical Theremin)
```
┌─────────────────┐         ┌──────────────┐
│   I2C Bus       │         │  PWM Channel │
│  (GPIO 21/22)   │         │  (GPIO 25)   │
└─────────────────┘         └──────────────┘
         │                           │
         ▼                           ▼
┌─────────────────────┐     ┌──────────────┐
│  VL53L0X Sensor #1  │     │    Passive   │
│  (Pitch @ 0x30)     │     │    Buzzer    │
│                     │     └──────────────┘
│  VL53L0X Sensor #2  │
│  (Volume @ 0x29)    │
└─────────────────────┘
```

### Component Responsibilities

**Main Loop:**
- Orchestrates sensor reading cycle
- Maps sensor data to audio parameters
- Manages timing and latency
- Handles error conditions

**Sensor Manager:**
- I2C communication with both VL53L0X sensors
- Address management (0x29 and 0x30)
- Reading smoothing/filtering
- Range validation and clamping

**Audio Output Manager:**
- PWM tone generation via ledcWriteTone()
- Volume control via ledcWrite()
- Frequency bounds enforcement
- Duty cycle calculation

## Key Technical Decisions

### Decision: VL53L0X Time-of-Flight Sensors
**Rationale:**
- Laser-based ToF provides high precision (±3%)
- No mutual interference between sensors
- Fast reading rate (<30ms)
- Wide usable range (50-500mm)
- Well-supported I2C library available

**Alternatives Considered:**
- HC-SR04 ultrasonic: Too slow, potential interference
- IR sensors: Too imprecise, affected by ambient light
- Capacitive sensing: Complex, would need custom antennas

### Decision: PWM Audio via Passive Buzzer
**Rationale:**
- Simplest implementation for educational project
- No additional audio circuitry required
- Adequate for demonstrating theremin concept
- Direct GPIO control, minimal latency

**Trade-offs:**
- Limited audio quality (square wave only)
- No true volume control (only duty cycle variation)
- Upgrade path: ESP32 DAC + amplifier for future enhancement

### Decision: Dual I2C Address Management via XSHUT
**Rationale:**
- Both VL53L0X modules have same default address (0x29)
- XSHUT pins allow sequential initialization
- One sensor gets new address (0x30), other keeps 0x29
- Both can then operate simultaneously on same I2C bus

**Implementation:**
1. Set both XSHUT pins LOW (disable both sensors)
2. Enable sensor #1 (XSHUT HIGH), configure to 0x30
3. Enable sensor #2 (XSHUT HIGH), keep at 0x29
4. Both sensors now addressable independently

**Alternative:** I2C multiplexer (TCA9548A) - more complex, overkill for 2 sensors

## Design Patterns in Use

### 1. Sensor Reading Pattern
```cpp
// Continuous polling with error handling
uint16_t readSensor(Adafruit_VL53L0X &sensor) {
    VL53L0X_RangingMeasurementData_t measure;
    sensor.rangingTest(&measure, false);

    if (measure.RangeStatus != 4) {  // 4 = out of range
        return measure.RangeMilliMeter;
    }
    return lastValidReading;  // Use last good value on error
}
```

### 2. Signal Smoothing Pattern
```cpp
// Exponential Weighted Moving Average (EWMA) - Current Implementation
// Provides better responsiveness than simple moving average
// Alpha controls balance: 0.0 = very smooth/slow, 1.0 = no smoothing/instant
const float SMOOTHING_ALPHA = 0.3f;
float smoothedPitchDistance = 0.0f;
float smoothedVolumeDistance = 0.0f;
bool firstReading = true;

int applyExponentialSmoothing(float& smoothedValue, int newReading, bool isFirstReading) {
    if (isFirstReading) {
        // On first reading, initialize the smoothed value
        smoothedValue = (float)newReading;
    } else {
        // Apply EWMA formula: smoothed = alpha * new + (1 - alpha) * previous
        smoothedValue = (SMOOTHING_ALPHA * newReading) + ((1.0f - SMOOTHING_ALPHA) * smoothedValue);
    }
    return (int)smoothedValue;
}

// Legacy: Simple moving average (replaced October 27, 2025)
// Kept for reference - had ~100ms latency vs ~35ms with EWMA
/*
const int SAMPLES = 5;
int readings[SAMPLES];
int index = 0;

int smoothedRead(int newValue) {
    readings[index] = newValue;
    index = (index + 1) % SAMPLES;

    int sum = 0;
    for (int i = 0; i < SAMPLES; i++) {
        sum += readings[i];
    }
    return sum / SAMPLES;
}
*/
```

### 2a. Multi-Oscillator Mixing Pattern (October 27, 2025)
```cpp
// Per-Oscillator Volume Control Pattern
// Each oscillator has independent volume (0.0-1.0)
// Volume applied before mixing in Oscillator::getNextSample()

class Oscillator {
private:
    float volume;  // 0.0 = silent, 1.0 = full volume

public:
    void setVolume(float vol) {
        volume = constrain(vol, 0.0f, 1.0f);
    }

    int16_t getNextSample(float sampleRate) {
        // ... generate waveform sample ...

        // Apply volume control before returning
        return (int16_t)(sample * volume);
    }
};

// Multi-Oscillator Mixing with Automatic Clipping Prevention
// Sum all active oscillators, then average by count
// Math guarantees no clipping: max = (32767 + 19660 + 13107) / 3 = 21,845

void generateAudioBuffer() {
    for (int i = 0; i < BUFFER_SIZE; i++) {
        int activeCount = 0;
        int32_t mixedSample = 0;  // Use int32_t to prevent overflow

        // Add samples from all active oscillators (already volume-scaled)
        if (oscillator.isActive()) {
            mixedSample += oscillator.getNextSample((float)SAMPLE_RATE);
            activeCount++;
        }
        if (oscillator2.isActive()) {
            mixedSample += oscillator2.getNextSample((float)SAMPLE_RATE);
            activeCount++;
        }
        if (oscillator3.isActive()) {
            mixedSample += oscillator3.getNextSample((float)SAMPLE_RATE);
            activeCount++;
        }

        // Average to prevent clipping and maintain consistent volume
        int16_t sample = (activeCount > 0) ? (mixedSample / activeCount) : 0;

        // Apply global amplitude/gain if needed
        // ...
    }
}

// Example Configuration:
// oscillator.setWaveform(Oscillator::SINE);
// oscillator.setVolume(1.0);     // 100% - Full volume
//
// oscillator2.setWaveform(Oscillator::SQUARE);
// oscillator2.setOctaveShift(-1); // One octave down (sub-bass)
// oscillator2.setVolume(0.6);     // 60% - Quieter
//
// oscillator3.setWaveform(Oscillator::OFF);
// oscillator3.setVolume(0.4);     // 40% - Quietest (when enabled)
```

### 3. Range Mapping Pattern
```cpp
// Floating-point map for smooth frequency transitions (October 27, 2025)
// Eliminates quantization from integer map() function
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Map sensor distance to frequency with bounds checking
int mapDistance(int distance, int minDist, int maxDist,
                int minFreq, int maxFreq) {
    // Use float mapping for sub-Hz precision
    float frequencyFloat = mapFloat((float)distance,
                                     (float)minDist,
                                     (float)maxDist,
                                     (float)maxFreq,  // Note: inverted for theremin
                                     (float)minFreq);

    // Constrain and convert to integer
    int freq = constrain((int)frequencyFloat, minFreq, maxFreq);

    return freq;
}
```

## Critical Implementation Paths

### Path 1: Initialization Sequence
```
setup() {
    1. Serial.begin(115200)
    2. Wire.begin(GPIO21, GPIO22)  // I2C init
    3. pinMode(XSHUT1_PIN, OUTPUT)
    4. pinMode(XSHUT2_PIN, OUTPUT)

    5. digitalWrite(XSHUT1_PIN, LOW)   // Disable both
    6. digitalWrite(XSHUT2_PIN, LOW)
    7. delay(10)

    8. digitalWrite(XSHUT1_PIN, HIGH)  // Enable sensor 1
    9. delay(10)
    10. sensor1.begin(0x30)            // Set new address
    11. sensor1.startContinuous()

    12. digitalWrite(XSHUT2_PIN, HIGH) // Enable sensor 2
    13. delay(10)
    14. sensor2.begin(0x29)            // Default address
    15. sensor2.startContinuous()

    16. ledcSetup(PWM_CHANNEL, BASE_FREQ, RESOLUTION)
    17. ledcAttachPin(BUZZER_PIN, PWM_CHANNEL)
}
```

### Path 2: Main Loop Execution
```
loop() {
    1. Read sensor1 @ 0x30 → pitchDistance
    2. Smooth pitchDistance → smoothedPitch
    3. Map smoothedPitch → frequency (100-2000 Hz)

    4. Read sensor2 @ 0x29 → volumeDistance
    5. Smooth volumeDistance → smoothedVolume
    6. Map smoothedVolume → dutyCycle (0-255)

    7. ledcWriteTone(PWM_CHANNEL, frequency)
    8. ledcWrite(PWM_CHANNEL, dutyCycle)

    9. Optional: Serial debug output
    10. delay(10) // ~100Hz update rate
}
```

### Path 3: Error Recovery
```
I2C Error Detected:
    1. Log error to Serial
    2. Use last valid reading
    3. Continue operation
    4. If persistent (>10 errors): attempt sensor reset

Sensor Out of Range:
    1. Check RangeStatus == 4
    2. Use last valid value OR
    3. Set to safe default (silence or min/max)

Power/Init Failure:
    1. Serial error message
    2. Flash LED if available
    3. Halt or retry initialization
```

## Component Relationships

### I2C Bus Hierarchy
```
ESP32 I2C Controller (Wire)
├── VL53L0X #1 (Pitch) @ 0x30
│   ├── Controlled via XSHUT on GPIO16
│   └── Continuous ranging mode
└── VL53L0X #2 (Volume) @ 0x29
    ├── Controlled via XSHUT on GPIO17
    └── Continuous ranging mode
```

### Data Flow
```
Sensor Distance (mm)
    ↓
Smoothing Filter
    ↓
Range Validation
    ↓
Map to Parameter Space
    ↓
Frequency (Hz) or Duty Cycle (0-255)
    ↓
PWM Output
    ↓
Audio/Buzzer
```

## Timing Considerations

### Target Latency Budget
- Sensor read: ~20ms (VL53L0X measurement time)
- Smoothing: <1ms (simple averaging)
- Mapping: <1ms (arithmetic operations)
- PWM update: <1ms (register write)
- **Total: ~25-30ms** (acceptable for musical control)

### Loop Frequency
- Target: ~100Hz (10ms delay in loop)
- With sensor reads: actual ~50Hz (20ms per sensor pair)
- Trade-off: responsiveness vs. stability

## Controls Architecture (Phase 3 - November 2, 2025)

### Controls System Overview

```
Control Flow Architecture:

┌──────────────────────────────────────────────────────────────┐
│                        main.cpp                               │
│  Creates: SerialControls, GPIOControls, Theremin             │
└────────┬──────────────────────────────────┬──────────────────┘
         │                                  │
         ▼                                  ▼
┌─────────────────────┐         ┌──────────────────────────┐
│ SerialControls      │         │ GPIOControls             │
│ .h/cpp              │         │ .h/cpp                   │
├─────────────────────┤         ├──────────────────────────┤
│ - Command parser    │         │ - MCP23017 I2C GPIO      │
│ - Oscillator ctrl   │         │ - 3x waveform switches   │
│ - Effects ctrl      │         │ - 3x octave switches     │
│ - Sensor enable     │         │ - 50ms debouncing        │
│ - Help/Status       │         │ - Startup sync           │
└──────────┬──────────┘         └───────────┬──────────────┘
           │                                │
           └────────────┬───────────────────┘
                        │
                        ▼
            ┌───────────────────────┐
            │    Theremin.h/cpp     │
            │    (Coordinator)      │
            ├───────────────────────┤
            │ - getSensorManager()  │
            │ - AudioEngine access  │
            └───────────┬───────────┘
                        │
                        ▼
            ┌───────────────────────┐
            │   AudioEngine.h/cpp   │
            │   (Audio Synthesis)   │
            ├───────────────────────┤
            │ - Oscillator control  │
            │ - Effects chain       │
            │ - Thread-safe setters │
            └───────────────────────┘
```

### 9. GPIO Controls Pattern (MCP23017)

```cpp
// Hardware Control via I2C GPIO Expander
// Clean architecture: Controls as siblings of Theremin

class GPIOControls {
private:
    Theremin* theremin;
    Adafruit_MCP23X17 mcp;
    bool initialized;
    bool controlsEnabled;  // Master enable/disable
    bool firstUpdate;      // Force initial sync

    // State tracking for debouncing
    struct OscillatorState {
        Oscillator::Waveform waveform;
        int8_t octave;
        unsigned long lastChangeTime;
    };

    OscillatorState osc1State, osc2State, osc3State;
    static constexpr unsigned long DEBOUNCE_MS = 50;

public:
    GPIOControls(Theremin* theremin)
        : theremin(theremin), initialized(false),
          controlsEnabled(true), firstUpdate(true) {}

    bool begin() {
        // Initialize MCP23017
        if (!mcp.begin_I2C(0x20)) {
            Serial.println("[GPIO] MCP23017 not found!");
            return false;
        }

        // Configure all pins as INPUT_PULLUP
        for (uint8_t pin = 0; pin < 16; pin++) {
            mcp.pinMode(pin, INPUT_PULLUP);
        }

        initialized = true;

        // Critical: Read initial switch positions and apply
        // This ensures oscillators match physical switches at boot
        update();  // Force initial sync
        firstUpdate = false;  // Subsequent updates use debouncing

        return true;
    }

    void update() {
        if (!initialized || !controlsEnabled) return;

        // Update each oscillator from switches
        updateOscillator(1, osc1State,
            PIN_OSC1_SINE, PIN_OSC1_TRI, PIN_OSC1_SQ,
            PIN_OSC1_OCT_UP, PIN_OSC1_OCT_DOWN);
        // ... repeat for osc2, osc3
    }

private:
    void updateOscillator(int oscNum, OscillatorState& state,
                         uint8_t pinA, uint8_t pinB, uint8_t pinC,
                         uint8_t pinUp, uint8_t pinDown) {
        // Read current switch positions
        Oscillator::Waveform newWaveform = readWaveform(pinA, pinB, pinC);
        int8_t newOctave = readOctave(pinUp, pinDown);

        // Check if state changed
        bool changed = (newWaveform != state.waveform ||
                       newOctave != state.octave);

        // Debouncing: Ignore if too soon after last change
        unsigned long now = millis();
        if (!firstUpdate && changed &&
            (now - state.lastChangeTime < DEBOUNCE_MS)) {
            return;  // Too soon, ignore
        }

        // Apply changes (firstUpdate skips debouncing)
        if (firstUpdate || changed) {
            AudioEngine* audio = theremin->getAudioEngine();
            audio->setOscillatorWaveform(oscNum, newWaveform);
            audio->setOscillatorOctave(oscNum, newOctave);

            state.waveform = newWaveform;
            state.octave = newOctave;
            state.lastChangeTime = now;

            Serial.printf("[GPIO] Osc%d: %s, Octave %+d\n",
                oscNum, getWaveformName(newWaveform), newOctave);
        }
    }

    Oscillator::Waveform readWaveform(uint8_t pinA, uint8_t pinB, uint8_t pinC) {
        // Active LOW with INPUT_PULLUP
        bool a = !mcp.digitalRead(pinA);  // SINE
        bool b = !mcp.digitalRead(pinB);  // TRIANGLE
        bool c = !mcp.digitalRead(pinC);  // SQUARE

        if (a) return Oscillator::SINE;
        if (b) return Oscillator::TRIANGLE;
        if (c) return Oscillator::SQUARE;
        return Oscillator::OFF;  // All HIGH = OFF
    }

    int8_t readOctave(uint8_t pinUp, uint8_t pinDown) {
        // Binary encoding: 00=normal, 01=+1, 10=-1, 11=normal
        bool up = !mcp.digitalRead(pinUp);
        bool down = !mcp.digitalRead(pinDown);

        if (!up && !down) return 0;   // Both open
        if (!up && down) return -1;   // Down closed
        if (up && !down) return 1;    // Up closed
        return 0;                      // Both closed
    }
};
```

**Key Design Patterns:**

1. **Startup Sync Pattern**
   - Problem: Oscillators initialized with defaults, switches might be different
   - Solution: `firstUpdate` flag forces initial application without debouncing
   - Critical: Call update() in begin() before returning

2. **Debouncing Pattern**
   - Time-based debouncing (50ms window)
   - Track lastChangeTime per oscillator
   - Prevents switch bounce from causing audio glitches

3. **Active LOW Pattern**
   - MCP23017 pins configured INPUT_PULLUP
   - Switch connects pin to GND when closed
   - Pin HIGH = open, LOW = closed
   - Read with !mcp.digitalRead(pin) to invert logic

4. **Graceful Degradation**
   - System continues if MCP23017 not found
   - Serial commands still work (fallback mode)
   - Easy development without hardware

### 10. Serial Controls Pattern

```cpp
// Text-based control interface for debugging/testing

class SerialControls {
private:
    Theremin* theremin;

public:
    SerialControls(Theremin* theremin) : theremin(theremin) {}

    void update() {
        handleSerialCommands();
    }

private:
    void handleSerialCommands() {
        if (!Serial.available()) return;

        String cmd = Serial.readStringUntil('\n');
        cmd.trim();

        if (cmd.length() == 0) return;

        // Support batch commands (semicolon-separated)
        int sepIndex;
        while ((sepIndex = cmd.indexOf(';')) != -1) {
            String subcmd = cmd.substring(0, sepIndex);
            subcmd.trim();
            executeCommand(subcmd);
            cmd = cmd.substring(sepIndex + 1);
            cmd.trim();
        }
        executeCommand(cmd);
    }

    void executeCommand(String cmd) {
        cmd.toLowerCase();

        // Help commands
        if (cmd == "help" || cmd == "?") {
            printHelp();
            return;
        }

        // Status commands
        if (cmd == "status") {
            printStatus();
            return;
        }

        // Oscillator commands: osc1:sine, osc2:octave:-1, etc.
        if (cmd.startsWith("osc")) {
            int oscNum = cmd.charAt(3) - '0';
            if (oscNum < 1 || oscNum > 3) {
                Serial.println("[CMD] Invalid oscillator (1-3)");
                return;
            }

            // Parse parameter: osc1:waveform or osc1:octave:value
            int colon1 = cmd.indexOf(':', 4);
            if (colon1 == -1) return;

            String param = cmd.substring(colon1 + 1);

            // Handle waveform change
            if (param.startsWith("sine") || param.startsWith("square") ||
                param.startsWith("tri") || param.startsWith("saw") ||
                param.startsWith("off")) {
                int wf = parseWaveform(param);
                if (wf != -1) {
                    theremin->getAudioEngine()->setOscillatorWaveform(
                        oscNum, (Oscillator::Waveform)wf);
                    Serial.printf("[CMD] Osc%d waveform: %s\n",
                        oscNum, param.c_str());
                }
            }
            // Handle octave change: osc1:octave:-1
            else if (param.startsWith("oct")) {
                int colon2 = param.indexOf(':');
                if (colon2 != -1) {
                    int octave = param.substring(colon2 + 1).toInt();
                    theremin->getAudioEngine()->setOscillatorOctave(
                        oscNum, octave);
                    Serial.printf("[CMD] Osc%d octave: %+d\n",
                        oscNum, octave);
                }
            }
            // Handle volume change: osc1:vol:0.5
            else if (param.startsWith("vol")) {
                int colon2 = param.indexOf(':');
                if (colon2 != -1) {
                    float volume = param.substring(colon2 + 1).toFloat();
                    theremin->getAudioEngine()->setOscillatorVolume(
                        oscNum, volume);
                    Serial.printf("[CMD] Osc%d volume: %.2f\n",
                        oscNum, volume);
                }
            }
        }

        // Effects commands: delay:on, chorus:mix:0.3, etc.
        // Sensor commands: sensors:pitch:off, etc.
        // ... (similar pattern)
    }
};
```

**Key Design Patterns:**

1. **Batch Command Pattern**
   - Support semicolon-separated commands
   - Example: `osc1:sine;osc1:octave:1;osc1:vol:0.8`
   - Useful for preset loading

2. **"Last Wins" Behavior**
   - Serial and GPIO both control same oscillators
   - No arbitration needed - whichever is used last takes control
   - Useful for debugging: override hardware with serial

3. **Forward Declaration Pattern**
   - Avoid circular dependencies
   - SerialControls.h forward declares Theremin
   - Full Theremin.h included only in SerialControls.cpp

## Effects Architecture (Phase 4 - October 31, 2025)

### Effects System Overview

```
Audio Signal Flow with Effects:

Oscillator 1 ─┐
Oscillator 2 ─┼─→ Mixer ─→ EffectsChain ─→ DAC Output
Oscillator 3 ─┘               │
                              ↓
                    ┌─────────┴──────────┐
                    │                    │
              DelayEffect          ChorusEffect
              (circular buffer)    (modulated delay + LFO)
                    │                    │
                    └─────────┬──────────┘
                              ↓
                         Mixed Output
```

### Effects Chain Architecture

```cpp
// Stack Allocation Pattern (RAII)
// Effects are direct members of EffectsChain, not pointers
// Benefit: Automatic cleanup, no heap fragmentation

class EffectsChain {
private:
    uint32_t sampleRate;

    // Direct member initialization (stack allocation)
    DelayEffect delay;    // ~13KB for 300ms @ 22050 Hz
    ChorusEffect chorus;  // ~3KB for 50ms max depth

public:
    EffectsChain(uint32_t sampleRate)
        : sampleRate(sampleRate),
          delay(300, sampleRate),      // Constructor initializer list
          chorus(sampleRate) {

        // Configure effects
        delay.setFeedback(0.5f);
        delay.setMix(0.3f);
        delay.setEnabled(false);

        chorus.setRate(1.0f);
        chorus.setDepth(5.0f);
        chorus.setMix(0.2f);
        chorus.setEnabled(false);
    }

    // Signal flow: input → delay → chorus → output
    int16_t process(int16_t input) {
        int16_t output = input;
        output = delay.process(output);
        output = chorus.process(output);
        return output;
    }
};
```

### 4. Delay Effect Pattern

```cpp
// Digital Delay with Circular Buffer
// Classic feedback delay implementation

class DelayEffect {
private:
    int16_t* delayBuffer;  // Circular buffer
    size_t bufferSize;     // In samples
    size_t writeIndex;     // Current write position

    float feedback;        // 0.0-0.95 (prevent runaway)
    float wetDryMix;       // 0.0-1.0
    bool enabled;

public:
    int16_t process(int16_t input) {
        // Bypass optimization
        if (!enabled) return input;

        // Read delayed sample (circular buffer)
        int16_t delayedSample = delayBuffer[writeIndex];

        // Write with feedback: new = input + (delayed * feedback)
        int32_t newSample = input + (delayedSample * feedback);
        newSample = constrain(newSample, -32768, 32767);
        delayBuffer[writeIndex] = (int16_t)newSample;

        // Advance write pointer (circular)
        writeIndex = (writeIndex + 1) % bufferSize;

        // Mix dry and wet signals
        int32_t output = (input * (1.0f - wetDryMix)) + (delayedSample * wetDryMix);
        return constrain(output, -32768, 32767);
    }
};

// Memory calculation:
// Buffer size = (delay_ms / 1000.0) * sample_rate
// Example: 300ms @ 22050 Hz = 6615 samples = 13230 bytes
```

### 5. Chorus Effect Pattern with Oscillator-Based LFO

```cpp
// Modulated Delay for Chorus/Vibrato Effects
// Key Innovation: Reuses Oscillator class as LFO!

class ChorusEffect {
private:
    int16_t* delayBuffer;
    size_t bufferSize;
    size_t writeIndex;

    Oscillator lfo;        // Reuse Oscillator as LFO!
    float lfoDepthMs;      // Modulation depth
    float wetDryMix;

public:
    ChorusEffect(uint32_t sampleRate)
        : sampleRate(sampleRate) {

        // Configure LFO (using Oscillator class)
        lfo.setWaveform(Oscillator::SINE);
        lfo.setFrequency(2.0f);      // 2 Hz LFO rate
        lfo.setVolume(1.0f);

        // Allocate buffer for max depth (50ms)
        bufferSize = (50.0f / 1000.0f) * sampleRate + 100;
        delayBuffer = new int16_t[bufferSize];
        memset(delayBuffer, 0, bufferSize * sizeof(int16_t));
    }

    int16_t process(int16_t input) {
        if (!enabled) return input;

        // Write input
        delayBuffer[writeIndex] = input;

        // Get LFO value (sine LUT, ~100x faster than sin()!)
        float lfoValue = lfo.getNextSampleNormalized(sampleRate);

        // Calculate modulated delay time
        float delayTimeMs = lfoDepthMs + (lfoValue * lfoDepthMs);
        float delayInSamples = (delayTimeMs / 1000.0f) * sampleRate;

        // Read with linear interpolation
        int16_t delayedSample = readDelayBuffer(delayInSamples);

        // Mix
        int32_t output = (input * (1.0f - wetDryMix)) + (delayedSample * wetDryMix);

        writeIndex = (writeIndex + 1) % bufferSize;
        return constrain(output, -32768, 32767);
    }

    // Linear interpolation for fractional delay reads
    int16_t readDelayBuffer(float delayInSamples) {
        float readPos = (float)writeIndex - delayInSamples;

        // Wrap around
        while (readPos < 0) readPos += bufferSize;
        while (readPos >= bufferSize) readPos -= bufferSize;

        // Interpolate between two samples
        int index1 = (int)readPos;
        int index2 = (index1 + 1) % bufferSize;
        float fraction = readPos - index1;

        return (int16_t)(delayBuffer[index1] * (1.0f - fraction) +
                         delayBuffer[index2] * fraction);
    }
};
```

### 6. Oscillator-Based LFO Pattern

```cpp
// Design Pattern: Reuse Oscillator class for LFO
// Massive Performance Benefit: Sine LUT vs sin() calls

// BAD: Traditional LFO implementation (slow)
float getLFOValue_Slow(float phase) {
    return sin(phase * 2.0f * PI);  // ~100x slower!
}

// GOOD: Oscillator-based LFO (fast)
class ChorusEffect {
    Oscillator lfo;  // Reuse existing Oscillator class

    void configureLFO() {
        lfo.setWaveform(Oscillator::SINE);
        lfo.setFrequency(2.0f);  // Sub-audio rate (0.1-10 Hz)
        lfo.setVolume(1.0f);
    }

    float getLFOValue() {
        // Uses optimized sine LUT from Oscillator
        return lfo.getNextSampleNormalized(sampleRate);  // Fast!
    }
};

// Critical Bug Fix (October 31, 2025):
// Oscillator::setFrequency() was constraining to 20 Hz minimum (audio range)
// Updated to allow 0.1-20000 Hz range to support LFO use

// Before (bug):
void setFrequency(float freq) {
    frequency = constrain(freq, 20.0f, 20000.0f);  // Too restrictive!
}

// After (fixed):
void setFrequency(float freq) {
    frequency = constrain(freq, 0.1f, 20000.0f);  // Allows LFO range
}
```

### 7. Stack vs Heap Allocation Pattern

```cpp
// Pattern: RAII with Stack Allocation for Effects

// BAD: Heap allocation (fragmentation risk)
class EffectsChain_Bad {
    DelayEffect* delay;    // Pointers = heap allocation
    ChorusEffect* chorus;

    EffectsChain_Bad() {
        delay = new DelayEffect(300, 22050);    // Heap allocation
        chorus = new ChorusEffect(22050);       // Heap allocation
    }

    ~EffectsChain_Bad() {
        delete delay;    // Manual cleanup required
        delete chorus;   // Easy to forget or get wrong
    }
};

// GOOD: Stack allocation (RAII pattern)
class EffectsChain_Good {
    DelayEffect delay;     // Direct members = stack allocation
    ChorusEffect chorus;

    EffectsChain_Good(uint32_t sampleRate)
        : delay(300, sampleRate),    // Constructor initializer list
          chorus(sampleRate) {
        // Effects constructed on stack
        // Automatic cleanup when EffectsChain destroyed
    }

    // No destructor needed - automatic cleanup!
    // RAII: Resource Acquisition Is Initialization
};

// Benefits of Stack Allocation:
// ✓ No heap fragmentation (critical for embedded systems)
// ✓ Automatic cleanup (RAII pattern)
// ✓ Slightly faster allocation/deallocation
// ✓ Deterministic memory usage
// ✓ Simpler code (no manual new/delete)

// Trade-offs:
// ✗ Fixed at compile time (can't dynamically add/remove effects)
// ✗ Uses more stack space (but we have plenty)
```

### 8. Bypass Optimization Pattern

```cpp
// Pattern: Early return for disabled effects
// Minimizes CPU usage when effect is bypassed

class DelayEffect {
    bool enabled;

    int16_t process(int16_t input) {
        // Check enabled flag FIRST
        if (!enabled) {
            return input;  // Immediate return, no processing!
        }

        // Only execute expensive operations if enabled
        int16_t delayedSample = delayBuffer[writeIndex];
        // ... rest of delay processing ...

        return output;
    }
};

// Performance Impact:
// Enabled:  ~3-5% CPU (full delay processing)
// Disabled: ~0.1% CPU (single if-check + return)

// This is why our system uses only 9% CPU with 3 oscillators + 2 effects:
// - 3 oscillators: ~6%
// - Delay (enabled): ~1.5%
// - Chorus (enabled): ~1.5%
// - Total: ~9% (91% headroom!)
```

### Effects Integration in AudioEngine

```cpp
// Modified Audio Pipeline (October 31, 2025)

void AudioEngine::generateAudioBuffer() {
    for (size_t i = 0; i < BUFFER_SIZE; i++) {
        // Step 1: Mix oscillators
        int16_t sample = mixOscillators();

        // Step 2: Apply effects chain (NEW!)
        sample = effectsChain->process(sample);

        // Step 3: Apply amplitude smoothing
        smoothedAmplitude += (currentAmplitude - smoothedAmplitude) * SMOOTHING_FACTOR;
        sample = (int16_t)(sample * (smoothedAmplitude / 100.0f));

        // Step 4: Convert to DAC format (unsigned 8-bit)
        uint8_t dacValue = (sample >> 8) + 128;
        buffer[i] = dacValue;
    }
}

// Critical: Effects MUST be processed after mixing, before DAC conversion
// Order matters: Mix → Effects → Amplitude → DAC Format
```

### Performance Characteristics

**CPU Usage Breakdown (3 osc + delay + chorus):**
```
Component                   CPU %    Notes
─────────────────────────────────────────────────────────
Oscillator 1 (SINE)        ~2%      Phase accumulator + sine LUT
Oscillator 2 (SQUARE)      ~2%      Phase accumulator + comparison
Oscillator 3 (OFF)         ~0.1%    Early return optimization
Mixing (averaging)         ~1%      Integer math
DelayEffect (enabled)      ~1.5%    Circular buffer operations
ChorusEffect (enabled)     ~1.5%    Interpolation + LFO lookup
Amplitude smoothing        ~0.5%    EWMA calculation
DAC format conversion      ~0.5%    Bit shift + offset
─────────────────────────────────────────────────────────
Total                      ~9%      (91% headroom available!)
```

**Memory Usage:**
```
Component                Size        Location
──────────────────────────────────────────────────
DelayEffect buffer       ~13 KB      Stack (in EffectsChain)
ChorusEffect buffer      ~3 KB       Stack (in EffectsChain)
EffectsChain object      ~16 KB      Stack (in AudioEngine)
Oscillator sine LUT      512 bytes   PROGMEM (Flash)
──────────────────────────────────────────────────
Total RAM impact         ~16 KB      (from 314 KB free)
```

### 11. Display Page Registration Pattern (November 2025)

```cpp
// Page-based display system with callback registration
// Components register pages using lambda callbacks

class DisplayManager {
private:
    std::vector<DisplayPage> pages;
    std::vector<PageDrawCallback> overlays;
    uint8_t currentPageIndex;

public:
    // Register a page with name, callback, optional title, and weight
    void registerPage(String name, PageDrawCallback drawFunc, String title = "", int weight = 0) {
        pages.emplace_back(name, drawFunc, title, weight);

        // Sort by weight (ascending), then alphabetically
        std::sort(pages.begin(), pages.end(), [](const DisplayPage& a, const DisplayPage& b) {
            if (a.weight != b.weight) {
                return a.weight < b.weight;  // Lower weight = earlier
            }
            return a.name < b.name;  // Alphabetical for ties
        });
    }

    // Register global overlay (appears on all pages)
    void registerOverlay(PageDrawCallback overlayFunc) {
        overlays.push_back(overlayFunc);
    }

    void update() {
        if (pages.empty()) return;

        display.clearDisplay();

        // 1. Auto-draw title if provided
        if (!pages[currentPageIndex].title.isEmpty()) {
            display.setCursor(0, 0);
            display.print(pages[currentPageIndex].title.toUpperCase());
            display.drawLine(0, 9, SCREEN_WIDTH - 1, 9, SSD1306_WHITE);
        }

        // 2. Draw current page content
        pages[currentPageIndex].drawFunction(display);

        // 3. Draw all overlays on top
        for (auto& overlay : overlays) {
            overlay(display);
        }

        display.display();
    }
};

// Component registers its own pages
class PerformanceMonitor {
public:
    void setDisplay(DisplayManager* disp) {
        displayManager = disp;

        // Register page using lambda that captures 'this'
        displayManager->registerPage("Performance", [this](Adafruit_SSD1306& oled) {
            this->drawPerformancePage(oled);
        }, "SYSTEM", 1);  // Title "SYSTEM", weight 1
    }

private:
    void drawPerformancePage(Adafruit_SSD1306& oled) {
        oled.setCursor(0, DisplayManager::CONTENT_START_Y);
        oled.printf("CPU: %.1f%%\n", getCPUUsage());
        oled.printf("RAM: %d KB\n", getFreeRAM());
    }
};
```

**Key Design Patterns:**

1. **Callback-Based Registration**
   - Components register drawing callbacks, not static data
   - Allows dynamic content updates
   - Lambda captures enable access to private members

2. **Weight-Based Sorting**
   - Pages ordered by weight (lower = earlier)
   - Alphabetical sorting for equal weights
   - Predictable page order

3. **Auto-Title Rendering**
   - Optional title parameter
   - DisplayManager renders title + separator if provided
   - Cursor auto-positioned at CONTENT_START_Y

4. **Decoupled Components**
   - Each component manages its own page content
   - No central knowledge of all pages needed
   - Easy to add/remove pages

### 12. Notification Overlay Pattern (November 2025)

```cpp
// Time-limited notification system using singleton pattern
// Shows brief status messages on top of any page

class NotificationManager {
private:
    DisplayManager* displayManager;
    String currentMessage;
    unsigned long hideTime;
    bool active;

    static NotificationManager* instance;  // Singleton for callback

public:
    NotificationManager(DisplayManager* display) : displayManager(display) {
        instance = this;  // Set singleton instance

        // Register overlay callback in constructor
        if (displayManager) {
            displayManager->registerOverlay(overlayCallbackStatic);
        }
    }

    // Show notification with auto-hide
    void show(String message, uint16_t durationMs = 2000) {
        currentMessage = message;
        hideTime = millis() + durationMs;
        active = true;
    }

    // Update - call in main loop
    void update() {
        if (active && millis() >= hideTime) {
            clear();
        }
    }

    void clear() {
        active = false;
        currentMessage = "";
    }

private:
    // Instance method - actual drawing
    void drawOverlay(Adafruit_SSD1306& display) {
        if (!active || currentMessage.length() == 0) {
            return;  // Nothing to draw
        }

        // Measure text bounds
        display.setFont(&TomThumb);
        int16_t x1, y1;
        uint16_t textWidth, textHeight;
        display.getTextBounds(currentMessage, 0, 0, &x1, &y1, &textWidth, &textHeight);

        // Position at bottom-center
        const int PADDING = 4;
        const int BOX_HEIGHT = textHeight + (PADDING * 2);
        const int BOX_WIDTH = textWidth + (PADDING * 2);
        const int BOX_X = (DisplayManager::SCREEN_WIDTH - BOX_WIDTH) / 2;
        const int BOX_Y = DisplayManager::SCREEN_HEIGHT - BOX_HEIGHT - 2;

        // Draw background box
        display.fillRect(BOX_X, BOX_Y, BOX_WIDTH, BOX_HEIGHT, SSD1306_WHITE);

        // Draw text (inverted colors)
        display.setTextColor(SSD1306_BLACK);
        display.setCursor(BOX_X + PADDING, BOX_Y + PADDING + textHeight);
        display.print(currentMessage);

        display.setFont(nullptr);  // Reset font
    }

    // Static callback wrapper required for DisplayManager
    static void overlayCallbackStatic(Adafruit_SSD1306& display) {
        if (instance) {
            instance->drawOverlay(display);
        }
    }
};

// Static member definition
NotificationManager* NotificationManager::instance = nullptr;

// Usage example:
notificationManager->show("OSC1:SIN");     // Show for 2 seconds
notificationManager->show("ERROR", 5000);  // Show for 5 seconds
```

**Key Design Patterns:**

1. **Singleton Pattern**
   - Static instance pointer for callback access
   - Required because DisplayManager uses std::function callbacks
   - Only one NotificationManager should exist

2. **Static Callback Wrapper**
   - Static method required for registration
   - Forwards to instance method for actual work
   - Common pattern for C++ callbacks

3. **Auto-Hide Timer**
   - Stores hideTime as millis() timestamp
   - update() checks if time expired
   - No separate timer task needed

4. **Overlay System Integration**
   - Registers as global overlay in constructor
   - Appears on top of any page
   - Conditional rendering (only if active)

5. **Text Bounds Calculation**
   - Dynamic box sizing based on message length
   - getTextBounds() for accurate measurements
   - Ensures notification fits on screen

**Memory Impact:**
- NotificationManager instance: ~100 bytes
- No heap allocation in hot path (update/draw)
- String stored only when active

**Performance:**
- Negligible when inactive (early return)
- ~1-2ms draw time when active
- No impact on audio performance

### 13. NetworkManager Pattern (November 2025)

```cpp
// Unified network infrastructure coordinator
// Encapsulates WiFiManager, OTAManager, and mDNS registration

class NetworkManager {
private:
    AsyncWebServer* server;  // Shared server pointer
    WiFiManager wifiManager;
    OTAManager* otaManager;
    DisplayManager* displayManager;

    bool initialized;
    String ssid;
    String localIP;
    int8_t rssi;

public:
    NetworkManager(AsyncWebServer* srv, DisplayManager* disp = nullptr)
        : server(srv), displayManager(disp), otaManager(nullptr), initialized(false) {}

    bool begin(bool resetCredentials = false) {
        // Setup WiFi with captive portal
        if (!setupWiFi(resetCredentials)) {
            return false;
        }

        // Setup mDNS
        if (!setupMDNS("theremin")) {
            DEBUG_PRINTLN("[Network] mDNS failed");
        }

        // Initialize OTA if needed
        setupOTA();

        // Register display page if available
        registerDisplayPage();

        initialized = true;
        return true;
    }

private:
    bool setupWiFi(bool resetCredentials) {
        if (resetCredentials) {
            DEBUG_PRINTLN("[Network] Resetting WiFi credentials...");
            wifiManager.resetSettings();
        }

        // Configure WiFiManager
        wifiManager.setConfigPortalTimeout(180);  // 3 minutes
        wifiManager.setAPCallback([](WiFiManager* mgr) {
            DEBUG_PRINTLN("[WiFi] Entered config mode");
        });

        // Auto-connect or start captive portal
        if (!wifiManager.autoConnect("Theremin-Setup")) {
            DEBUG_PRINTLN("[WiFi] Failed to connect");
            return false;
        }

        // Store connection info
        ssid = WiFi.SSID();
        localIP = WiFi.localIP().toString();
        rssi = WiFi.RSSI();

        DEBUG_PRINTLN("[WiFi] Connected!");
        return true;
    }

    bool setupMDNS(const char* hostname) {
        if (MDNS.begin(hostname)) {
            MDNS.addService("http", "tcp", 80);
            DEBUG_PRINTF("[mDNS] Registered: %s.local\n", hostname);
            return true;
        }
        return false;
    }

    void setupOTA() {
        if (!server) return;

        otaManager = new OTAManager(server);
        if (otaManager->begin("admin", "theremin")) {
            DEBUG_PRINTLN("[OTA] Enabled at /update");
        }
    }

    void registerDisplayPage() {
        if (!displayManager) return;

        displayManager->registerPage("Network", [this](Adafruit_SSD1306& oled) {
            oled.setCursor(0, DisplayManager::CONTENT_START_Y);
            oled.printf("SSID: %s\n", ssid.c_str());
            oled.printf("IP: %s\n", localIP.c_str());
            oled.printf("Signal: %d dBm\n", rssi);
            oled.printf("Mode: %s\n",
                WiFi.getMode() == WIFI_MODE_STA ? "STA" :
                WiFi.getMode() == WIFI_MODE_AP ? "AP" : "AP+STA");
        }, "NETWORK", 10);  // Title "NETWORK", weight 10 (after system pages)
    }
};
```

**Key Design Patterns:**

1. **Coordinator Pattern**
   - NetworkManager coordinates WiFiManager, OTAManager, and mDNS
   - Single point of initialization for all network features
   - Simplifies main.cpp - one begin() call instead of many

2. **Shared AsyncWebServer Pattern**
   - Server created in main.cpp, passed as pointer
   - OTA and future WebSocket backend share same server instance
   - Only one server.begin() call needed
   - Prevents port conflicts

3. **WiFi Credentials Reset**
   - Special boot state detection (in main.cpp)
   - All oscillators OFF + all octave switches -1 + button held
   - NetworkManager::begin(true) triggers wifiManager.resetSettings()
   - System boots into AP mode for reconfiguration

4. **Display Integration**
   - Optional DisplayManager pointer in constructor
   - Auto-registers network status page if display available
   - Shows SSID, IP, signal strength, WiFi mode
   - Lambda callback captures 'this' for live updates

5. **Auto-Reconnect Pattern**
   - WiFiManager handles connection logic
   - Saves credentials to flash automatically
   - Auto-connects on subsequent boots
   - Falls back to AP mode if connection fails

**System Reboot Feature (Bonus):**
```cpp
// In GPIOControls.cpp - Very long press detection
void GPIOControls::updateButton() {
    // ... existing state machine ...

    case LONG_PRESS_ACTIVE:
        if (!buttonPressed) {
            // Released - exit modifier mode
            buttonState = IDLE;
            modifierActive = false;
            modifierWasUsed = false;
        }
        else if (!modifierWasUsed &&
                 (now - buttonPressTime >= VERY_LONG_PRESS_THRESHOLD_MS)) {
            // Very long press - trigger reboot (only if modifier not used)
            performSystemReboot();
        }
        break;
}

void GPIOControls::performSystemReboot() {
    DEBUG_PRINTLN("[GPIO] VERY LONG PRESS - REBOOTING...");

    // Show notification
    showNotification("REBOOTING...", 2000);

    // CRITICAL: Force display update to show notification immediately
    // Without this, the notification never appears (loop is blocked)
    if (displayManager) {
        displayManager->update();
    }

    // Delay to let user see notification (audio stops after ~11ms)
    delay(2000);

    // Restart ESP32
    ESP.restart();
}
```

**Protection Against Accidental Reboot:**
- `modifierWasUsed` flag tracks if any secondary control touched
- Set to false when entering modifier mode
- Set to true when any secondary control changed
- Reboot only triggers if flag is still false after 10s
- Safe to use modifier mode for extended periods

**Memory Impact:**
- NetworkManager instance: ~100 bytes
- WiFiManager library: ~20KB Flash, ~2KB RAM
- AsyncWebServer: ~30KB Flash, ~5KB RAM (shared with OTA)

## Configuration Constants

```cpp
// Hardware pins
#define I2C_SDA 21
#define I2C_SCL 22
#define XSHUT1_PIN 16
#define XSHUT2_PIN 17
#define BUZZER_PIN 25

// I2C addresses
#define SENSOR1_ADDR 0x30  // Pitch
#define SENSOR2_ADDR 0x29  // Volume

// Operating ranges
#define PITCH_MIN_DIST 50    // mm
#define PITCH_MAX_DIST 400   // mm
#define VOLUME_MIN_DIST 50   // mm
#define VOLUME_MAX_DIST 300  // mm

// Audio ranges
#define MIN_FREQUENCY 100    // Hz
#define MAX_FREQUENCY 2000   // Hz
#define PWM_CHANNEL 0
#define PWM_RESOLUTION 8     // bits

// Effects parameters (October 31, 2025)
#define DELAY_TIME_DEFAULT 300      // ms
#define DELAY_FEEDBACK_DEFAULT 0.5f // 50%
#define DELAY_MIX_DEFAULT 0.3f      // 30% wet
#define CHORUS_RATE_DEFAULT 1.0f    // Hz (LFO frequency)
#define CHORUS_DEPTH_DEFAULT 5.0f   // ms (modulation depth)
#define CHORUS_MIX_DEFAULT 0.2f     // 20% wet
