# ESP32 Theremin - Modular Architecture

## Overview

The ESP32 Theremin has been refactored from a single monolithic file into a clean, modular, object-oriented architecture. This makes the code more maintainable, testable, and extensible for future features.

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────┐
│                      main.cpp                           │
│                   (Entry Point)                         │
│  - Creates Theremin instance                            │
│  - Calls begin() and update()                           │
└────────────────────┬────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────┐
│                  Theremin.h/cpp                         │
│              (Main Coordinator)                         │
│  - Manages SensorManager and AudioEngine                │
│  - Maps sensor distances to audio parameters            │
│  - Handles debug output                                 │
└────────┬────────────────────────────────────┬───────────┘
         │                                    │
         ▼                                    ▼
┌────────────────────────┐    ┌─────────────────────────┐
│  SensorManager.h/cpp   │    │   AudioEngine.h/cpp     │
│   (Input Handling)     │    │   (Audio Synthesis)     │
├────────────────────────┤    ├─────────────────────────┤
│ - getPitchDistance()   │    │ - setFrequency()        │
│ - getVolumeDistance()  │    │ - setAmplitude()        │
│ - Smoothing filter     │    │ - update()              │
│                        │    │ - PWM generation        │
│ Supports:              │    │                         │
│ ✓ Simulation (ADC)     │    │ Future extensions:      │
│ ✓ Hardware (VL53L0X)   │    │ • DAC output            │
│                        │    │ • Waveform generators   │
└────────────────────────┘    │ • Multiple oscillators  │
                              └─────────────────────────┘
```

## File Structure

```
theremin/
├── include/
│   ├── SensorManager.h      # Sensor input abstraction
│   ├── AudioEngine.h         # Audio synthesis abstraction
│   └── Theremin.h            # Main coordinator
│
├── src/
│   ├── main.cpp              # Entry point (40 lines)
│   ├── SensorManager.cpp     # Sensor implementation
│   ├── AudioEngine.cpp       # Audio implementation
│   └── Theremin.cpp          # Coordinator implementation
│
└── memory-bank/              # Project documentation
```

## Class Responsibilities

### SensorManager
**Purpose:** Abstract sensor input (simulation or hardware)

**Public Interface:**
- `bool begin()` - Initialize sensors
- `int getPitchDistance()` - Get smoothed pitch sensor reading (mm)
- `int getVolumeDistance()` - Get smoothed volume sensor reading (mm)

**Features:**
- Conditional compilation for simulation vs hardware
- Built-in smoothing filter (5-sample moving average)
- Consistent API regardless of sensor type

**Future Extensions:**
- Different smoothing algorithms
- Calibration routines
- Multiple sensor types

### AudioEngine
**Purpose:** Generate audio output

**Public Interface:**
- `void begin()` - Initialize audio hardware
- `void setFrequency(int freq)` - Set frequency (100-2000 Hz)
- `void setAmplitude(int amplitude)` - Set amplitude (0-100%)
- `void update()` - Apply settings to hardware

**Current Implementation:**
- PWM-based square wave generation
- 8-bit resolution, GPIO25 output

**Future Extensions:**
- DAC output for better audio quality
- Waveform selection (sine, triangle, sawtooth)
- Multiple oscillators with mixing
- Effects (vibrato, tremolo, reverb)
- ADSR envelope

### Theremin
**Purpose:** Coordinate sensors and audio

**Public Interface:**
- `bool begin()` - Initialize all subsystems
- `void update()` - Main update loop
- `void setDebugMode(bool enabled)` - Toggle debug output

**Responsibilities:**
- Create and manage SensorManager and AudioEngine instances
- Map sensor distances to audio parameters
- Handle debug logging
- Provide clean API for main.cpp

**Future Extensions:**
- Preset management
- UI handling (buttons/switches)
- Display management
- Configuration system

## Benefits of New Architecture

### 1. Separation of Concerns
Each class has a single, clear responsibility:
- SensorManager → Input
- AudioEngine → Output
- Theremin → Coordination

### 2. Testability
Classes can be tested independently:
```cpp
// Test SensorManager in isolation
SensorManager sensors;
sensors.begin();
int distance = sensors.getPitchDistance();
```

### 3. Extensibility
Adding features requires minimal changes to existing code:
```cpp
// Add display support
class Theremin {
private:
    SensorManager sensors;
    AudioEngine audio;
    Display display;  // ← Just add new component
};
```

### 4. Maintainability
- Easy to find code: sensor bugs → check SensorManager.cpp
- Clear dependencies: include files show relationships
- Self-documenting: class names describe purpose

### 5. Reusability
Classes can be used in other projects:
- SensorManager → Any VL53L0X-based project
- AudioEngine → Any ESP32 audio project

## Code Size Comparison

### Before (Monolithic)
- `main.cpp`: 250+ lines, everything in one file

### After (Modular)
- `main.cpp`: 40 lines (clean entry point)
- `SensorManager.*`: ~130 lines (sensor logic)
- `AudioEngine.*`: ~80 lines (audio logic)
- `Theremin.*`: ~90 lines (coordination)

**Total:** Similar line count, but organized and documented

## Adding New Features

### Example: Adding Waveform Selection

**1. Create WaveformGenerator base class:**
```cpp
// include/WaveformGenerator.h
class WaveformGenerator {
public:
    virtual float getSample(float phase) = 0;
    virtual const char* getName() = 0;
};
```

**2. Implement specific waveforms:**
```cpp
class SineWave : public WaveformGenerator {
    float getSample(float phase) override {
        return sin(2.0 * PI * phase);
    }
    const char* getName() override { return "Sine"; }
};
```

**3. Extend AudioEngine:**
```cpp
// Add to AudioEngine.h
void setWaveform(WaveformGenerator* wave);

// Minimal changes to existing code!
```

### Example: Adding OLED Display

**1. Create Display class:**
```cpp
// include/Display.h
class Display {
public:
    void begin();
    void showFrequency(int freq);
    void showWaveform(const char* name);
};
```

**2. Add to Theremin:**
```cpp
// Theremin.h
private:
    Display display;  // ← One line

// Theremin.cpp - begin()
display.begin();  // ← One line

// Theremin.cpp - update()
display.showFrequency(frequency);  // ← One line
```

**No changes needed** to SensorManager or AudioEngine!

## Migration Summary

### What Changed
✅ All sensor logic extracted to SensorManager class
✅ All audio logic extracted to AudioEngine class
✅ Coordination logic moved to Theremin class
✅ main.cpp simplified to ~40 lines
✅ Proper separation of concerns

### What Stayed the Same
✅ Functionality identical to original code
✅ Still supports both simulation and hardware
✅ Same PWM audio generation
✅ Same sensor smoothing algorithm
✅ Same debug output format

### Build Status
✅ **Compiles successfully** for simulation (`esp32dev-wokwi`)
✅ Ready to build for hardware (`esp32dev`)

## Next Steps

1. **Test in Wokwi simulator** - Verify behavior matches original
2. **Test on hardware** - When components arrive
3. **Add future features** using new modular structure:
   - Display class for OLED
   - WaveformGenerator for different sounds
   - UserInterface class for buttons/switches
   - Preset system for saving configurations

## Conclusion

The refactored architecture provides a solid foundation for future expansion while maintaining all existing functionality. Adding features like DAC audio, waveform selection, OLED display, and UI controls will now be straightforward additions rather than complex modifications to monolithic code.
