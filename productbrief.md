# Product Brief v2.0 - ESP32 Advanced Theremin

> **📋 Note on Architecture:** This document describes the **target architecture** for Phases 1-5 of the project. The current implementation (Phase 1) uses a simplified class structure:
> - `SensorManager` handles VL53L0X sensors (will become VL53L0XManager + SensorCalibration)
> - `AudioEngine` handles PWM audio (will evolve into Oscillator + AudioMixer + DACOutput + EffectsChain)
> - `Theremin` orchestrates the system
>
> See the actual codebase for current implementation details. This document serves as the roadmap and reference for future development.

## 1. Project Overview

**Project Name:** ESP32 Advanced Theremin
**Type:** DIY Performance Electronic Musical Instrument
**Goal:** Create a professional-grade theremin using ESP32 and Time-of-Flight sensors with multiple oscillators, effects, and versatile I/O options for live performance and recording.

**Current Status (November 1, 2025):**
- ✅ Phase 0 COMPLETE: Wokwi simulation functional
- ✅ Phase 1 COMPLETE: Hardware assembled and tested
- ✅ Phase 2 COMPLETE: DAC + oscillators + waveforms working
- ✅ **Phase 4 COMPLETE: Three-effect audio engine! (Delay + Chorus + Reverb)**
- ✅ 14.5% CPU usage with 85% headroom - performance EXCEEDED targets!
- ⏳ Phase 3 pending: Hardware controls + display (waiting for parts)
- 🔮 Phase G optional: Quality polish if desired

---

## 2. Evolution from v1.0

This version represents a significant evolution from the initial educational prototype:

**From:** Simple proof-of-concept with basic pitch/volume control
**To:** Performance-ready instrument with multiple oscillators, effects, and professional I/O

**Key Philosophy Shift:**
- v1.0: Learning tool for ESP32 + I2C + basic audio
- v2.0: **Playable musical instrument** with THE TONE™ in mind 🎸

---

## 3. Functional Requirements

### Core Features (Phase 1-2)
- **Dual Sensor Control:** VL53L0X sensors for pitch and volume via hand distance
- **Single Oscillator:** Selectable waveforms (Sine/Square/Sawtooth) via rotary switch
- **Octave Control:** -1/Normal/+1 octave switching per oscillator
- **Audio Output:** Internal DAC → PAM8403 amplifier → 3W 8Ω speaker
- **Real-time Response:** Target latency <20ms hand-to-sound

### Advanced Features (Phase 3-5)
- **Multiple Oscillators:** 2-3 independent oscillators (CPU-dependent)
- **Visual Feedback:** 2x WS2812B LED strips (8 LEDs each) as sensor range meters
- **Professional I/O:** 6.35mm jack line-out for external amplifiers/mixers
- **Effects Chain:** Delay, Chorus, (Reverb if CPU permits)
- **OLED Display:** SSD1306 0.96" showing waveform/CPU load/settings
- **Preset System:** (Future) Save/load configurations

### Optional/Future Features
- Headphone output with proper attenuation (low priority - HX Stomp available)
- MIDI out (USB or DIN)
- CV/Gate for modular integration
- Expression pedal input
- Tap tempo for effects

---

## 4. Technical Architecture

### 4.1 Hardware Components

#### Main Processing
- **Microcontroller:** ESP32 DevKit (dual-core, 240MHz, 520KB RAM)
- **Power Supply:** 5V 10A dedicated PSU with terminal blocks for distribution

#### Sensing & Control
- **Proximity Sensors:** 2x VL53L0X Time-of-Flight (I2C addresses 0x29, 0x30)
- **GPIO Expander:** MCP23017 (I2C, 16 additional GPIO for switches)
- **Control Switches:**
  - 3x Rotary 4-position (waveform: Off/Sine/Square/Saw per oscillator)
  - 3x Toggle 3-position (octave: -1/0/+1 per oscillator)
  - 3x Toggle 2-position (effects: Delay/Chorus/Reverb on/off)
  - 2x Toggle 2-position (LED meter enable, internal amp enable)

#### Audio Chain
```
ESP32 DAC (GPIO25/26) → Split:
  ├─→ [Volume POT] → PAM8403 → Speaker 8Ω 3W
  └─→ Jack 6.35mm (line-out, bypass amp)
```

**Power Distribution with Physical Switches:**
```
5V 10A PSU → Terminal Blocks:
  ├─→ ESP32 (5V pin) → Self-regulated 3.3V for sensors + display
  ├─→ [SPST Switch] → WS2812B LED strips (~480mA max)
  └─→ [SPST Switch] → PAM8403 + Speaker (200-500mA)
                    └─→ [LED indicator] → Shows amp powered status
```

**Audio Path Details:**
- **DAC:** Internal 8-bit (phase 1-2), optional I2S upgrade (MAX98357A/PCM5102)
- **Sample Rate:** 22-32kHz (sufficient for theremin range, saves CPU)
- **Internal Amp:** PAM8403 with volume potentiometer, **powered via physical switch** (no GPIO control)
- **Line Output:** Direct from DAC, pre-amplification (correct level for amp/mixer inputs)
- **LED Strips:** **Powered via separate physical switch** (no GPIO control, saves power when not needed)

#### Visual Feedback
- **LED Meters:** 2x WS2812B strips (8 LEDs each) showing sensor range
- **Display:** SSD1306 0.96" OLED (128x64, I2C)
- **Status LEDs:** Internal amp on/off indicator

#### Power Distribution
```
5V 10A PSU → Terminal Blocks:
  ├─→ ESP32 (5V pin) → Self-regulated 3.3V for sensors + display
  ├─→ WS2812B LED strips (switchable, ~480mA max)
  └─→ PAM8403 + Speaker (switchable, 200-500mA)
```

### 4.2 I2C Bus Architecture

**I2C Device Map:**
| Device | Address | Update Rate | Priority |
|--------|---------|-------------|----------|
| VL53L0X #1 (Pitch) | 0x30 | Continuous (~30ms) | CRITICAL |
| VL53L0X #2 (Volume) | 0x29 | Continuous (~30ms) | CRITICAL |
| MCP23017 (GPIO) | 0x20 | Event-driven (interrupt) | MEDIUM |
| SSD1306 (Display) | 0x3C | 20-30Hz | LOW |

**I2C Bus Load Management:**
- VL53L0X sensors: Continuous polling (unavoidable, ~30ms per reading)
- MCP23017: Interrupt-driven, reads only on switch change
- Display: Throttled to 20-30Hz refresh (not real-time)
- Total estimated bus load: ~40-50% (acceptable)

**Note:** WS2812B LEDs use GPIO data line (not I2C), no bus impact.

### 4.3 GPIO Pin Assignment

**ESP32 Pins:**
```
GPIO21 (SDA)     → I2C Data (sensors, expander, display)
GPIO22 (SCL)     → I2C Clock
GPIO16           → VL53L0X #1 XSHUT (address config)
GPIO17           → VL53L0X #2 XSHUT (address config)
GPIO25           → DAC Output (audio)
GPIO26           → (Reserved for future stereo/second DAC channel)
GPIO18           → WS2812B LED Strip #1 (pitch meter)
GPIO19           → WS2812B LED Strip #2 (volume meter)
```

**Note:** Amplifier and LED strip power are controlled by physical SPST switches on 5V power rails (hardware-only, no software control needed).

**MCP23017 Pins (via I2C):**
- GPA0-GPA7: Oscillator waveform rotary switches (4-pos × 3 = 12 combinations)
- GPB0-GPB7: Octave switches (3-pos × 3) + Effect switches (3) + LED/Display enable (2)

### 4.4 Software Architecture

#### Development Environment
- **IDE:** Visual Studio Code
- **Framework:** Arduino (ESP32 core)
- **Build System:** PlatformIO
- **Simulation:** Wokwi (for prototyping/testing before hardware)

#### Code Structure (Refactored)
```
src/
├── main.cpp                    # Main loop and setup
├── audio/
│   ├── Oscillator.h/.cpp      # Single oscillator class (waveform generation)
│   ├── AudioMixer.h/.cpp      # Mix multiple oscillators
│   ├── EffectsChain.h/.cpp    # Delay, Chorus, Reverb
│   └── DACOutput.h/.cpp       # DAC abstraction (internal/I2S)
├── sensors/
│   ├── VL53L0XManager.h/.cpp  # Dual sensor handling with XSHUT init
│   └── SensorCalibration.h    # Range mapping and smoothing
├── io/
│   ├── SwitchController.h/.cpp # MCP23017 interrupt handling
│   └── LEDMeter.h/.cpp        # WS2812B visualization
├── ui/
│   └── DisplayManager.h/.cpp  # OLED rendering (CPU/waveform/presets)
└── utils/
    ├── PerformanceMonitor.h   # CPU/RAM tracking
    └── Config.h               # Constants and pin definitions
```

#### Key Algorithms

**Oscillator Implementation (Wavetable Lookup):**
```cpp
class Oscillator {
  private:
    float phase;                // Current phase (0.0 - 1.0)
    float frequency;            // Target frequency (Hz)
    int16_t* wavetable;         // Pre-computed waveform (1024 samples)

  public:
    int16_t getNextSample(float sampleRate) {
      int index = (int)(phase * WAVETABLE_SIZE);
      phase += frequency / sampleRate;
      if (phase >= 1.0) phase -= 1.0;
      return wavetable[index];
    }
};
```

**Why Wavetable vs Real-Time Calculation:**
- ✅ ~10x faster (simple array lookup + interpolation)
- ✅ Predictable CPU load
- ✅ Easy to add complex waveforms (arbitrary shapes)
- ❌ Uses ~3-4KB RAM per oscillator (acceptable with 520KB available)

**Distance → Frequency Mapping:**
```cpp
// Exponential mapping for musical scale feel
float frequency = MIN_FREQ * pow(2.0, (distance / RANGE) * OCTAVES);

// With smoothing (moving average)
smoothedDistance = (smoothedDistance * 0.8) + (rawDistance * 0.2);
```

**Effects Implementation:**

*Delay:*
```cpp
// Circular buffer
delaySample = delayBuffer[readPtr];
delayBuffer[writePtr] = inputSample + (delaySample * feedback);
output = inputSample + (delaySample * mix);
```

*Chorus:*
```cpp
// Modulated delay with LFO
delayTime = baseDelay + (LFO_sin * modDepth);
output = inputSample + delayedSample(delayTime) * mix;
```

*Reverb (Freeverb - CPU intensive):*
- 8x parallel comb filters
- 4x series allpass filters
- Estimated CPU load: 30-50% (may be prohibitive with 3 oscillators)

#### FreeRTOS Task Architecture

```
Core 0 (High Priority - Audio):
├── Audio Generation Task (10kHz, highest priority)
│   ├── Read sensors (throttled to avoid jitter)
│   ├── Generate oscillator samples
│   ├── Apply effects chain
│   └── Output to DAC
└── LED Update Task (60Hz)

Core 1 (Lower Priority - UI/Control):
├── Display Update Task (20-30Hz)
├── Switch Polling Task (event-driven via interrupt)
└── Performance Monitoring Task (1Hz)
```

**Latency Budget:**
- Sensor reading: ~30ms (VL53L0X hardware limit)
- Processing + DAC output: <5ms target
- **Total system latency: <35ms** (acceptable for musical performance)

---

### 4.5 Dynamic CPU Resource Management

**Key Design Feature: Real-time CPU Load Balancing**

Each oscillator can be set to **OFF** via its rotary switch (4-position: Off/Sine/Square/Saw). When set to OFF, the oscillator consumes **near-zero CPU** (<0.1% vs ~15% when active) through early-return optimization:

```cpp
class Oscillator {
  WaveformType waveform; // OFF, SINE, SQUARE, SAW

  int16_t getNextSample() {
    if (waveform == OFF) {
      return 0;  // ← IMMEDIATE RETURN, no calculations!
    }

    // Only execute if oscillator is active
    int index = (int)(phase * WAVETABLE_SIZE);
    phase += frequency / sampleRate;
    if (phase >= 1.0) phase -= 1.0;
    return wavetable[index];
  }
};
```

**Performance Implications:**

| Configuration | Active Oscillators | Effects Active | Est. CPU Load | Status |
|---------------|-------------------|----------------|---------------|--------|
| **Ambient Mode** | 1 (Sine) | Delay + Reverb | ~50-60% | ✅ Safe |
| **Rich Texture** | 3 (all waveforms) | None | ~45-55% | ✅ Safe |
| **Balanced** | 2 (Sine + Saw) | Delay + Chorus | ~55-70% | ✅ Safe |
| **Effect-Heavy** | 1 (any) | Delay + Chorus + Reverb | ~60-75% | ⚠️ Test |
| **Maximum Power** | 3 (all) | Delay + Chorus | ~70-85% | ⚠️ May glitch |

**Real-time Adaptation Strategy:**

This design allows **dynamic CPU allocation** during live performance:

1. **Need heavy effects?** → Disable 1-2 oscillators, free up ~30% CPU for reverb
2. **Want thick harmonic texture?** → Enable all 3 oscillators, disable effects
3. **Experiencing glitches?** → Immediately switch oscillator(s) to OFF without restarting
4. **Musical context changes?** → Adjust oscillator/effect balance on-the-fly

**Example Performance Scenarios:**

```
Song Intro (atmospheric):
→ OSC1: SINE, OSC2: OFF, OSC3: OFF
→ Effects: Delay + Reverb ON
→ CPU: ~50-60% (plenty of headroom)

Verse (rhythmic texture):
→ OSC1: SQUARE, OSC2: SAW, OSC3: OFF
→ Effects: Delay ON, others OFF
→ CPU: ~40-50% (very stable)

Chorus (wall of sound):
→ OSC1: SINE, OSC2: SQUARE, OSC3: SAW
→ Effects: Delay + Chorus ON
→ CPU: ~70-80% (pushing limits but usable)

Breakdown (spacey):
→ OSC1: SINE, OSC2: OFF, OSC3: OFF
→ Effects: ALL ON (Delay + Chorus + Reverb)
→ CPU: ~70-75% (if reverb implemented)
```

**Key Advantages:**

✅ **No reboot required** - all changes happen in real-time
✅ **Graceful degradation** - instantly respond to CPU overload
✅ **Musical flexibility** - adapt timbre to song structure
✅ **Learning tool** - understand CPU cost of each feature
✅ **Future-proof** - same instrument, multiple use cases

**Implementation Detail:**

The mixer can optionally skip inactive oscillators entirely:

```cpp
int16_t AudioMixer::mixOscillators() {
  int32_t mix = 0;
  int activeCount = 0;

  if (osc1.isActive()) { mix += osc1.getNextSample(); activeCount++; }
  if (osc2.isActive()) { mix += osc2.getNextSample(); activeCount++; }
  if (osc3.isActive()) { mix += osc3.getNextSample(); activeCount++; }

  // Average only active oscillators (prevents volume drop when disabling)
  return activeCount > 0 ? (int16_t)(mix / activeCount) : 0;
}
```

This ensures:
- Volume stays consistent regardless of active oscillator count
- Function call overhead eliminated for OFF oscillators
- Audio buffer never contains stale data

**Bottom Line:**

The 4-position rotary switches (Off/Sine/Square/Saw) are not just waveform selectors - they're **dynamic CPU allocation controls**. This transforms potential "not enough CPU" limitation into a **performance feature**, allowing the player to make real-time tradeoffs between harmonic complexity and effect depth based on musical needs.

---

## 5. Performance Budget & Benchmarks

### 5.1 CPU Load Estimates (per feature)

| Component | CPU % (estimated) | RAM Usage |
|-----------|-------------------|-----------|
| **1 Oscillator (wavetable)** | ~10-15% | ~4KB |
| **2 Oscillators** | ~20-30% | ~8KB |
| **3 Oscillators** | ~35-45% | ~12KB |
| **Delay Effect** | ~5-10% | ~20KB (buffer) |
| **Chorus Effect** | ~10-15% | ~25KB (buffer) |
| **Reverb Effect** | ~30-50% | ~40KB (buffers) |
| **Sensor Reading** | ~5% | <1KB |
| **LED Update (60Hz)** | ~3% | ~1KB |
| **Display (30Hz)** | ~2% | ~2KB |
| **Switch Handling** | <1% | <1KB |

### 5.2 Realistic Configurations

**Configuration A - Conservative (100% safe):**
- 2 Oscillators + Delay + Chorus
- Estimated CPU: ~55-70%
- RAM: ~60KB
- Status: ✅ **SAFE**

**Configuration B - Target (should work):**
- 3 Oscillators + Delay + Chorus
- Estimated CPU: ~65-80%
- RAM: ~65KB
- Status: ⚠️ **TEST REQUIRED** (checkpoint after Phase 3)

**Configuration C - Ambitious (risky):**
- 3 Oscillators + Delay + Chorus + Reverb
- Estimated CPU: ~85-95%+
- RAM: ~105KB
- Status: ❌ **LIKELY TOO HEAVY** (Plan B: remove reverb or 3rd oscillator)

### 5.3 Critical Checkpoints

**CHECKPOINT 1 (After Phase 2):**
```
✓ Benchmark 1 oscillator + delay
✓ Measure actual CPU load and latency
✓ Verify audio quality at 22kHz sample rate
→ DECISION: Proceed with 2nd oscillator OR optimize
```

**CHECKPOINT 2 (After Phase 3):**
```
✓ Test 3 oscillators simultaneously
✓ Add chorus effect
✓ Monitor for audio dropouts/glitches
→ DECISION: Keep 3 osc OR drop to 2, proceed with reverb OR skip
```

**Target Metrics:**
- CPU usage: <75% sustained (leave headroom for peaks)
- Audio dropout rate: 0 (critical)
- Sensor-to-sound latency: <20ms
- Free RAM: >100KB (for safety)

---

## 6. Development Roadmap

### Phase 0 - Wokwi Virtual Prototyping ✅ COMPLETE
- [x] Circuit simulation with 2 potentiometers + buzzer
- [x] Basic pitch/volume control logic
- [x] Code refactoring with class architecture

### Phase 1 - Hardware Base Implementation 🔄 IN PROGRESS
**Goal:** Get real sensors + buzzer working on ESP32

**Tasks:**
- [ ] Setup PlatformIO project with correct libraries
- [ ] Physical circuit assembly (ESP32 + 2x VL53L0X + passive buzzer)
- [ ] Test VL53L0X XSHUT sequential initialization (address 0x29 and 0x30)
- [ ] Verify I2C communication stability
- [ ] Port Wokwi code to real hardware
- [ ] Test basic pitch control (1 sensor → PWM frequency)

**OTA Update Implementation ✅ COMPLETE (October 20, 2025):**
- [x] Created OTAManager class (include/OTAManager.h + src/OTAManager.cpp)
- [x] Implemented WiFi Access Point mode (ESP32 creates "Theremin-OTA" network)
- [x] Integrated ElegantOTA library v3.1.7
- [x] Added HTTP Basic Authentication for security (admin/theremin)
- [x] Fixed IP web interface at http://192.168.4.1/update
- [x] Conditional compilation with #ifdef ENABLE_OTA
- [x] Non-blocking operation (theremin continues playing during OTA)
- [x] Updated to use #pragma once for consistency with other headers
- [x] Created comprehensive OTA_SETUP.md documentation
- [x] Build verified: ✅ Compiles successfully (RAM: 48KB, Flash: 847KB)

**Success Criteria:**
- ✓ Both sensors read distances correctly
- ✓ Hand movement changes buzzer pitch
- ✓ No I2C errors or crashes
- ✓ Latency feels acceptable (<50ms)

**Components Needed:**
- ESP32 Dev Board
- 2x VL53L0X breakout modules
- Passive piezo buzzer
- Breadboard + jumpers
- 100Ω resistor (buzzer protection)

---

### Phase 2 - Audio Upgrade: DAC + Amplifier ✅ COMPLETE
**Goal:** Replace buzzer with real audio output and implement multiple waveforms

**Tasks:**
- [ ] Implement first Oscillator class with wavetable (sine/square/saw)
- [ ] Switch from PWM buzzer to ESP32 internal DAC (GPIO25)
- [ ] Add PAM8403 amplifier module + 8Ω speaker
- [ ] Implement octave switching logic (-1/0/+1)
- [ ] Add 6.35mm jack for line-out (direct from DAC, pre-amp)
- [ ] Test audio quality and volume levels
- [ ] **CHECKPOINT 1: Benchmark CPU/RAM with Display monitoring**

**Display Implementation (early for debugging):**
- [ ] Connect SSD1306 OLED (I2C address 0x3C)
- [ ] Create DisplayManager class
- [ ] Show real-time CPU usage % (calculated from processing time)
- [ ] Show free RAM in KB
- [ ] Show current frequency and sensor distances
- [ ] Refresh rate: 20-30Hz (no faster needed)

**Success Criteria:**
- ✓ Clean audio output (no crackling/distortion)
- ✓ Speaker and line-out both functional
- ✓ Octave switching works correctly
- ✓ **CPU usage <30%** with 1 oscillator + display
- ✓ **Latency <20ms** measured
- ✓ Display shows accurate CPU/RAM metrics

**Components Needed:**
- PAM8403 amplifier module (with volume pot)
- 8Ω 3W speaker
- 6.35mm mono jack
- SSD1306 0.96" OLED display
- SPST switch (amp enable/disable)
- LED indicator (amp status)

**Decision Point:**
- If CPU >40% with 1 osc → investigate optimization before adding more oscillators
- If latency >30ms → reduce sensor polling rate or optimize audio generation

---

### Phase 3 - Multiple Oscillators Expansion
**Goal:** Add 2nd and 3rd oscillators with independent controls

**Tasks:**
- [ ] Implement AudioMixer class (sum multiple oscillator outputs)
- [ ] Add 2nd Oscillator instance
- [ ] **BENCHMARK:** Test CPU load with 2 oscillators
- [ ] If CPU <60%, add 3rd Oscillator
- [ ] Add MCP23017 I2C GPIO expander
- [ ] Wire rotary switches for waveform selection (3x 4-position)
- [ ] Wire toggle switches for octave control (3x 3-position)
- [ ] Implement SwitchController class with interrupt handling
- [ ] Test all switch combinations
- [ ] **CHECKPOINT 2: CPU/RAM with 2-3 oscillators**

**Success Criteria:**
- ✓ 2 oscillators working: CPU <60%, no audio glitches
- ✓ 3 oscillators (if attempted): CPU <75%, audio stable
- ✓ All switches respond correctly via MCP23017
- ✓ No I2C conflicts between sensors, expander, display
- ✓ Latency still <20ms

**Components Needed:**
- MCP23017 I2C expander breakout
- 3x Rotary switches (4-position, make-before-break)
- 3x Toggle switches (3-position ON-ON-ON)
- Resistors for pull-ups (if not on MCP23017 module)

**Decision Point:**
- **If 3 osc causes CPU >80% or audio glitches:**
  - **Plan B:** Drop to 2 oscillators permanently
  - Update BOM and panel layout accordingly
  - Proceed to Phase 4

---

### Phase 4 - Visual Feedback & Effects ✅ **COMPLETE!** (November 1, 2025)
**Goal:** Add LED meters and effects (Delay, Chorus, Reverb)

**Status:** ✅ **ALL THREE EFFECTS COMPLETE!** (Delay + Chorus + Reverb) - LED meters deferred

**Completed Tasks:**
- [x] ✅ Implement EffectsChain class (include/EffectsChain.h + src/EffectsChain.cpp)
- [x] ✅ Add DelayEffect (circular buffer, feedback, mix controls)
  - [x] Circular buffer delay with feedback (10-2000ms range)
  - [x] Configurable feedback (0.0-0.95) and wet/dry mix
  - [x] ~13KB buffer for 300ms default delay
- [x] ✅ Add ChorusEffect (modulated delay with Oscillator-based LFO!)
  - [x] Innovative design: Reuses Oscillator class as LFO (~100x faster than sin()!)
  - [x] LFO rate: 0.1-10 Hz, depth: 1-50ms
  - [x] Linear interpolation for fractional delay reads
- [x] ✅ Add ReverbEffect (Simplified Freeverb algorithm!)
  - [x] 4 parallel comb filters + 2 series allpass filters
  - [x] Sample-rate agnostic (millisecond-based delays)
  - [x] Three strategic noise gates eliminate quantization buzzing
  - [x] Configurable room size, damping, and mix
- [x] ✅ AudioEngine integration - all effects in processing pipeline
- [x] ✅ ControlHandler integration - complete serial command control!
  - [x] All effects controllable via serial commands
  - [x] `effects:status` command shows all three effects
  - [x] Volume/pitch smoothing toggles for testing
- [x] ✅ **PERFORMANCE VALIDATED:** 14.5% CPU with 3 osc + delay + chorus + reverb! 🎉

**Deferred Tasks (LED meters - lower priority):**
- [ ] Connect 2x WS2812B LED strips (8 LEDs each)
- [ ] Implement LEDMeter class (map sensor distance → LED bar graph)
- [ ] Add toggle switch to enable/disable LED meters
- [ ] Display updates to show waveform preview

**Success Criteria:**
- ✅ **ALL THREE EFFECTS working on hardware!** Delay + Chorus + Reverb
- ✅ **Total CPU 14.5%** with all effects (85% headroom - EXCEEDED targets!)
- ✅ **Effects sound musical** - delay repeats, chorus shimmers, reverb adds space
- ✅ **No audio artifacts** - zero dropouts, clean reverb tail decay
- ✅ **Professional architecture** - RAII, Oscillator-based LFO, noise gates
- ✅ **Serial command control** - complete runtime control of all effects
- ✅ **Noise gate solution** - eliminates reverb quantization buzzing
- ⏳ **LED meters** - deferred (can add anytime, not critical)

**Phase 4 Complete Date:** November 1, 2025

**Optional Phase G (Quality Polish):**
- Not required - current quality excellent
- If pursuing: int32_t precision → full Freeverb upgrade
- See EFFECTS_IMPLEMENTATION_PLAN.md for details

---

### Phase 5 - Professional I/O & Polish
**Goal:** Finalize hardware integration and user experience

**Tasks:**
- [ ] Install physical SPST switch for amp power (on 5V rail to PAM8403)
- [ ] Install physical SPST switch for LED strip power (on 5V rail)
- [ ] Wire LED indicator in series with amp power (lights when amp powered)
- [ ] Label all controls (waveform, octave, effects, volume)
- [ ] Cable management and strain relief
- [ ] Test all signal paths:
  - Internal speaker (amp switch ON)
  - Line-out to external amp (amp switch ON or OFF, both work)
  - LED meters (switch ON/OFF)
- [ ] Final calibration of sensor ranges
- [ ] Optimize smoothing filters for playability
- [ ] Document control layout and signal flow

**Optional Enhancements:**
- [ ] Preset save/load system (store in SPIFFS/EEPROM)
- [ ] MIDI output implementation (USB MIDI or DIN)
- [ ] Expression pedal input (for effect parameters)
- [ ] Tap tempo for delay (with footswitch)

**Success Criteria:**
- ✓ All I/O paths functional and labeled
- ✓ Instrument is playable and responsive
- ✓ Professional appearance and ergonomics
- ✓ No loose wires or unstable connections
- ✓ User can switch between all features easily

---

### Phase 6 - Enclosure & Finishing (Future)
**Goal:** Build proper case and final aesthetics

**Tasks:**
- [ ] Design enclosure (3D print, laser-cut, or handmade)
- [ ] Mount sensors at ergonomic height/distance
- [ ] Create control panel with labeled switches
- [ ] Install jacks and power connector on rear panel
- [ ] Internal mounting for ESP32, breadboard, or PCB
- [ ] Cosmetic finishing (paint, labels, LED diffusers)

**Optional Professional Touches:**
- Custom PCB (eliminate breadboard)
- Wooden/metal enclosure (vintage theremin aesthetic)
- Adjustable sensor mounting arms
- Foot switch for effects/presets
- External expression pedal jack

---

### Phase 7 - Future Upgrades (Rabbit Hole Awaits 🐰)
**Goal:** THE TONE™ perfection (never truly ends)

**Ideas for Future Iterations:**
- **I2S DAC Upgrade:** Switch to MAX98357A or PCM5102 for 16-bit audio
- **Reverb Effect:** With better DAC and optimized code
- **More Waveforms:** Arbitrary wavetables, user-loadable
- **MIDI In:** Control theremin via MIDI keyboard/sequencer
- **CV/Gate Out:** Integrate with Eurorack modular system
- **WiFi Control:** Web interface for remote parameter tweaking
- **Granular Synthesis:** If you're feeling particularly adventurous
- **Multi-timbral Mode:** Different oscillators on different MIDI channels
- **Touch Sensors:** Capacitive touch for additional expression
- **Headphone Output:** With proper attenuation circuit (low priority for now)

**Note:** These are explicitly "future scope" to avoid feature creep. Current roadmap through Phase 5 is already ambitious!

---

## 7. Bill of Materials (BOM)

### Core Components (Phase 1-2)
| Component | Quantity | Est. Cost (€) | Notes |
|-----------|----------|---------------|-------|
| ESP32 Dev Board | 1 | 5-8 | Any variant (30-pin recommended) |
| VL53L0X ToF Sensor | 2 | 6-12 | Breakout with regulator + pull-ups |
| PAM8403 Amplifier | 1 | 1-2 | With volume potentiometer |
| Speaker 8Ω 3W | 1 | 2-4 | Full-range, ~50mm diameter |
| SSD1306 OLED 0.96" | 1 | 3-5 | I2C, 128x64 resolution |
| Passive Piezo Buzzer | 1 | 1 | For Phase 1 testing only |
| **Subtotal** | | **18-32** | |

### Expansion Components (Phase 3-4)
| Component | Quantity | Est. Cost (€) | Notes |
|-----------|----------|---------------|-------|
| MCP23017 I2C Expander | 1 | 2-3 | Breakout or bare IC + decoupling |
| Rotary Switch 4-pos | 3 | 6-12 | Waveform selection per oscillator |
| Toggle Switch 3-pos | 3 | 3-6 | Octave control (ON-ON-ON) |
| Toggle Switch 2-pos | 5 | 2-5 | Effects, LED, amp enable |
| WS2812B LED Strip 8-LED | 2 | 4-8 | Or individual addressable LEDs |
| LED Indicator (amp) | 1 | 0.50 | 3mm or 5mm, any color |
| 220Ω Resistor (LED) | 1 | 0.10 | Current limiting |
| **Subtotal** | | **17.6-34.6** | |

### Professional I/O (Phase 5)
| Component | Quantity | Est. Cost (€) | Notes |
|-----------|----------|---------------|-------|
| 6.35mm Mono Jack | 1 | 1-2 | Panel-mount, line-out |
| SPST Toggle Switch | 2 | 1-2 | Amp power, LED power (on 5V rails) |
| 3.5mm Stereo Jack | 1 | 1 | (Future) Headphone out |
| 100Ω + 47Ω Resistors | 2 | 0.20 | (Future) Headphone attenuator |
| **Subtotal** | | **3.2-5.2** | |

### Power & Connectivity
| Component | Quantity | Est. Cost (€) | Notes |
|-----------|----------|---------------|-------|
| 5V 10A Power Supply | 1 | 10-15 | Dedicated PSU (already owned) |
| Terminal Blocks | 3-4 | 2-4 | Power distribution |
| Breadboard 830-point | 1-2 | 5-10 | Or custom PCB later |
| Jumper Wires (M-M) | 1 pack | 2-3 | Various lengths |
| Jumper Wires (M-F) | 1 pack | 2-3 | For sensors/modules |
| **Subtotal** | | **21-35** | (Exclude PSU if owned: 11-20) |

### Miscellaneous
| Component | Quantity | Est. Cost (€) | Notes |
|-----------|----------|---------------|-------|
| 100Ω Resistor (buzzer) | 1 | 0.10 | Protection for Phase 1 |
| Hookup Wire (22-24 AWG) | 1 spool | 3-5 | Internal wiring |
| Heat Shrink Tubing | 1 pack | 2-3 | Insulation |
| Velcro/Mounting Tape | 1 roll | 2-3 | Module mounting |
| Enclosure (DIY/3D print) | 1 | 0-30 | Depends on approach |
| **Subtotal** | | **7.1-41.1** | |

### **TOTAL ESTIMATED COST**
- **Minimum (no enclosure, PSU owned):** €55-80
- **Typical (basic enclosure, PSU owned):** €75-110
- **Maximum (professional finish, new PSU):** €90-150

**Note:** Prices are estimates for EU/Italy market. Bulk buying (e.g., AliExpress) can reduce costs by 30-50%.

---

## 8. Risks, Challenges & Mitigation

### 8.1 Technical Risks

| Risk | Likelihood | Impact | Mitigation Strategy |
|------|------------|--------|---------------------|
| **CPU overload with 3 oscillators + effects** | HIGH | HIGH | Checkpoint benchmarks after each phase; Plan B: drop to 2 osc or skip reverb |
| **I2C bus congestion causing latency** | MEDIUM | MEDIUM | Throttle display refresh, use async sensor reads, FreeRTOS task priorities |
| **Audio glitches/dropouts under load** | MEDIUM | HIGH | Lower sample rate (22kHz), optimize audio loop, test extensively |
| **VL53L0X sensor noise/jitter** | MEDIUM | MEDIUM | Moving average filter, calibrate dead zones, smooth mapping curves |
| **MCP23017 interrupt conflicts** | LOW | MEDIUM | Careful ISR design, debounce switches in software |
| **Power supply noise affecting DAC** | LOW | LOW | Add decoupling caps, separate analog/digital grounds if needed |
| **WS2812B LEDs causing voltage drops** | LOW | LOW | Separate 5V rail with switch, buffer data line |

### 8.2 Development Challenges

**Challenge:** Balancing feature richness vs. real-time performance
- **Solution:** Incremental development with mandatory checkpoints
- **Fallback:** Feature priority list (oscillators > effects > visual feedback)

**Challenge:** Manual switch wiring complexity (17+ connections)
- **Solution:** Use MCP23017 to consolidate, clear labeling, test each switch individually
- **Future:** Custom PCB to eliminate breadboard

**Challenge:** Sensor calibration for musical playability
- **Solution:** Adjustable min/max ranges via serial interface or potentiometers
- **Testing:** Multiple hand sizes, different lighting conditions

**Challenge:** Enclosure design (sensors must be externally accessible)
- **Solution:** Modular approach - get electronics working first, case later
- **Inspiration:** Study classic theremin ergonomics (Moog Etherwave, etc.)

### 8.3 Success Criteria & Testing

**Minimum Viable Instrument (MVI):**
- ✓ 2 oscillators with sine/square/saw waveforms
- ✓ Pitch and volume control via sensors (<20ms latency)
- ✓ Delay effect functional
- ✓ Line-out produces clean signal for external amp
- ✓ Internal speaker works for practice
- ✓ No crashes or audio dropouts in 10-minute session

**Stretch Goals:**
- ✓ 3 oscillators simultaneously
- ✓ Chorus effect in addition to delay
- ✓ LED meters providing visual feedback
- ✓ Display showing waveform and CPU metrics
- ✓ Reverb effect (if CPU permits)

**Testing Protocol:**
- **Unit Tests:** Each class/module tested in isolation
- **Integration Tests:** Sensors → Audio pipeline → Output
- **Performance Tests:** CPU/RAM benchmarks at each checkpoint
- **User Tests:** Playability evaluation by musician (you!)

---

## 9. Software Libraries & Dependencies

### PlatformIO Configuration
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200

lib_deps =
    adafruit/Adafruit VL53L0X @ ^1.2.0
    adafruit/Adafruit MCP23017 @ ^2.3.0
    adafruit/Adafruit SSD1306 @ ^2.5.7
    adafruit/Adafruit GFX Library @ ^1.11.5
    fastled/FastLED @ ^3.6.0

build_flags =
    -DDEBUG_MODE=1
```

**Note:** The `build_flags` shown are project-specific. The current implementation uses `-DDEBUG_MODE=1` for custom debug logging and `-DWOKWI_SIMULATION` for simulation builds. The `-DBOARD_HAS_PSRAM` flag is only needed if your ESP32 variant has PSRAM (most standard DevKits do not).

### Key Libraries

| Library | Purpose | Version | License |
|---------|---------|---------|---------|
| **Adafruit_VL53L0X** | ToF sensor control | 1.2.0+ | BSD |
| **Adafruit_MCP23017** | GPIO expander | 2.3.0+ | BSD |
| **Adafruit_SSD1306** | OLED display | 2.5.7+ | BSD |
| **Adafruit_GFX** | Graphics primitives | 1.11.5+ | BSD |
| **FastLED** | WS2812B LED control | 3.6.0+ | MIT |
| **Wire.h** | I2C communication | Built-in | - |
| **FreeRTOS** | Multitasking | Built-in (ESP32) | MIT |

**Notes:**
- All libraries available via PlatformIO Library Manager
- Adafruit libraries well-documented with examples
- FastLED supports multiple LED types (future-proof)

---

## 10. Future Considerations & Open Questions

### 10.1 Headphone Output (Deferred)

**Current Status:** LOW PRIORITY - HX Stomp available as workaround

**If implemented in future:**
- Requires attenuator circuit (100Ω + 47Ω to ground)
- 3.5mm TRS jack with switching contact to mute speaker
- Separate volume control (or PAM8403 channel R repurposed)
- **Risk:** Easy to damage headphones if attenuation incorrect
- **Recommendation:** Only implement after thorough testing with cheap headphones

**Reference Design:**
```
DAC → [100Ω] → Jack 3.5mm (L+R) → Headphones 32Ω
              ↓
            [47Ω]
              ↓
             GND
```

**Decision:** Address in Phase 7 or later, not critical for initial builds.

### 10.2 I2S DAC Upgrade Path

**When to consider:**
- Audio quality becomes limiting factor
- Want to implement high-quality reverb
- Need true stereo output (panning oscillators)
- Performing live and need professional audio specs

**Recommended I2S DACs:**
- **MAX98357A:** DAC + 3W amp, I2S, ~€5 (all-in-one solution)
- **PCM5102:** Stereo DAC, I2S, excellent SNR, ~€4 (requires external amp)
- **UDA1334A:** Stereo DAC, I2S, Adafruit breakout, ~€8

**Migration complexity:** MEDIUM
- Change from `dacWrite()` to `i2s_write()`
- Requires buffer management (I2S is DMA-based)
- Better CPU efficiency (DMA offloads audio output)
- More complex debugging

### 10.3 MIDI Implementation Ideas

**MIDI Out - Theremin to DAW/Synth:**
- Send pitch (MIDI note number) and velocity (volume sensor)
- Pitch bend for microtonal expression
- Control change (CC) messages for effects parameters
- **Use case:** Use theremin as MIDI controller for soft synths

**MIDI In - External Control:**
- MIDI notes trigger specific frequencies (quantized mode)
- MIDI CC controls effect parameters (delay time, chorus depth)
- **Use case:** Integrate into MIDI setup, sync with sequencers

**Libraries:** `MIDI.h` (USB MIDI) or `FortySevenEffects MIDI Library` (DIN)

### 10.4 Alternative Sensor Options (Future R&D)

**Current:** VL53L0X (30-1000mm range, laser ToF)

**Alternatives if VL53L0X proves limiting:**
- **VL53L1X:** Longer range (up to 4m), faster readings
- **VL53L4CD:** Wide field-of-view (18°), better hand detection
- **APDS-9960:** Gesture sensor (proximity + RGB + gesture recognition)
- **Ultrasonic (HC-SR04):** Cheaper but noisier, tested in Wokwi

**Decision:** Stick with VL53L0X for v2.0, evaluate alternatives only if major issues arise.

---

## 11. Design Philosophy & Artistic Vision

### Why This Project Exists

**Beyond Technical Challenge:**
This isn't just about making sounds with an ESP32. It's about:
- **Expressive Performance:** Real-time gestural control, no buttons/knobs mid-performance
- **Sonic Exploration:** Multiple oscillators and effects for timbral variety
- **The Tone™ Quest:** As a guitarist, chasing that perfect sound never ends
- **DIY Ethos:** Building your own instrument creates deeper connection to music

**Inspiration:**
- Classic theremins (Moog Etherwave, Theremin World)
- Modular synths (Eurorack philosophy of building custom signal chains)
- Guitar pedal culture (stacking effects for unique tones)
- ESP32 audio projects (Talkie, AudioInI2S, Mozzi)

### Musical Applications

**Genre Fit:**
- Ambient / Drone (sustained tones, slow evolving textures)
- Experimental / Noise (harsh waveforms, heavy effects)
- Electronic / Synthwave (retro vibes, leads over backing tracks)
- Soundtracks / Scoring (eerie, otherworldly atmospheres)
- Live Looping (with external looper pedal via line-out)

**Integration with Existing Setup:**
```
Theremin Line-Out → HX Stomp → FRFR Speaker / DAW
                     ↓
                Additional FX, IR loading, recording
```

The theremin becomes another "instrument" in your rig, processable like guitar.

---

## 12. Documentation & Knowledge Transfer

### 12.1 Project Documentation Requirements

**Code Documentation:**
- Inline comments for complex algorithms (wavetable generation, effects DSP)
- Header file comments (class purpose, public API usage)
- README.md with build instructions and dependencies

**Hardware Documentation:**
- Fritzing diagram (breadboard view for Phase 1-4)
- Schematic diagram (Eagle/KiCad for Phase 5+)
- Component placement photos (top/bottom of breadboard)
- Pin assignment table (quick reference)

**User Manual (Future):**
- Control layout diagram (what each switch does)
- Quick start guide (power on, basic operation)
- Calibration procedure (adjusting sensor ranges)
- Troubleshooting guide (common issues and fixes)

### 12.2 Lessons Learned Log

**Maintain journal for:**
- What worked better than expected (e.g., "VL53L0X more stable than thought")
- What didn't work (e.g., "Reverb too CPU-heavy, removed")
- Design decisions and rationale (why DAC internal vs I2S)
- Performance optimization tricks discovered
- Gotchas and workarounds (e.g., "MCP23017 interrupt pin needs pull-up")

This becomes invaluable for:
- Debugging later issues
- Sharing knowledge with community (blog post, GitHub)
- Planning v3.0 improvements

---

## 13. Community & Open Source

### Sharing the Project

**If/When Published:**
- **GitHub Repository:** Full code, schematics, BOM, build log
- **License:** MIT or GPL (for code), CERN-OHL (for hardware)
- **Video Demo:** Playing the instrument, showcasing features
- **Blog Post:** Technical deep-dive, challenges, solutions
- **Platforms:** Hackaday, Arduino Project Hub, Reddit (r/synthdiy, r/esp32)

**What Makes This Project Shareable:**
- Clear incremental roadmap (others can stop at any phase)
- Realistic performance budgets (managing expectations)
- Multiple fallback options (Plan B for CPU issues)
- Detailed BOM with cost estimates
- Beginner-friendly yet extensible architecture

**Potential Community Contributions:**
- Alternative enclosure designs (3D printable STLs)
- Additional effect algorithms
- Preset libraries
- MIDI implementation variants
- I2S DAC integration examples

---

## 14. Success Metrics & Project Completion

### Minimum Viable Product (MVP) Definition

**Phase 1-2 Success = MVP Complete:**
- ✓ ESP32 + 2 sensors + DAC + amp + speaker fully functional
- ✓ 1 oscillator with 3 waveforms + octave switching
- ✓ Line-out works for external amp
- ✓ Display shows CPU/RAM metrics
- ✓ Latency <20ms
- ✓ Can play simple melodies controllably

**At this point, you have a WORKING INSTRUMENT.** Everything beyond is enhancement.

### Full Feature Set (Phases 3-5)

**Complete when:**
- 2-3 oscillators (whichever CPU permits)
- Delay + Chorus effects (Reverb optional)
- LED visual feedback
- All switches operational via MCP23017
- Professional I/O (line-out, amp enable/disable)
- No critical bugs or stability issues

**This is a PERFORMANCE-READY INSTRUMENT.**

### The Rabbit Hole (Phase 7+)

**There is no "done."**

Welcome to THE TONE™ quest - there's always:
- One more oscillator to add
- One more effect to implement
- Better DAC, better amp, better speakers
- MIDI, CV, WiFi, Bluetooth...
- Custom PCB, custom enclosure, custom everything

**But that's the beauty of it.** 🐰🎸

---

## 15. Next Immediate Actions

### Week 1-2: Hardware Assembly (Phase 1)
1. **Order/gather components:**
   - ESP32, VL53L0X (x2), buzzer, breadboard, wires
   - SSD1306 display (for early CPU monitoring)

2. **Setup development environment:**
   - Install PlatformIO in VSCode
   - Create new project with platformio.ini config
   - Test blink sketch on ESP32

3. **Build Phase 1 circuit:**
   - ESP32 + 2x VL53L0X with XSHUT addressing
   - Passive buzzer on GPIO25 (PWM)
   - Test I2C communication with both sensors

4. **Port Wokwi code to hardware:**
   - Adapt sensor reading logic
   - Test distance → frequency mapping
   - Verify latency is acceptable

### Week 3-4: Audio Upgrade (Phase 2)
1. **Order audio components:**
   - PAM8403, speaker 8Ω 3W, jack 6.35mm

2. **Implement Oscillator class:**
   - Pre-generate wavetables (sine, square, saw)
   - Test wavetable lookup performance

3. **Switch from PWM to DAC output:**
   - Refactor audio generation loop
   - Add PAM8403 + speaker
   - Install line-out jack

4. **Add display for monitoring:**
   - Connect SSD1306 (I2C)
   - Implement CPU usage calculation
   - Show real-time metrics

5. **CHECKPOINT 1:**
   - Measure CPU %, RAM usage, latency
   - Document findings
   - Decide: proceed to Phase 3 or optimize?

### Month 2: Expansion & Effects (Phases 3-4)
- Follow roadmap based on Phase 2 results
- Incremental addition of oscillators
- Implement effects one at a time
- Continuous performance monitoring

---

## 16. Closing Notes

### This Is a Journey, Not a Destination

You started with "let's make a simple theremin" and now you're designing a multi-oscillator synthesizer with effects, visual feedback, and professional I/O. **This is the way.** 🎸

**Key Takeaways:**
1. **Incremental development saves sanity** - checkpoints prevent runaway feature creep
2. **Performance budgets are your friend** - know your limits before you hit them
3. **Plan B is not failure** - dropping from 3 to 2 oscillators still makes a great instrument
4. **Documentation is future-you's best friend** - you'll thank yourself in 6 months
5. **The Tone™ is a journey** - enjoy the process, don't stress the perfect endpoint

### Final Wisdom from a Fellow Tone Chaser

> "The perfect tone doesn't exist. But the search for it is where all the fun happens. And hey, at least you're building your own gear now - that's already 10x cooler than just buying pedals."
>
> — Every guitarist who learned to solder 😄

**Now go make some weird, wonderful sounds!** 🎶⚡

---

**Document Version:** 2.0
**Last Updated:** 2025-01-XX (update date when finalizing)
**Status:** Living Document (will evolve with project)
**Author:** [Your Name]
**Contact:** [Email/GitHub]

---

## Appendix A - Quick Reference Tables

### A.1 Pin Assignment Summary
| Function | ESP32 Pin | Connected To | Notes |
|----------|-----------|--------------|-------|
| I2C SDA | GPIO21 | VL53L0X, MCP23017, SSD1306 | Shared bus |
| I2C SCL | GPIO22 | VL53L0X, MCP23017, SSD1306 | Shared bus |
| Sensor 1 XSHUT | GPIO16 | VL53L0X #1 | Address config |
| Sensor 2 XSHUT | GPIO17 | VL53L0X #2 | Address config |
| DAC Out | GPIO25 | PAM8403 IN, Jack 6.35mm | Audio signal |
| LED Strip 1 | GPIO18 | WS2812B (pitch meter) | Data line |
| LED Strip 2 | GPIO19 | WS2812B (volume meter) | Data line |

**Hardware Switches (not GPIO-controlled):**
- PAM8403 amplifier power: Physical SPST switch on 5V rail
- WS2812B LED strips power: Physical SPST switch on 5V rail
- Amp status LED: Wired in series with amp power (lights when amp powered)

### A.2 I2C Address Map
| Device | Address (Hex) | Configurable? |
|--------|---------------|---------------|
| VL53L0X #1 (Pitch) | 0x30 | Yes (via XSHUT) |
| VL53L0X #2 (Volume) | 0x29 | Default |
| MCP23017 (GPIO) | 0x20 | Yes (via A0-A2 pins) |
| SSD1306 (Display) | 0x3C | Usually fixed |

### A.3 Effect Parameters Reference
| Effect | Parameter | Range | Description |
|--------|-----------|-------|-------------|
| **Delay** | Time | 50-1000ms | Delay buffer length |
| | Feedback | 0-95% | Repeating decay rate |
| | Mix | 0-100% | Wet/dry balance |
| **Chorus** | Rate | 0.1-5Hz | LFO modulation speed |
| | Depth | 0-50ms | Pitch variation amount |
| | Mix | 0-100% | Wet/dry balance |
| **Reverb** | Room Size | 0.1-1.0 | Virtual room dimensions |
| (optional) | Damping | 0.1-1.0 | High frequency rolloff |
| | Mix | 0-100% | Wet/dry balance |

---

**END OF DOCUMENT**

*May your oscillators stay in tune and your CPU stay under 75%. Happy hacking! 🎵🔧*
