# Wokwi Simulation Plan - ESP32 Theremin
## Option C: Mock Sensor Data with Potentiometers

**Created**: October 15, 2025
**Strategy**: Replace VL53L0X sensors with potentiometers for simulation testing
**Goal**: Validate audio synthesis logic without sensor dependencies

---

## Overview

This plan replaces the VL53L0X Time-of-Flight sensors with potentiometers in the Wokwi simulation environment. This allows us to test the theremin's audio synthesis logic (frequency/volume mapping and PWM generation) without dependencies on specific sensor availability in the simulator.

### Why This Approach?

- ✅ **No sensor dependencies** - Works immediately in Wokwi
- ✅ **Validates core logic** - Tests frequency/volume mapping independently
- ✅ **Easy debugging** - Clear ADC → Distance → Audio signal path
- ✅ **Reproducible tests** - Set exact potentiometer values for testing
- ✅ **Preserves hardware code** - VL53L0X code remains intact for hardware deployment
- ✅ **Fast iteration** - No hardware assembly required for initial testing

---

## Architecture Changes

### Hardware Mapping Table

| Real Hardware Component | Wokwi Simulation Component | GPIO Pin | Purpose |
|------------------------|---------------------------|----------|---------|
| VL53L0X #1 (Pitch) | Potentiometer | GPIO34 (ADC1_CH6) | Simulates pitch distance (5-400mm) |
| VL53L0X #2 (Volume) | Potentiometer | GPIO35 (ADC1_CH7) | Simulates volume distance (5-300mm) |
| Passive Buzzer | Wokwi Buzzer | GPIO25 (PWM) | Audio output (unchanged) |
| 220Ω Resistor | Wokwi Resistor | Between GPIO25 & Buzzer | Current limiting |

### Signal Flow

```
Potentiometer → ADC Reading (0-4095) → Distance Simulation (mm) → Smoothing Filter →
Frequency/Volume Mapping → PWM Generation → Buzzer Output
```

### Code Strategy

Use **conditional compilation with clean helper functions** in a single `main.cpp`:
- `WOKWI_SIMULATION` defined: Use analog input from potentiometers
- `WOKWI_SIMULATION` undefined: Use VL53L0X I2C sensors (hardware)

**Architecture Benefits**:
- ✅ Single source file - no duplication
- ✅ Clean helper functions: `simulationSetup()`, `hardwareSetup()`, `simulationReadPitch()`, etc.
- ✅ Shared audio synthesis logic in `processAndPlayAudio()`
- ✅ Crystal clear `setup()` and `loop()` functions (~10 lines each)
- ✅ Easy maintenance - change audio logic once, affects both builds

---

## Implementation Steps

### Step 1: Create `wokwi.toml`

**Location**: Project root directory (`/Users/fquagliati/claude/theremin/wokwi.toml`)

**Purpose**: Tells Wokwi where to find the compiled firmware

```toml
[wokwi]
version = 1

# PlatformIO builds firmware to these locations
firmware = '.pio/build/esp32dev/firmware.bin'
elf = '.pio/build/esp32dev/firmware.elf'

# Optional: Enable RFC2217 serial port forwarding
# Allows external serial monitor tools to connect
rfc2217ServerPort = 4000
```

**Notes**:
- `firmware`: Binary file for simulation
- `elf`: Debug symbols for better error messages
- `rfc2217ServerPort`: Optional serial port forwarding (useful for external tools)

---

### Step 2: Create `diagram.json`

**Location**: Project root directory (`/Users/fquagliati/claude/theremin/diagram.json`)

**Purpose**: Defines the virtual circuit layout and connections

```json
{
  "version": 1,
  "author": "ESP32 Theremin Project",
  "editor": "wokwi",
  "parts": [
    {
      "type": "wokwi-esp32-devkit-v1",
      "id": "esp32",
      "top": 0,
      "left": 0,
      "attrs": {}
    },
    {
      "type": "wokwi-potentiometer",
      "id": "pitch_pot",
      "top": -100,
      "left": -150,
      "attrs": {
        "label": "Pitch (Distance)"
      }
    },
    {
      "type": "wokwi-potentiometer",
      "id": "volume_pot",
      "top": -100,
      "left": 100,
      "attrs": {
        "label": "Volume (Distance)"
      }
    },
    {
      "type": "wokwi-buzzer",
      "id": "buzzer",
      "top": 200,
      "left": 0,
      "attrs": {
        "volume": "0.5"
      }
    },
    {
      "type": "wokwi-resistor",
      "id": "r1",
      "top": 150,
      "left": 50,
      "attrs": {
        "value": "220"
      }
    }
  ],
  "connections": [
    ["pitch_pot:SIG", "esp32:GPIO34", "green", ["h0"]],
    ["pitch_pot:GND", "esp32:GND.1", "black", ["h0"]],
    ["pitch_pot:VCC", "esp32:3V3", "red", ["h0"]],

    ["volume_pot:SIG", "esp32:GPIO35", "blue", ["h0"]],
    ["volume_pot:GND", "esp32:GND.2", "black", ["h0"]],
    ["volume_pot:VCC", "esp32:3V3", "red", ["h0"]],

    ["esp32:GPIO25", "r1:1", "purple", ["h0"]],
    ["r1:2", "buzzer:1", "purple", ["h0"]],
    ["buzzer:2", "esp32:GND.3", "black", ["h0"]]
  ]
}
```

**Key Components**:
- **ESP32 DevKit v1**: Main microcontroller
- **2x Potentiometers**: Simulate distance sensors (0-3.3V analog output)
- **1x Buzzer**: Audio output
- **1x 220Ω Resistor**: Current limiting for buzzer protection

**Connection Summary**:
- Left potentiometer (Pitch) → GPIO34 (ADC input)
- Right potentiometer (Volume) → GPIO35 (ADC input)
- Buzzer → GPIO25 via 220Ω resistor
- All components powered from ESP32 3.3V rail

---

### Step 3: Update `platformio.ini`

**Location**: Project root directory (`/Users/fquagliati/claude/theremin/platformio.ini`)

**Purpose**: Create separate build environment for Wokwi simulation

**Additions to existing file**:

```ini
; Existing hardware environment (unchanged)
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps =
    adafruit/Adafruit_VL53L0X@^1.2.0
monitor_speed = 115200
upload_speed = 921600

; New simulation-specific environment
[env:esp32dev-wokwi]
platform = espressif32
board = esp32dev
framework = arduino
build_flags =
    -DWOKWI_SIMULATION
monitor_speed = 115200
upload_speed = 921600
; Note: VL53L0X library omitted - not needed for simulation
```

**Key Differences**:
- `env:esp32dev-wokwi`: New environment name
- `build_flags = -DWOKWI_SIMULATION`: Enables conditional compilation
- VL53L0X library excluded from simulation build (reduces compile time)

**Build Commands**:
```bash
# Build for Wokwi simulation
pio run -e esp32dev-wokwi

# Build for hardware
pio run -e esp32dev
```

---

### Step 4: Refactor `main.cpp` with Clean Architecture

**Location**: `src/main.cpp` (modify existing file)

**Purpose**: Single file with conditional compilation and clean helper functions

**Complete Implementation**:

```cpp
/*
 * ESP32 Theremin - Clean Architecture
 *
 * Single file supporting both Wokwi simulation and hardware via conditional compilation.
 * Uses WOKWI_SIMULATION macro to switch between potentiometers (simulation) and VL53L0X sensors (hardware).
 *
 * Build Commands:
 * - Simulation: pio run -e esp32dev-wokwi
 * - Hardware:   pio run -e esp32dev
 */

#include <Arduino.h>

// ============================================================================
// CONDITIONAL INCLUDES & DEFINITIONS
// ============================================================================
#ifdef WOKWI_SIMULATION
  // Simulation mode: potentiometers via ADC
  #define PITCH_INPUT_PIN 34    // ADC1_CH6
  #define VOLUME_INPUT_PIN 35   // ADC1_CH7

  // Helper to convert ADC to distance
  int adcToDistance(int adc, int minDist, int maxDist) {
    return map(adc, 0, 4095, minDist, maxDist);
  }
#else
  // Hardware mode: VL53L0X sensors via I2C
  #include <Wire.h>
  #include "Adafruit_VL53L0X.h"

  #define SDA_PIN 21
  #define SCL_PIN 22
  #define XSHUT_PIN_1 16
  #define XSHUT_PIN_2 17
  #define SENSOR_ADDR_1 0x30
  #define SENSOR_ADDR_2 0x29

  Adafruit_VL53L0X pitchSensor = Adafruit_VL53L0X();
  Adafruit_VL53L0X volumeSensor = Adafruit_VL53L0X();
  VL53L0X_RangingMeasurementData_t pitchMeasure;
  VL53L0X_RangingMeasurementData_t volumeMeasure;
#endif

// ============================================================================
// SHARED CONFIGURATION (same for both modes)
// ============================================================================
#define BUZZER_PIN 25
#define PWM_CHANNEL 0
#define PWM_RESOLUTION 8
#define PWM_FREQUENCY 2000

#define PITCH_MIN_DIST 50
#define PITCH_MAX_DIST 400
#define VOLUME_MIN_DIST 50
#define VOLUME_MAX_DIST 300

#define MIN_FREQUENCY 100
#define MAX_FREQUENCY 2000
#define MIN_DUTY_CYCLE 0
#define MAX_DUTY_CYCLE 128

#define SAMPLES 5
int pitchReadings[SAMPLES];
int volumeReadings[SAMPLES];
int pitchIndex = 0;
int volumeIndex = 0;

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================
#ifdef WOKWI_SIMULATION
  void simulationSetup();
  int simulationReadPitch();
  int simulationReadVolume();
#else
  void hardwareSetup();
  int hardwareReadPitch();
  int hardwareReadVolume();
#endif

void sharedSetup();
void processAndPlayAudio(int pitchDistance, int volumeDistance);
int smoothReading(int readings[], int &index, int newReading);

// ============================================================================
// SETUP (Clean & Simple)
// ============================================================================
void setup() {
  Serial.begin(115200);
  delay(100);

  #ifdef WOKWI_SIMULATION
    Serial.println("\n=== ESP32 Theremin [SIMULATION] ===");
    simulationSetup();
  #else
    Serial.println("\n=== ESP32 Theremin [HARDWARE] ===");
    hardwareSetup();
  #endif

  sharedSetup();
  Serial.println("=== Ready! ===\n");
}

// ============================================================================
// MAIN LOOP (Clean & Simple)
// ============================================================================
void loop() {
  int pitchDistance, volumeDistance;

  #ifdef WOKWI_SIMULATION
    pitchDistance = simulationReadPitch();
    volumeDistance = simulationReadVolume();
  #else
    pitchDistance = hardwareReadPitch();
    volumeDistance = hardwareReadVolume();
  #endif

  processAndPlayAudio(pitchDistance, volumeDistance);
  delay(20);
}

// ============================================================================
// SIMULATION-SPECIFIC FUNCTIONS
// ============================================================================
#ifdef WOKWI_SIMULATION

void simulationSetup() {
  pinMode(PITCH_INPUT_PIN, INPUT);
  pinMode(VOLUME_INPUT_PIN, INPUT);
  Serial.println("[INIT] Analog inputs configured (GPIO34, GPIO35)");
}

int simulationReadPitch() {
  int adc = analogRead(PITCH_INPUT_PIN);
  return adcToDistance(adc, PITCH_MIN_DIST, PITCH_MAX_DIST);
}

int simulationReadVolume() {
  int adc = analogRead(VOLUME_INPUT_PIN);
  return adcToDistance(adc, VOLUME_MIN_DIST, VOLUME_MAX_DIST);
}

#endif

// ============================================================================
// HARDWARE-SPECIFIC FUNCTIONS
// ============================================================================
#ifndef WOKWI_SIMULATION

void hardwareSetup() {
  Wire.begin(SDA_PIN, SCL_PIN);
  Serial.println("[INIT] I2C initialized");

  pinMode(XSHUT_PIN_1, OUTPUT);
  pinMode(XSHUT_PIN_2, OUTPUT);
  digitalWrite(XSHUT_PIN_1, LOW);
  digitalWrite(XSHUT_PIN_2, LOW);
  delay(10);

  // Initialize pitch sensor at 0x30
  digitalWrite(XSHUT_PIN_1, HIGH);
  delay(10);
  if (!pitchSensor.begin(SENSOR_ADDR_1)) {
    Serial.println("[ERROR] Pitch sensor failed!");
    while(1);
  }
  Serial.println("[INIT] Pitch sensor initialized at 0x30");

  // Initialize volume sensor at 0x29
  digitalWrite(XSHUT_PIN_2, HIGH);
  delay(10);
  if (!volumeSensor.begin(SENSOR_ADDR_2)) {
    Serial.println("[ERROR] Volume sensor failed!");
    while(1);
  }
  Serial.println("[INIT] Volume sensor initialized at 0x29");
}

int hardwareReadPitch() {
  pitchSensor.rangingTest(&pitchMeasure, false);
  return (pitchMeasure.RangeStatus != 4) ? pitchMeasure.RangeMilliMeter : PITCH_MAX_DIST;
}

int hardwareReadVolume() {
  volumeSensor.rangingTest(&volumeMeasure, false);
  return (volumeMeasure.RangeStatus != 4) ? volumeMeasure.RangeMilliMeter : VOLUME_MAX_DIST;
}

#endif

// ============================================================================
// SHARED FUNCTIONS (used by both modes)
// ============================================================================

void sharedSetup() {
  ledcSetup(PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(BUZZER_PIN, PWM_CHANNEL);
  Serial.println("[INIT] PWM configured");

  for (int i = 0; i < SAMPLES; i++) {
    pitchReadings[i] = 0;
    volumeReadings[i] = 0;
  }
  Serial.println("[INIT] Smoothing filter initialized");
}

int smoothReading(int readings[], int &index, int newReading) {
  readings[index] = newReading;
  index = (index + 1) % SAMPLES;

  long sum = 0;
  for (int i = 0; i < SAMPLES; i++) {
    sum += readings[i];
  }
  return sum / SAMPLES;
}

void processAndPlayAudio(int pitchDistance, int volumeDistance) {
  // Apply smoothing
  int pitchSmooth = smoothReading(pitchReadings, pitchIndex, pitchDistance);
  int volumeSmooth = smoothReading(volumeReadings, volumeIndex, volumeDistance);

  // Map to audio parameters (inverse: closer = higher/louder)
  int frequency = map(pitchSmooth, PITCH_MIN_DIST, PITCH_MAX_DIST,
                      MAX_FREQUENCY, MIN_FREQUENCY);
  int dutyCycle = map(volumeSmooth, VOLUME_MIN_DIST, VOLUME_MAX_DIST,
                      MAX_DUTY_CYCLE, MIN_DUTY_CYCLE);

  // Constrain to valid ranges
  frequency = constrain(frequency, MIN_FREQUENCY, MAX_FREQUENCY);
  dutyCycle = constrain(dutyCycle, MIN_DUTY_CYCLE, MAX_DUTY_CYCLE);

  // Generate audio
  if (dutyCycle > 5) {
    ledcWriteTone(PWM_CHANNEL, frequency);
    ledcWrite(PWM_CHANNEL, dutyCycle);
  } else {
    ledcWriteTone(PWM_CHANNEL, 0);
  }

  // Debug output (throttled)
  static int loopCount = 0;
  if (loopCount++ % 10 == 0) {
    Serial.print("[PITCH] ");
    Serial.print(pitchSmooth);
    Serial.print("mm → ");
    Serial.print(frequency);
    Serial.print("Hz  |  [VOLUME] ");
    Serial.print(volumeSmooth);
    Serial.print("mm → ");
    Serial.print(dutyCycle);
    Serial.println();
  }
}
```

**Key Implementation Details**:

1. **ADC Resolution**: ESP32 has 12-bit ADC (0-4095 range)
2. **Pin Selection**: GPIO34/35 are ADC1 channels (ADC2 conflicts with WiFi)
3. **Smoothing**: 5-sample moving average reduces jitter
4. **Inverse Mapping**: Closer = higher frequency/volume (like real theremin)
5. **Silence Threshold**: Duty cycle < 5 triggers complete silence
6. **Debug Throttling**: Prints every 10th loop to keep serial readable

---

### Step 5: Build and Test Workflow

#### 5.1 Compile Firmware

```bash
# Navigate to project directory
cd /Users/fquagliati/claude/theremin

# Build for Wokwi simulation
pio run -e esp32dev-wokwi

# Expected output location:
# .pio/build/esp32dev-wokwi/firmware.bin
# .pio/build/esp32dev-wokwi/firmware.elf
```

**Troubleshooting Build Errors**:
- Missing library: Run `pio lib install`
- Syntax errors: Check conditional compilation flags
- Platform issues: Update platform with `pio platform update espressif32`

#### 5.2 Start Wokwi Simulation

**Method 1: VS Code Extension**
1. Press `F1` or `Cmd+Shift+P`
2. Type "Wokwi: Start Simulator"
3. Press Enter

**Method 2: Wokwi CLI**
```bash
wokwi-cli --interactive
```

**What Happens**:
- Wokwi reads `diagram.json` and builds virtual circuit
- Firmware from `wokwi.toml` path is loaded
- Serial monitor connects automatically
- Simulation starts running

#### 5.3 Testing Sequence

**Test 1: Verify Initialization**
```
Expected Serial Output:
=== ESP32 Theremin Wokwi Simulation ===
[INIT] Analog input pins configured
[INIT] PWM configured for buzzer
[INIT] Smoothing filter initialized (5 samples)
=== Initialization Complete ===
```

**Test 2: Pitch Control (Left Potentiometer)**
| Pot Position | Expected Behavior |
|--------------|-------------------|
| 0% (CCW) | High frequency (~2000 Hz) |
| 25% | ~1500 Hz |
| 50% (Center) | ~1050 Hz |
| 75% | ~550 Hz |
| 100% (CW) | Low frequency (~100 Hz) |

**Test 3: Volume Control (Right Potentiometer)**
| Pot Position | Expected Behavior |
|--------------|-------------------|
| 0% (CCW) | Silent (duty cycle = 0) |
| 25% | Quiet (duty cycle = 32) |
| 50% (Center) | Medium (duty cycle = 64) |
| 75% | Loud (duty cycle = 96) |
| 100% (CW) | Loudest (duty cycle = 128) |

**Test 4: Combined Control**
1. Set left pot to 50% (mid frequency)
2. Sweep right pot from 0% → 100%
3. Should hear frequency stay constant while volume increases

**Test 5: Smoothing Behavior**
1. Rapidly turn a potentiometer
2. Audio should change smoothly (not jittery)
3. Adjust `SAMPLES` constant if too sluggish or too noisy

#### 5.4 Debug Tools

**Serial Monitor**:
- Watch real-time ADC → Distance → Audio mapping
- Verify values are within expected ranges
- Check for error messages

**Wokwi Logic Analyzer**:
1. Right-click GPIO25 → "Add to Logic Analyzer"
2. Observe PWM signal characteristics:
   - Frequency changes with pitch pot
   - Duty cycle changes with volume pot

**Wokwi Oscilloscope** (if available):
- Connect to GPIO25
- Visualize audio waveform
- Verify square wave generation

---

## Expected Behavior Summary

### Potentiometer → Audio Response Table

| Left Pot (Pitch) | Right Pot (Volume) | Frequency | Duty Cycle | Result |
|------------------|-------------------|-----------|------------|--------|
| 0% | 0% | 2000 Hz | 0 | High pitch, silent |
| 0% | 100% | 2000 Hz | 128 | High pitch, loud |
| 50% | 50% | 1050 Hz | 64 | Mid pitch, medium volume |
| 100% | 0% | 100 Hz | 0 | Low pitch, silent |
| 100% | 100% | 100 Hz | 128 | Low pitch, loud |

### Audio Characteristics

**Square Wave Limitations**:
- Harsh, buzzy sound (not smooth sine wave)
- Limited dynamic range with duty cycle control
- Harmonics create "digital" sound quality

**This is expected** - passive buzzer with PWM produces square waves. For smoother audio, hardware version can use DAC + amplifier.

---

## Advantages of This Simulation Approach

| Advantage | Description |
|-----------|-------------|
| **Immediate Testing** | No waiting for hardware or sensor libraries |
| **Reproducible** | Exact potentiometer values for regression testing |
| **Core Logic Validation** | Tests frequency/volume mapping independently |
| **Easy Debugging** | Clear signal path: ADC → Distance → Audio |
| **Hardware Preservation** | VL53L0X code remains intact |
| **Fast Iteration** | Change code, rebuild, test in seconds |
| **Educational** | Visualize how distance affects sound |
| **Cost Effective** | No hardware purchase required for testing |

---

## Transitioning to Hardware

When ready to build the physical theremin:

### Step 1: Verify Hardware Code
```bash
# Build for real hardware
pio run -e esp32dev

# Upload to ESP32
pio run -e esp32dev -t upload
```

### Step 2: Hardware Differences
| Simulation | Hardware | Change Required |
|------------|----------|-----------------|
| Potentiometer ADC | VL53L0X I2C | Automatic (conditional compilation) |
| Simulated distance | Real distance measurement | None - same units (mm) |
| Audio output | Same (PWM buzzer) | None |

### Step 3: Audio Synthesis Logic
**NO CHANGES NEEDED** - The frequency/volume mapping code is identical:
```cpp
// This works in both simulation and hardware
int frequency = map(distance, MIN_DIST, MAX_DIST, MAX_FREQ, MIN_FREQ);
int dutyCycle = map(distance, MIN_DIST, MAX_DIST, MAX_DUTY, MIN_DUTY);
ledcWriteTone(PWM_CHANNEL, frequency);
ledcWrite(PWM_CHANNEL, dutyCycle);
```

### Step 4: Calibration
Hardware may require fine-tuning:
- Adjust distance ranges based on sensor positioning
- Modify frequency ranges for musical preferences
- Tune smoothing samples for responsiveness

---

## Project File Structure

After implementation, the project will contain:

```
theremin/
├── platformio.ini          # Build configurations (hardware + simulation)
├── wokwi.toml             # Wokwi simulator configuration [NEW]
├── diagram.json           # Virtual circuit diagram [NEW]
├── WOKWI_SIMULATION_PLAN.md # This document [NEW]
│
├── src/
│   └── main.cpp           # ⭐ SINGLE unified file (hardware + simulation)
│
├── include/
│   └── README
│
├── lib/
│   └── README
│
├── memory-bank/           # Project documentation
│   ├── projectbrief.md
│   ├── activeContext.md
│   ├── productContext.md
│   ├── progress.md
│   ├── systemPatterns.md
│   └── techContext.md
│
└── .pio/                  # Build artifacts (auto-generated)
    └── build/
        ├── esp32dev/              # Hardware build
        └── esp32dev-wokwi/        # Simulation build [NEW]
```

**Key Difference**: Single `main.cpp` file uses conditional compilation (`#ifdef WOKWI_SIMULATION`) to support both builds from one source.

---

## Troubleshooting Guide

### Issue: Simulation doesn't start
**Solution**:
- Verify `wokwi.toml` points to correct firmware path
- Check that firmware was successfully built
- Ensure Wokwi extension is installed and activated

### Issue: No serial output
**Solution**:
- Check `Serial.begin(115200)` in code
- Verify baud rate matches in Wokwi serial monitor
- Try closing and reopening serial monitor

### Issue: No audio output
**Solution**:
- Check GPIO25 connection in `diagram.json`
- Verify PWM setup in code (`ledcSetup`, `ledcAttachPin`)
- Increase buzzer volume attribute in `diagram.json`

### Issue: Jittery audio
**Solution**:
- Increase `SAMPLES` constant (e.g., 10 instead of 5)
- Add delay at end of loop (e.g., `delay(30)`)
- Check for serial output spam (reduces performance)

### Issue: Potentiometer has no effect
**Solution**:
- Verify connections in `diagram.json` (SIG → GPIO, VCC → 3V3, GND → GND)
- Check correct GPIO pins in code (34, 35)
- Test with `Serial.println(analogRead(34));` to verify ADC works

---

## Next Steps

### Phase 1: Setup (Immediate)
- [ ] Create `wokwi.toml` in project root
- [ ] Create `diagram.json` in project root
- [ ] Refactor `src/main.cpp` with clean architecture and conditional compilation
- [ ] Update `platformio.ini` with `esp32dev-wokwi` environment

### Phase 2: Testing (Same Day)
- [ ] Build firmware: `pio run -e esp32dev-wokwi`
- [ ] Start Wokwi simulation
- [ ] Verify initialization via serial monitor
- [ ] Test pitch control (left potentiometer)
- [ ] Test volume control (right potentiometer)
- [ ] Test combined control

### Phase 3: Validation (Same Day)
- [ ] Document test results
- [ ] Verify frequency ranges (100-2000 Hz)
- [ ] Verify volume control (duty cycle 0-128)
- [ ] Check smoothing behavior
- [ ] Measure response latency (should be <50ms)

### Phase 4: Hardware Transition (When Ready)
- [ ] Order/acquire hardware components
- [ ] Assemble physical circuit
- [ ] Build and upload hardware firmware: `pio run -e esp32dev -t upload`
- [ ] Test with real VL53L0X sensors
- [ ] Compare simulation vs. hardware behavior
- [ ] Fine-tune calibration if needed

---

## Success Criteria

Simulation is successful when:
- ✅ Both potentiometers control independent parameters
- ✅ Frequency changes continuously (100-2000 Hz range)
- ✅ Volume control works (silent to loud)
- ✅ Response is smooth (no jitter or latency)
- ✅ Serial output shows correct ADC → Distance → Audio mapping
- ✅ Audio synthesis logic is validated

---

## References

### Wokwi Documentation
- ESP32 Simulation: https://docs.wokwi.com/guides/esp32
- PlatformIO Integration: https://docs.wokwi.com/vscode/getting-started
- diagram.json Format: https://docs.wokwi.com/diagram-format
- Project Configuration: https://docs.wokwi.com/vscode/project-config

### ESP32 Resources
- PWM/LEDC Documentation: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/ledc.html
- ADC Documentation: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/adc.html

### Project Documentation
- Project Brief: `memory-bank/projectbrief.md`
- System Patterns: `memory-bank/systemPatterns.md`
- Technical Context: `memory-bank/techContext.md`

---

**Document Version**: 1.0
**Last Updated**: October 15, 2025
**Status**: Ready for Implementation
