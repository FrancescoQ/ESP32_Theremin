# Progress - ESP32 Theremin

> **📌 Roadmap Reference:**
> This progress tracker follows the v2.0 development roadmap (Phases 0-7).
> See `/productbrief.md` for complete feature specifications and phase details.

## Current Status

**Project Phase:** Phase 1 - Architecture Refactoring Complete ✅
**Overall Completion:** ~15% (Phase 0 + Phase 1 foundation complete)
**Last Updated:** October 19, 2025

### Status Summary
Major architectural refactoring completed! The project has evolved from a monolithic 250-line main.cpp into a clean, modular, object-oriented architecture with separate classes:
- **SensorManager** (header + implementation): Handles all sensor input (simulation and hardware)
- **AudioEngine** (header + implementation): Manages PWM audio synthesis
- **Theremin** (header + implementation): Coordinates sensors and audio

The Wokwi simulation environment is complete and functional. The refactored architecture is future-proof and ready for v2.0 features (DAC audio, multiple oscillators, effects, display, etc.).

**Current State:** Clean Phase 1 foundation ready. Next: Test in Wokwi, then proceed to Phase 2 (DAC + Amplifier upgrade)

## What Works

### Documentation ✅
- [x] Complete project brief defining goals and requirements
- [x] Product context explaining why and how the theremin should work
- [x] System architecture and design patterns documented
- [x] Technical stack and constraints identified
- [x] Active context for ongoing work tracking
- [x] Progress tracking initialized

### Decisions Made ✅
- [x] Sensor selection: VL53L0X Time-of-Flight (2 units)
- [x] Audio output: Passive piezoelectric buzzer with PWM
- [x] Development environment: VSCode + PlatformIO + Arduino framework
- [x] I2C addressing strategy: XSHUT-based sequential initialization
- [x] Development approach: Wokwi simulation first, then hardware

## What's Left to Build

### Phase 0: Wokwi Virtual Prototyping ✅ COMPLETE

- [x] **Development Environment**
  - [x] PlatformIO installed and accessible at `/Users/fquagliati/.platformio/penv/bin/pio`
  - [x] Created PlatformIO project structure
  - [x] Configured platformio.ini with dual environments (hardware + simulation)
  - [x] Successfully built firmware for simulation

- [x] **Wokwi Simulation Setup**
  - [x] Created diagram.json with ESP32, 2 potentiometers, buzzer, and resistor
  - [x] Created wokwi.toml configuration pointing to correct firmware path
  - [x] Virtual circuit properly wired (GPIO34/35 for ADC, GPIO25 for PWM)
  - [x] Ready for simulation launch

- [x] **Architecture Refactoring (October 18, 2025)**
  - [x] Extracted SensorManager class (include/SensorManager.h + src/SensorManager.cpp)
  - [x] Extracted AudioEngine class (include/AudioEngine.h + src/AudioEngine.cpp)
  - [x] Created Theremin coordinator class (include/Theremin.h + src/Theremin.cpp)
  - [x] Implemented conditional compilation with `#ifdef WOKWI_SIMULATION`
  - [x] Added smoothing filter (5-sample moving average) in SensorManager
  - [x] Implemented ADC-to-distance conversion for simulation mode
  - [x] Simplified main.cpp to ~40 lines (was 250+)
  - [x] Comprehensive serial debug output
  - [x] Created ARCHITECTURE.md documentation
  - [x] Build verified: ✅ Compiles successfully

### Phase 1: Hardware Base Implementation ✅ FOUNDATION COMPLETE

**Goal:** Get real sensors + buzzer working on ESP32

**Status:** Architecture complete, ready for hardware testing

- [x] **Architecture Foundation**
  - [x] SensorManager class with hardware/simulation modes
  - [x] AudioEngine class with PWM audio generation
  - [x] Theremin coordinator class
  - [x] Clean separation of concerns
  - [x] Future-proof design for v2.0 features

- [x] **OTA Update Implementation (October 20, 2025)**
  - [x] Created OTAManager class (include/OTAManager.h + src/OTAManager.cpp)
  - [x] Implemented WiFi Access Point mode (ESP32 creates "Theremin-OTA" network)
  - [x] Integrated ElegantOTA library v3.1.7
  - [x] Added HTTP Basic Authentication for security (admin/theremin)
  - [x] Fixed IP web interface at http://192.168.4.1/update
  - [x] Conditional compilation with #ifdef ENABLE_OTA
  - [x] Non-blocking operation (theremin continues playing during OTA)
  - [x] Updated OTAManager.h to use #pragma once for consistency
  - [x] **Optional button activation** - GPIO pin configurable via macro
    - [x] OTA_ENABLE_PIN macro in main.cpp (-1 = always on, >=0 = button check)
    - [x] Button logic encapsulated in OTAManager.begin()
    - [x] Saves ~50-70KB RAM when button not pressed during boot
    - [x] Active LOW with internal pullup (connect button to GND)
  - [x] Created comprehensive OTA_SETUP.md documentation
  - [x] Updated README.md and OTA_SETUP.md with button activation details
  - [x] Build verified: ✅ Compiles successfully (RAM: 48KB, Flash: 847KB)

- [ ] **Hardware Testing (When Components Acquired)**
  - [ ] Acquire ESP32 development board
  - [ ] Acquire 2x VL53L0X ToF sensor modules
  - [ ] Acquire passive piezo buzzer
  - [ ] Connect ESP32, test upload
  - [ ] Test VL53L0X XSHUT sequential initialization
  - [ ] Verify I2C communication stability
  - [ ] Test basic pitch control (1 sensor → PWM frequency)

**Success Criteria:**
- ✓ Clean architecture implemented
- ⏳ Both sensors read distances correctly (pending hardware)
- ⏳ Hand movement changes buzzer pitch (pending hardware)
- ⏳ No I2C errors or crashes (pending hardware)

---

### Phase 2: Audio Upgrade - DAC + Amplifier (v2.0 Begins)

**Goal:** Replace PWM buzzer with real audio output (DAC + speaker + line-out)

**Status:** Not Started

- [ ] **Oscillator Class Implementation**
  - [ ] Create Oscillator class with wavetable generation
  - [ ] Implement sine/square/sawtooth waveforms
  - [ ] Pre-compute wavetables (1024 samples each)
  - [ ] Test wavetable lookup performance

- [ ] **DAC Audio Output**
  - [ ] Switch from PWM to ESP32 internal DAC (GPIO25)
  - [ ] Implement proper sample rate (22-32kHz)
  - [ ] Add PAM8403 amplifier module + 8Ω speaker
  - [ ] Add 6.35mm jack for line-out (pre-amp signal)
  - [ ] Test audio quality and volume levels

- [ ] **Display for Monitoring (Early Implementation)**
  - [ ] Connect SSD1306 OLED (I2C address 0x3C)
  - [ ] Create DisplayManager class
  - [ ] Show real-time CPU usage %
  - [ ] Show free RAM in KB
  - [ ] Show current frequency and sensor distances

- [ ] **CHECKPOINT 1: Benchmark**
  - [ ] Measure CPU usage with 1 oscillator + display
  - [ ] Measure latency (target <20ms)
  - [ ] Verify audio quality at chosen sample rate
  - [ ] **DECISION:** Proceed to Phase 3 OR optimize

**Success Criteria:**
- ✓ Clean DAC audio output (no crackling/distortion)
- ✓ Speaker and line-out both functional
- ✓ **CPU usage <30%** with 1 oscillator + display
- ✓ **Latency <20ms** measured
- ✓ Display shows accurate CPU/RAM metrics

---

### Phase 3: Multiple Oscillators Expansion (v2.0 Feature)

**Goal:** Add 2nd and 3rd oscillators with independent controls

**Status:** Not Started

- [ ] **AudioMixer Class**
  - [ ] Implement AudioMixer to sum multiple oscillator outputs
  - [ ] Add 2nd Oscillator instance
  - [ ] **BENCHMARK:** Test CPU load with 2 oscillators
  - [ ] If CPU <60%, add 3rd Oscillator

- [ ] **Control Expansion**
  - [ ] Add MCP23017 I2C GPIO expander
  - [ ] Wire rotary switches for waveform selection (3x 4-position)
  - [ ] Wire toggle switches for octave control (3x 3-position)
  - [ ] Implement SwitchController class with interrupt handling

- [ ] **CHECKPOINT 2: Performance Test**
  - [ ] Test CPU/RAM with 2-3 oscillators
  - [ ] Monitor for audio glitches/dropouts
  - [ ] **DECISION:** Keep 3 osc OR drop to 2

**Success Criteria:**
- ✓ 2 oscillators: CPU <60%, no audio glitches
- ✓ 3 oscillators (if attempted): CPU <75%, audio stable
- ✓ All switches respond correctly via MCP23017
- ✓ Latency still <20ms

---

### Phase 4: Visual Feedback & Effects (v2.0 Feature)

**Goal:** Add LED meters and effects (Delay, Chorus)

**Status:** Not Started

- [ ] **LED Meters**
  - [ ] Connect 2x WS2812B LED strips (8 LEDs each)
  - [ ] Implement LEDMeter class
  - [ ] Map sensor distance → LED bar graph

- [ ] **Effects Chain**
  - [ ] Implement EffectsChain class
  - [ ] Add Delay effect (circular buffer)
  - [ ] Add Chorus effect (modulated delay with LFO)
  - [ ] Wire toggle switches for effect on/off

- [ ] **CHECKPOINT 3: Full System Test**
  - [ ] Test CPU with oscillators + delay + chorus
  - [ ] **DECISION:** Attempt Reverb OR skip to Phase 5

**Success Criteria:**
- ✓ LED meters track sensor distances smoothly
- ✓ Effects sound good (no artifacts)
- ✓ **Total CPU <75%** with all features active

---

### Phase 5: Professional I/O & Polish (v2.0 Feature)

**Goal:** Finalize hardware integration and user experience

**Status:** Not Started

- [ ] Install amp enable/disable switch
- [ ] Wire LED indicator for amp status
- [ ] Label all controls
- [ ] Cable management and strain relief
- [ ] Test all signal paths
- [ ] Final calibration of sensor ranges
- [ ] Document control layout

**Success Criteria:**
- ✓ All I/O paths functional and labeled
- ✓ Instrument is playable and responsive
- ✓ Professional appearance

---

### Phase 6: Enclosure & Finishing (Future)

**Goal:** Build proper case and final aesthetics

**Status:** Not Started

(See `/productbrief.md` Phase 6 for details)

---

### Phase 7: Future Upgrades (Future)

**Goal:** THE TONE™ perfection (never truly ends)

**Status:** Not Started

Ideas: I2S DAC upgrade, MIDI, CV/Gate, WiFi control, granular synthesis...

(See `/productbrief.md` Phase 7 for complete list)

## Known Issues

### Current Issues
None yet - no code has been implemented.

### Potential Issues to Watch For
1. **I2C Address Conflict**: Both VL53L0X sensors default to 0x29
   - Solution ready: XSHUT sequential initialization
   - Must implement carefully to avoid addressing errors

2. **Reading Jitter**: Raw sensor readings may be noisy
   - Solution ready: Moving average filter
   - Will need tuning based on actual behavior

3. **Latency**: Two sequential sensor reads = ~50ms minimum
   - Currently acceptable for musical control
   - Monitor during testing, optimize if needed

4. **Wokwi Limitations**: VL53L0X may not be available in simulator
   - Fallback: Use HC-SR04 for logic testing
   - Migration path: Test logic in Wokwi, then adapt to VL53L0X on hardware

5. **PWM Audio Quality**: Square wave will sound harsh
   - Expected and acceptable for this phase
   - Clear upgrade path to DAC exists

## Evolution of Project Decisions

### Initial Planning (October 15, 2025)

**Sensor Choice: VL53L0X vs. Alternatives**
- **Decision:** VL53L0X Time-of-Flight laser sensors
- **Reasoning:**
  - Precision: ±3% accuracy ideal for musical control
  - No interference: Two sensors can operate simultaneously
  - Speed: <30ms readings fast enough for real-time
  - I2C interface: Clean integration with ESP32
- **Rejected alternatives:**
  - HC-SR04 ultrasonic: Too slow, mutual interference likely
  - IR sensors: Too imprecise, ambient light sensitive
  - Capacitive: Too complex for educational project

**Audio Output: Buzzer vs. DAC**
- **Decision:** Start with passive buzzer + PWM
- **Reasoning:**
  - Simplicity: No additional audio circuitry required
  - Educational: Direct PWM control teaches fundamentals
  - Adequate: Good enough to demonstrate theremin concept
  - Upgradeable: Clear path to DAC + amplifier later
- **Trade-off accepted:** Lower audio quality in exchange for simplicity

**Development Strategy: Simulation-First**
- **Decision:** Use Wokwi for initial prototyping
- **Reasoning:**
  - Risk reduction: Test logic before hardware investment
  - Rapid iteration: Faster than physical breadboarding
  - Educational: See circuit and code together
  - Cost: Can validate approach before buying components
- **Limitation:** May need to adapt from simulated sensors to real VL53L0X

## Milestone Tracking

### Milestone 1: Development Environment Ready
**Target:** Phase 0 completion
**Status:** Not started
**Criteria:**
- PlatformIO project created and builds successfully
- Can upload to ESP32 (real or simulated)
- Wokwi simulation launches without errors

### Milestone 2: Single Sensor Working
**Target:** Phase 1 completion
**Status:** Not started
**Criteria:**
- One VL53L0X sensor reads distance correctly
- Serial output shows stable readings
- Basic PWM tone generated successfully

### Milestone 3: Dual Sensor Operation
**Target:** Phase 2 completion
**Status:** Not started
**Criteria:**
- Both sensors initialized at different addresses
- Both sensors readable simultaneously
- No I2C bus conflicts or hangs

### Milestone 4: Basic Theremin Functionality
**Target:** Phase 3 completion
**Status:** Not started
**Criteria:**
- Hand movement over pitch sensor changes frequency
- Hand movement over volume sensor changes volume
- Both controls work simultaneously and independently
- Latency imperceptible (<50ms)

### Milestone 5: Polished Theremin
**Target:** Phase 4 completion
**Status:** Not started
**Criteria:**
- Readings smoothed, no jitter
- Calibrated ranges feel natural to play
- Robust error handling prevents crashes
- Can play continuously without issues

### Milestone 6: Project Complete
**Target:** Phase 5 (optional features)
**Status:** Not started
**Criteria:**
- All "Must Have" requirements met
- All "Should Have" features implemented
- Project documented and repeatable
- Success criteria from project brief satisfied

## Next Actions

### Immediate (Next Session)
1. Check if PlatformIO is installed in VSCode
2. Check if Wokwi license is active
3. Decide: Start with simulation or go straight to hardware?
4. Create PlatformIO project structure
5. Write initial platformio.ini configuration

### Short Term (Phase 0)
1. Set up complete development environment
2. Create Wokwi simulation (if using)
3. Implement and test basic sensor reading logic
4. Validate audio generation concept
5. Build confidence with tools before hardware

### Medium Term (Phases 1-3)
1. Acquire and test real hardware components
2. Solve I2C dual-sensor addressing
3. Implement complete pitch and volume control
4. Achieve basic playable theremin

### Long Term (Phases 4-5)
1. Refine and optimize performance
2. Add optional enhancements
3. Document lessons learned
4. Consider upgrade paths (DAC, better sensors, enclosure)

## Metrics & Measurements

### Performance Targets
- **Latency:** <50ms (gesture to sound)
- **Sensor update rate:** ~50-100 Hz
- **Frequency range:** 100-2000 Hz (verified playable)
- **Volume range:** 0-100% (perceptibly different)
- **Stability:** No crashes during 5-minute continuous operation

### Development Velocity
- **Phase 0:** TBD (environment setup)
- **Phase 1:** TBD (hardware testing)
- **Phase 2:** TBD (dual sensor)
- **Phase 3:** TBD (audio mapping)
- **Phase 4:** TBD (refinement)
- **Total project time:** TBD

*Velocity tracking will be updated as phases complete.*

## Lessons Learned

### Technical Lessons
*To be documented as development progresses*

### Process Lessons
*To be documented as development progresses*

### What Worked Well
*To be documented as development progresses*

### What Could Be Improved
*To be documented as development progresses*

---

**Note:** This progress document will be updated regularly as development proceeds. Check `activeContext.md` for current work focus and recent changes.
