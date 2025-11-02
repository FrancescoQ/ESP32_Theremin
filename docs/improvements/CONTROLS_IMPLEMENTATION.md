# Controls System Implementation

**Implementation Date:** October 29 - November 2, 2025
**Phase:** Phase 3 (Controls)
**Status:** ✅ Complete

## Overview

Successfully implemented a comprehensive control system for the ESP32 Theremin with **dual input methods**:
- **Serial Commands**: Text-based control via Serial Monitor (debugging/testing)
- **GPIO Hardware**: Physical switches via MCP23017 I2C GPIO expander

**Architecture Achievement:** Clean separation of control inputs from audio engine, allowing multiple control sources without conflicts.

---

## Problem Statement

The Phase 2 theremin had:
- Fixed oscillator configurations (hard-coded in setup)
- No runtime parameter adjustment
- No way to test different settings without recompiling
- No physical controls for live performance

**Goals:**
1. **Serial command interface** for testing and debugging
2. **Hardware switch control** for live performance
3. **Runtime parameter changes** without affecting audio quality
4. **Thread-safe control** of audio engine from main loop
5. **Modular architecture** supporting multiple input sources

---

## Solution Architecture

### Design Principles

1. **Separation of Concerns**
   - SerialControls handles serial command parsing
   - GPIOControls handles MCP23017 switch reading
   - Both use the same AudioEngine API (unified control path)
   - Theremin class coordinates but doesn't parse commands

2. **Thread Safety**
   - Audio engine runs on Core 1 (FreeRTOS task)
   - Controls read on Core 0 (main loop)
   - All parameter changes use mutex protection
   - AudioEngine provides thread-safe setters

3. **Architectural Elevation**
   - Controls are siblings of Theremin (not children)
   - Both SerialControls and GPIOControls live in main.cpp
   - Clean dependency: Controls → Theremin → AudioEngine
   - No circular dependencies

4. **"Last Wins" Behavior**
   - Serial and GPIO can both control oscillators
   - Whichever is used most recently takes precedence
   - No complex arbitration logic needed
   - Useful for debugging: override hardware with serial commands

---

## Implementation Timeline

### Phase A: AudioEngine Control Methods (Oct 29, 2025) ✅

**Goal:** Add thread-safe API to AudioEngine for runtime control

**Implementation:**
- Added methods to AudioEngine.h:
  - `setOscillatorWaveform(oscNum, waveform)`
  - `setOscillatorOctave(oscNum, octave)`
  - `setOscillatorVolume(oscNum, volume)`
- All use paramMutex for thread safety
- Validation of input parameters (oscillator number, ranges)

**Testing:** Called methods directly from setup() to verify functionality

**Build Impact:** +796 bytes Flash, no RAM increase

---

### Phase B: Serial Command Parser (Oct 30, 2025) ✅

**Goal:** Create ControlHandler class with serial command parsing

**Initial Implementation:**
- Class: `ControlHandler` (later renamed to SerialControls)
- Command format: `oscN:parameter:value`
- Examples:
  - `osc1:sine` - Set oscillator 1 to sine wave
  - `osc2:octave:-1` - Shift oscillator 2 down one octave
  - `osc3:vol:0.5` - Set oscillator 3 to 50% volume

**Features:**
- Case-insensitive parsing
- Support for abbreviated commands (tri, saw, oct, vol)
- Error handling with helpful messages
- Non-blocking serial reading

**Build Impact:** +9,440 bytes Flash for command system

---

### Phase C: Extended Serial Commands (Oct 30, 2025) ✅

**Goal:** Add help, status, and batch commands

**Commands Added:**
- `help` or `?` - Print complete command reference
- `status` - Show all oscillator states
- `status:osc1` - Show specific oscillator
- Batch commands: `osc1:sine;osc1:octave:1;osc1:vol:0.8`

**Enhancement:** Status Introspection
- Added getter methods to AudioEngine and Oscillator
- `getOscillatorWaveform()`, `getOscillatorOctave()`, `getOscillatorVolume()`
- Thread-safe getters using mutex (consistent with setters)
- Real-time state display instead of placeholders

**Build Impact:** +2,736 bytes Flash total

---

### Phase C2: Audio & Sensor Control Commands (Oct 30, 2025) ✅

**Goal:** Add direct audio control and sensor enable/disable

**New Commands:**

**Audio Control:**
- `audio:freq:440` - Set frequency to 440 Hz (manual pitch)
- `audio:amp:75` - Set amplitude to 75% (manual volume)
- `audio:status` - Show current audio values

**Sensor Control:**
- `sensors:pitch:on/off` - Enable/disable pitch sensor
- `sensors:volume:on/off` - Enable/disable volume sensor
- `sensors:enable` - Enable both (batch alias)
- `sensors:disable` - Disable both (batch alias)
- `sensors:status` - Show sensor enable states

**Architecture:**
- Sensor enable flags stored in SensorManager
- Theremin::update() checks flags before applying sensor values
- Manual audio commands persist when sensors disabled
- Use case: Test fixed frequencies, individual sensor testing

**Refactor:** Changed ControlHandler constructor to accept Theremin* instead of AudioEngine* for full system access

---

### Phase D: GPIO Controls with MCP23017 (Nov 2, 2025) ✅

**Goal:** Add hardware switch control via I2C GPIO expander

**Hardware:**
- MCP23017 I2C GPIO expander (address 0x20)
- 16 GPIO pins for 3 oscillators (5 pins each: 3 waveform + 2 octave)
- INPUT_PULLUP configuration (active LOW switches)

**Class:** `GPIOControls` (include/GPIOControls.h + src/GPIOControls.cpp)

**Pin Mappings:**
```
OSC1: Waveform pins 6,5,14 | Octave pins 7,15
OSC2: Waveform pins 4,11,3 | Octave pins 12,13
OSC3: Waveform pins 1,9,0  | Octave pins 10,2
```

**Switch Logic:**
- **3-pin waveform**: SINE/TRIANGLE/SQUARE (all HIGH = OFF)
  - Maps directly to physical switch positions
  - 1 pin per waveform type
- **2-pin octave**: +1/-1 (both LOW = 0)
  - Binary encoding: 00=normal, 01=+1, 10=-1, 11=normal

**Features:**
- Debouncing: 50ms delay prevents switch bounce
- State tracking per oscillator (detect changes only)
- Graceful degradation if MCP23017 not found
- Debug output shows switch state changes

**Architecture Decision:** Stack allocation (RAII pattern)
- GPIOControls created as direct member in main.cpp
- Consistent with effects architecture
- No heap fragmentation over long runtime

**Build Impact:** ~30KB for GPIO controls + MCP23017 library

---

### Phase E: Polish & Integration (Nov 2, 2025) ✅

**Goal:** Fix startup bug and clean up architecture

**Startup Sync Issue:**
- **Problem:** Oscillators initialized with default waveforms in AudioEngine constructor, but physical switches might be in different positions at startup
- **Result:** Audio didn't match switch positions until first switch change
- **Solution:** Read initial switch positions in GPIOControls::begin() and apply to oscillators before first loop

**Architecture Refactor: ControlHandler → SerialControls**
- **Original design:** ControlHandler contained both serial and GPIO
- **Problem:** Growing into monolithic control class
- **Solution:** Split into two sibling classes
  - `SerialControls` - handles serial commands only
  - `GPIOControls` - handles MCP23017 switches only
  - Both elevated to main.cpp (siblings of Theremin, not children)

**Benefits:**
- Clear separation of serial vs hardware control
- Each class has single responsibility
- Easy to test independently
- Matches architectural pattern: Controls → Theremin → AudioEngine

**Additional Features:**
- GPIOMonitor class for debug visualization
- Enable/disable features for audio and sensors
- Complete status reporting commands

---

## Serial Command Reference

### Oscillator Control
```
osc1:sine              # Set oscillator 1 to sine wave
osc1:square            # Set oscillator 1 to square wave
osc1:triangle          # Set oscillator 1 to triangle wave (or 'tri')
osc1:sawtooth          # Set oscillator 1 to sawtooth wave (or 'saw')
osc1:off               # Turn off oscillator 1

osc1:octave:-1         # Shift oscillator 1 down one octave (or 'oct')
osc1:octave:0          # Reset oscillator 1 to base octave
osc1:octave:1          # Shift oscillator 1 up one octave

osc1:vol:0.0           # Set oscillator 1 to 0% volume (silent)
osc1:vol:0.5           # Set oscillator 1 to 50% volume
osc1:vol:1.0           # Set oscillator 1 to 100% volume
```

### Audio & Sensor Control
```
audio:freq:440         # Set frequency to 440 Hz (manual pitch)
audio:amp:75           # Set amplitude to 75% (manual volume)
audio:status           # Show current audio values

sensors:pitch:on       # Enable pitch sensor
sensors:pitch:off      # Disable pitch sensor
sensors:volume:on      # Enable volume sensor
sensors:volume:off     # Disable volume sensor
sensors:enable         # Enable both sensors (batch alias)
sensors:disable        # Disable both sensors (batch alias)
sensors:status         # Show sensor enable states

sensors:volume:smooth:on   # Enable volume smoothing
sensors:volume:smooth:off  # Disable (instant response for reverb testing)
sensors:pitch:smooth:on    # Enable pitch smoothing
sensors:pitch:smooth:off   # Disable pitch smoothing
```

### Effects Control
```
delay:on               # Enable delay effect
delay:off              # Disable delay effect
delay:time:300         # Set delay time to 300ms
delay:feedback:0.5     # Set feedback to 50%
delay:mix:0.3          # Set wet/dry mix to 30%

chorus:on              # Enable chorus effect
chorus:off             # Disable chorus effect
chorus:rate:2.0        # Set LFO rate to 2.0 Hz
chorus:depth:15        # Set modulation depth to 15ms
chorus:mix:0.4         # Set wet/dry mix to 40%

reverb:on              # Enable reverb effect
reverb:off             # Disable reverb effect
reverb:room:0.5        # Set room size (0.0-1.0)
reverb:damp:0.5        # Set damping (0.0-1.0)
reverb:mix:0.3         # Set wet/dry mix to 30%

effects:status         # Show all effect states
effects:reset          # Clear all effect buffers
```

### Status & Help
```
help                   # Show all commands (or '?')
status                 # Show all oscillator states
status:osc1            # Show oscillator 1 state
```

### Batch Commands
```
osc1:sine;osc1:octave:1;osc1:vol:0.8   # Multiple commands at once
sensors:disable;audio:freq:440          # Test fixed frequency
```

---

## GPIO Hardware Reference

### MCP23017 Configuration

**I2C Address:** 0x20
**Update Rate:** Event-driven (interrupt on pin change)
**Debounce:** 50ms software debounce

### Switch Wiring

**Waveform Switches (3-pin, 1 per waveform):**
```
Pin → Switch → GND (when closed)

OSC1 Waveform:
  Pin 6  (GPA6) → SINE switch
  Pin 5  (GPA5) → TRIANGLE switch
  Pin 14 (GPB6) → SQUARE switch
  All open = OFF
```

**Octave Switches (2-pin, binary encoding):**
```
OSC1 Octave:
  Pin 7  (GPA7) → Bit 0
  Pin 15 (GPB7) → Bit 1

Combinations:
  Both open (11)  = Normal (0)
  Bit0 closed (10) = Down (-1)
  Bit1 closed (01) = Up (+1)
  Both closed (00) = Normal (0)
```

### Pin Assignment Summary

| Oscillator | Waveform Pins | Octave Pins |
|------------|---------------|-------------|
| OSC1 | 6 (SINE), 5 (TRI), 14 (SQ) | 7 (bit0), 15 (bit1) |
| OSC2 | 4 (SINE), 11 (TRI), 3 (SQ) | 12 (bit0), 13 (bit1) |
| OSC3 | 1 (SINE), 9 (TRI), 0 (SQ) | 10 (bit0), 2 (bit1) |

---

## Architecture Patterns

### Thread Safety Pattern

**Problem:** Audio task runs on Core 1, controls on Core 0

**Solution:** Mutex-protected setters in AudioEngine
```cpp
void AudioEngine::setOscillatorWaveform(int oscNum, Waveform wf) {
    if (paramMutex && xSemaphoreTake(paramMutex, portMAX_DELAY)) {
        // Safe to modify parameters
        oscillator[oscNum].setWaveform(wf);
        xSemaphoreGive(paramMutex);
    }
}
```

**Result:** Zero audio glitches from parameter changes

---

### Forward Declaration Pattern

**Problem:** Circular dependencies (SerialControls needs Theremin, Theremin includes SerialControls)

**Solution:** Forward declarations in headers
```cpp
// In SerialControls.h
class Theremin;  // Forward declaration

class SerialControls {
    Theremin* theremin;  // Pointer, not include
};
```

**Result:** Clean compilation, no circular includes

---

### Stack Allocation (RAII)

**Pattern:** Direct members instead of pointers
```cpp
// In main.cpp
SerialControls serialControls(&theremin);  // Stack allocation
GPIOControls gpioControls(&theremin);      // Stack allocation
```

**Benefits:**
- Automatic cleanup (no delete needed)
- No heap fragmentation
- Consistent with effects architecture
- Perfect for embedded systems

---

## Performance Impact

### CPU Usage
- **Serial Commands:** <0.1% (only when parsing)
- **GPIO Reading:** ~1-2% (with 10ms throttle)
- **Total Control Overhead:** <3% CPU

### Memory Usage
- **RAM:** +144 bytes (GPIO state tracking)
- **Flash:** ~40KB total (command parser + MCP23017 library + GPIO controls)

### Latency
- **Serial Command:** Immediate (next loop iteration)
- **GPIO Switch:** ~50-60ms (debounce + I2C read)
- **Audio Response:** Seamless (mutex prevents glitches)

---

## Testing & Validation

### Serial Command Testing
- ✅ All commands execute without audio glitches
- ✅ Thread-safe parameter updates via AudioEngine API
- ✅ Serial commands work alongside theremin sensor control
- ✅ Invalid commands provide clear error messages

### GPIO Hardware Testing
- ✅ MCP23017 initializes correctly via I2C
- ✅ All 16 GPIO pins configured as INPUT_PULLUP
- ✅ Moving waveform switches changes oscillator waveforms
- ✅ Moving octave switches changes oscillator octaves
- ✅ Debouncing prevents switch bounce glitches
- ✅ Startup sync - oscillators match switch positions at boot
- ✅ No audio glitches during rapid switch changes

### Integration Testing
- ✅ Serial and GPIO can both control oscillators ("last wins")
- ✅ No I2C conflicts between sensors, MCP23017, display
- ✅ All Phase 1-4 features still functional
- ✅ No crashes or hangs

---

## Related Files

### Control Classes
- `include/SerialControls.h` + `src/SerialControls.cpp` (~200 lines)
- `include/GPIOControls.h` + `src/GPIOControls.cpp` (~180 lines)
- `include/GPIOMonitor.h` + `src/GPIOMonitor.cpp` (debug utility)

### Audio Engine Integration
- `include/AudioEngine.h` + `src/AudioEngine.cpp` (control methods)
- `include/Oscillator.h` + `src/Oscillator.cpp` (getters added)

### Hardware Configuration
- `include/PinConfig.h` (MCP23017 pin mappings)

### Coordinator
- `src/main.cpp` (SerialControls + GPIOControls + Theremin)
- `include/Theremin.h` + `src/Theremin.cpp` (getSensorManager getter)

### Documentation
- `/CONTROLS_IMPLEMENTATION_PLAN.md` (detailed roadmap with all phases)
- `memory-bank/activeContext.md` (controls patterns)
- `memory-bank/progress.md` (Phase 3 tracking)

---

## Design Insights

### What Worked Well

1. **Incremental Development**
   - Started with AudioEngine API (Phase A)
   - Added serial commands for testing (Phase B)
   - Extended with advanced features (Phase C/C2)
   - Finally added hardware (Phase D)
   - **Benefit:** Each phase validated before proceeding

2. **Serial-First Approach**
   - Serial commands implemented before hardware
   - Allowed thorough testing of AudioEngine API
   - Hardware implementation just mapped switches to existing commands
   - **Benefit:** Hardware bugs easy to isolate

3. **Unified Control Path**
   - Both SerialControls and GPIOControls use same AudioEngine methods
   - "Last wins" behavior naturally emerged
   - No complex arbitration needed
   - **Benefit:** Simple mental model, easy debugging

4. **Architectural Elevation**
   - Controls as siblings of Theremin (not children)
   - Clean separation of concerns
   - No circular dependencies
   - **Benefit:** Maintainable, testable, extensible

### Technical Discoveries

1. **Startup Sync Critical**
   - Must read GPIO switches during begin(), not just in loop
   - Otherwise oscillators don't match physical state at boot
   - **Solution:** GPIOControls::begin() reads and applies initial state

2. **MCP23017 Graceful Degradation**
   - System continues if MCP23017 not found
   - Serial commands still work (fallback mode)
   - **Benefit:** Development without hardware, easier debugging

3. **Debouncing Essential**
   - 50ms debounce prevents switch bounce artifacts
   - Time-based (millis()) works better than count-based
   - **Pattern:** Track lastChangeTime per oscillator

### Lessons Learned

1. **Forward Declarations Resolve Circular Dependencies**
   - Use forward declarations in headers
   - Include full headers only in .cpp files
   - **Result:** Clean compilation, no hacky workarounds

2. **Stack Allocation for Long-Running Systems**
   - RAII pattern prevents memory leaks
   - No heap fragmentation over time
   - Consistent with effects architecture
   - **Result:** Stable embedded system

3. **Mutex Protection is Non-Negotiable**
   - Audio glitches without mutex (race conditions)
   - Small performance cost (~1-2%)
   - **Conclusion:** Always protect shared state in multi-core systems

---

## Future Enhancements

### Potential Additions

1. **Preset System**
   - Save/load oscillator + effect configurations
   - Store in SPIFFS or EEPROM
   - GPIO button for preset switching

2. **MIDI Control**
   - Map serial commands to MIDI CC messages
   - Use theremin as MIDI controller
   - MCP23017 has spare pins for MIDI jack

3. **Display Integration**
   - Show current oscillator settings on OLED
   - Visual feedback for switch changes
   - CPU/RAM metrics

4. **Additional Switch Types**
   - Momentary buttons for effects toggle
   - Rotary encoders for continuous parameters
   - Expression pedal input (ADC)

5. **Web Interface**
   - Control via WiFi (like OTA manager)
   - Parameter tweaking from phone/tablet
   - Preset management

---

## Conclusion

**Phase 3 Complete:** Comprehensive control system with serial and GPIO inputs!

**Key Achievements:**
- ✅ Serial command interface for all features
- ✅ Hardware switch control via MCP23017
- ✅ Thread-safe parameter updates (zero glitches)
- ✅ Modular architecture (controls as siblings)
- ✅ Startup sync - oscillators match switches at boot
- ✅ Clean separation: SerialControls, GPIOControls, Theremin

**Architecture Success:**
- Clean dependency flow: Controls → Theremin → AudioEngine
- No circular dependencies
- Stack allocation (RAII) prevents leaks
- Easy to add more control sources (MIDI, WiFi, etc.)

**Ready for:** Phase 5 polish and enclosure work!

---

**Phase 3 Complete Date:** November 2, 2025
**Implementation Duration:** ~4 days (October 29 - November 2, 2025)
**Build Impact:** ~40KB Flash, +144 bytes RAM
