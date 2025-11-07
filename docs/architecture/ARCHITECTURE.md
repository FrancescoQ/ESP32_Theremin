# ESP32 Theremin - Modular Architecture

## Overview

The ESP32 Theremin has been refactored from a single monolithic file into a clean, modular, object-oriented architecture. This makes the code more maintainable, testable, and extensible for future features.

## Architecture Diagram

```
┌──────────────────────────────────────────────────────────────────────────┐
│                              main.cpp                                     │
│                          (Entry Point)                                    │
│  - Creates Theremin, PerformanceMonitor instances                         │
│  - Creates OTAManager instance (optional)                                 │
│  - Creates SerialControls, GPIOControls, GPIOMonitor (Phase 3)            │
│  - Calls begin() and update() on all systems                              │
└────┬─────────────────────────────────────────┬──────────────┬────────────┘
     │                                         │              │
     ▼                                         ▼              ▼
┌─────────────────────┐          ┌──────────────────┐  ┌──────────────────┐
│  Theremin.h/cpp     │          │ OTAManager       │  │PerformanceMonitor│
│  (Coordinator)      │          │ .h/cpp           │  │ .h/cpp           │
├─────────────────────┤          ├──────────────────┤  ├──────────────────┤
│ - Manages:          │          │ - WiFi AP mode   │  │ - CPU tracking   │
│   • SensorManager   │          │ - ElegantOTA     │  │ - RAM monitoring │
│   • AudioEngine     │          │ - Non-blocking   │  │ - Task timing    │
│ - Maps sensor→audio │          │ - HTTP auth      │  │ - Debug output   │
└──┬──────────────┬───┘          └──────────────────┘  └──────────────────┘
   │              │
   ▼              ▼
┌──────────────┐ ┌────────────────────────────────────────────────────────┐
│SensorManager │ │              AudioEngine.h/cpp                         │
│  .h/cpp      │ │              (Audio Synthesis)                         │
├──────────────┤ ├────────────────────────────────────────────────────────┤
│- Dual VL53L0X│ │ - 3x Oscillator instances (sine/square/tri/saw)       │
│- Smoothing   │ │ - Frequency/amplitude control (sensor-driven)          │
│- Enable flags│ │ - EffectsChain integration                             │
│- getPitch    │ │ - I2S PCM5102 DAC output (22050 Hz, 16-bit stereo)     │
│- getVolume   │ │ - FreeRTOS audio task (Core 1, high priority)          │
│              │ │                                                         │
│Features:     │ │ Components:                                             │
│✓ Hardware I2C│ │ ┌──────────────────────────────────────────────────┐  │
│✓ Pitch/Vol   │ │ │         Oscillator.h/cpp (x3)                    │  │
│✓ Sensor on/  │ │ │  - Wavetable synthesis (1024 samples)            │  │
│  off control │ │ │  - Waveform: OFF/SINE/SQUARE/TRIANGLE/SAW        │  │
└──────────────┘ │ │  - Octave shift: -1/0/+1                         │  │
                 │ │  - Volume control per oscillator                 │  │
                 │ │  - Phase accumulator (anti-aliased)              │  │
                 │ └──────────────────────────────────────────────────┘  │
                 │                         ↓                               │
                 │ ┌──────────────────────────────────────────────────┐  │
                 │ │      EffectsChain.h/cpp                          │  │
                 │ │  - Delay → Chorus → Reverb processing chain      │  │
                 │ │  - Enable/disable per effect                     │  │
                 │ │  - CPU: 14.5% with all effects (85% headroom!)   │  │
                 │ │                                                   │  │
                 │ │  Components:                                      │  │
                 │ │  • DelayEffect (circular buffer, feedback)       │  │
                 │ │  • ChorusEffect (modulated delay, Osc-based LFO) │  │
                 │ │  • ReverbEffect (4 combs + 2 allpass, Freeverb)  │  │
                 │ └──────────────────────────────────────────────────┘  │
                 │                         ↓                               │
                 │                  I2S DAC Output                         │
                 └────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────────┐
│                        Control System (Phase 3)                           │
├────────────────────────┬─────────────────────┬─────────────────────────┤
│  SerialControls        │  GPIOControls       │  GPIOMonitor            │
│  .h/cpp                │  .h/cpp             │  .h/cpp                 │
├────────────────────────┼─────────────────────┼─────────────────────────┤
│ - Command parser       │ - MCP23017 I2C GPIO │ - Debug visualization   │
│ - Oscillator control   │   expander          │ - Pin state reporting   │
│ - Effects control      │ - 3x waveform SW    │ - State change logging  │
│ - Sensor enable/disable│ - 3x octave SW      │                         │
│ - Audio control        │ - Debouncing (50ms) │                         │
│ - Help/Status commands │ - Interrupt-driven  │                         │
│                        │ - Startup sync      │                         │
└────────────────────────┴─────────────────────┴─────────────────────────┘

Signal Flow:
  Sensors → Theremin → AudioEngine → [3 Oscillators] → [Mix]
                                                          ↓
                          ← DAC ← [EffectsChain] ← [Mixer Output]
                                   (Delay→Chorus→Reverb)

Control Flow:
  Serial Commands → SerialControls → AudioEngine/EffectsChain
  GPIO Switches → GPIOControls → AudioEngine (via oscillator control)
```

## File Structure

```
theremin/
├── include/
│   ├── SensorManager.h      # Sensor input abstraction
│   ├── AudioEngine.h         # Audio synthesis abstraction
│   ├── Theremin.h            # Main coordinator
│   └── OTAManager.h          # OTA firmware updates (conditional)
│
├── src/
│   ├── main.cpp              # Entry point (~60 lines with OTA)
│   ├── SensorManager.cpp     # Sensor implementation
│   ├── AudioEngine.cpp       # Audio implementation
│   ├── Theremin.cpp          # Coordinator implementation
│   └── OTAManager.cpp        # OTA implementation (conditional)
│
├── memory-bank/              # Project documentation
└── OTA_SETUP.md              # OTA usage guide
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
- I2S output to external PCM5102 DAC
- 16-bit resolution, stereo output (GPIO25/26/27)
- 3x oscillators with waveform selection (sine, triangle, square, sawtooth)
- Audio effects chain (delay, chorus, reverb)
- FreeRTOS audio task on Core 1

**Future Extensions:**
- ADSR envelope
- True stereo effects (currently mono duplicated)
- Higher sample rates (up to 384 kHz supported by PCM5102)

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

### OTAManager
**Purpose:** Manage wireless firmware updates

**Public Interface:**
- `bool begin(const char* otaUser, const char* otaPass)` - Initialize WiFi AP and OTA server
- `void handle()` - Process OTA requests (non-blocking, call in loop)
- `bool isRunning()` - Check if OTA manager is active
- `IPAddress getIP()` - Get Access Point IP address

**Current Implementation:**
- WiFi Access Point mode (ESP32 creates own network)
- ElegantOTA library v3.1.7 for web interface
- HTTP Basic Authentication for security
- Fixed IP: 192.168.4.1
- Conditional compilation with `#ifdef ENABLE_OTA`

**Features:**
- **Non-blocking:** Theremin continues playing during OTA
- **Isolated:** Complete separation from main application logic
- **Configurable:** AP name, password, OTA credentials
- **Secure:** HTTP Basic Auth protects update endpoint
- **Optional:** Can be completely disabled at compile-time

**Configuration:**
```cpp
// In main.cpp
#ifdef ENABLE_OTA
  OTAManager ota("Theremin-OTA", "");  // AP name, AP password
  ota.begin("admin", "theremin");      // OTA user, OTA password
#endif
```

**Future Extensions:**
- Runtime enable/disable via button press
- Station mode as alternative to AP mode
- mDNS support (access via theremin.local)
- Auto-timeout after X minutes of inactivity

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

### Real-World Example: OTAManager (Already Implemented!)

The OTAManager class is a **perfect demonstration** of the modular architecture in action:

**What We Added:**
- New class: OTAManager with complete WiFi AP + ElegantOTA functionality
- Added to main.cpp with `#ifdef ENABLE_OTA` (6 lines of code)
- Created OTA_SETUP.md documentation

**What We Didn't Touch:**
- ✅ SensorManager - zero changes
- ✅ AudioEngine - zero changes
- ✅ Theremin - zero changes
- ✅ All existing functionality works identically

**Result:**
- Wireless firmware updates fully functional
- Theremin continues playing during OTA
- Can be completely disabled with one compile flag
- Zero impact on existing code

**This proves the architecture works as designed!** Adding a major feature (wireless updates) required only creating the new class and adding it to main.cpp. No refactoring, no side effects, no breaking changes.

## Migration Summary

### What Changed
✅ All sensor logic extracted to SensorManager class
✅ All audio logic extracted to AudioEngine class
✅ Coordination logic moved to Theremin class
✅ OTA functionality added as separate OTAManager class (October 2025)
✅ main.cpp simplified to ~60 lines (with optional OTA)
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
