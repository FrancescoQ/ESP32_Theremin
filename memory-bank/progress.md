# Progress - ESP32 Theremin

> **📌 Roadmap Reference:**
> This progress tracker follows the v2.0 development roadmap (Phases 0-7).
> See `/productbrief.md` for complete feature specifications and phase details.

## Current Status

**Project Phase:** Phase 2 - I2S DAC + Oscillator + Continuous Audio 🔨 TESTING & REFINEMENT
**Overall Completion:** ~30-35% (Phase 0 + Phase 1 complete, Phase 2 testing on real hardware)
**Last Updated:** October 27, 2025

### Status Summary
Major breakthrough achieved! Successfully implemented and now testing professional-grade continuous audio generation on **real hardware**:
- **Hardware Deployed**: ESP32 + 2x VL53L0X sensors + I2S DAC - all working on physical device
- **I2S DAC Output**: ESP32 built-in DAC on GPIO25 @ 22050 Hz producing real sound
- **Oscillator Class**: Digital oscillator with phase accumulator and square wave generation
- **Continuous Audio**: FreeRTOS task on Core 1 generates buffers continuously
- **Thread-Safe**: Mutex-protected parameter updates between sensor and audio tasks
- **Smooth Audio**: Zero gaps or stepping in audio output - verified on real hardware
- **Volume Control**: Working correctly (near = loud, far = quiet) - tested in practice
- **Sensor Optimizations**: High-speed timing + optimized reading architecture
- **Both Sensors**: Pitch and volume control functioning on real device

**Build Status:**
- RAM: 47,560 bytes (14.5%) - stable and optimized!
- Flash: 857,041 bytes (65.4%)
- No errors or warnings
- ✅ Running successfully on physical hardware

**Current State:** Phase 2 implemented and undergoing real-world testing and refinement. Hardware assembled and functioning. Sensor latency optimized (85ms → 75ms). Conservative approach: continuing validation before declaring phase complete. User feedback: "much nicer!" - improved responsiveness confirmed on actual device.

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

### Phase 1: Hardware Base Implementation ✅ COMPLETE

**Goal:** Get real sensors working on ESP32

**Status:** ✅ Complete - Hardware assembled and tested on physical device

- [x] **Architecture Foundation**
  - [x] SensorManager class with hardware/simulation modes
  - [x] AudioEngine class with PWM audio generation (later upgraded to DAC)
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

- [x] **Hardware Testing & Deployment (October 2025)**
  - [x] Acquired ESP32 development board
  - [x] Acquired 2x VL53L0X ToF sensor modules
  - [x] Physical hardware assembled and wired
  - [x] ESP32 successfully programmed via USB
  - [x] VL53L0X XSHUT sequential initialization working on hardware
  - [x] I2C communication stable on physical device
  - [x] Both sensors reading distances correctly on real hardware
  - [x] Pitch control functional (sensor → frequency changes)
  - [x] Volume control functional (sensor → amplitude changes)

**Success Criteria:**
- ✅ Clean architecture implemented
- ✅ Both sensors read distances correctly on physical device
- ✅ Hand movement changes frequency and volume on real hardware
- ✅ No I2C errors or crashes during extended testing

---

### Phase 2: I2S DAC + Oscillator + Continuous Audio 🔨 TESTING & REFINEMENT

**Goal:** Replace PWM with real audio output (DAC + continuous generation)

**Status:** 🔨 Implemented and testing on real hardware - Conservative validation phase

- [x] **Oscillator Class Implementation (October 27, 2025)**
  - [x] Created Oscillator class (include/Oscillator.h + src/Oscillator.cpp)
  - [x] Implemented digital oscillator with phase accumulator
  - [x] Square wave generation working
  - [x] Octave shifting functionality
  - [x] Tested on real hardware - producing actual sound!

- [x] **I2S DAC Audio Output (October 27, 2025)**
  - [x] Replaced PWM with ESP32 internal DAC (GPIO25)
  - [x] Implemented 22050 Hz sample rate
  - [x] FreeRTOS audio task on Core 1 for continuous generation
  - [x] Thread-safe parameter updates with mutex protection
  - [x] Zero audio gaps - perfectly smooth continuous audio verified on hardware
  - [x] Volume control functional (near = loud, far = quiet) tested in practice

- [x] **Sensor Optimizations (October 27, 2025)**
  - [x] Exponential smoothing (EWMA with alpha=0.3) implemented
  - [x] Floating-point frequency mapping for sub-Hz precision
  - [x] High-speed timing budget (20ms per sensor vs 33ms default)
  - [x] Optimized reading architecture with updateReadings() caching
  - [x] Total latency reduced from ~85ms to ~75ms
  - [x] User feedback: "much nicer!" - improved responsiveness confirmed

- [x] **Real Hardware Validation (October 2025 - Ongoing)**
  - [x] System running on physical device
  - [x] Both sensors controlling pitch and volume
  - [x] DAC producing real audio output
  - [x] Continuous audio generation verified - no gaps or glitches
  - [x] Extended testing in progress for final validation
  - [~] Conservative approach: continuing real-world testing before marking complete

- [ ] **Display for Monitoring (Deferred to Phase 3)**
  - [ ] Connect SSD1306 OLED (I2C address 0x3C)
  - [ ] Create DisplayManager class
  - [ ] Show real-time metrics

**Success Criteria:**
- ✅ Clean DAC audio output (no crackling/distortion) - verified on hardware
- ✅ Continuous audio with zero gaps - confirmed in practice
- ✅ Both sensors functional - tested extensively
- ✅ Smooth and responsive control - user confirmed "much nicer!"
- 🔨 Final validation and refinement ongoing before declaring complete

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

**Minor Pitch Stepping (Acceptable - October 27, 2025):**
- **Status:** ✅ Significantly improved, user-confirmed acceptable
- **Cause:** VL53L0X sensor returns integer millimeters (1mm = ~1.9 Hz steps)
- **Solution Implemented:** Exponential smoothing (EWMA with alpha=0.3) + float frequency mapping + sensor optimizations
- **Improvement:**
  - Smoothing: 65ms latency reduction (100ms → 35ms), sub-Hz precision
  - Sensor timing: ~10ms latency reduction (85ms → 75ms total)
- **User Feedback:** "Still a bit stepping, but I can live with it for now" → "much nicer!" after sensor optimizations
- **Remaining:** Minor stepping still audible but acceptable for current phase
- **Future Work:** Non-blocking sensor API, predictive filtering, adaptive smoothing available if needed
- **Documentation:** See PITCH_SMOOTHING_IMPROVEMENTS.md for complete tuning guide

### Resolved Issues

**Audio Stepping/Choppy (RESOLVED - October 27, 2025):**
- ✅ Fixed with FreeRTOS audio task on Core 1
- Audio now perfectly smooth and continuous

**Pitch Stepping (IMPROVED - October 27, 2025):**
- ✅ Improved from ~1.9 Hz integer steps to sub-Hz precision
- Exponential smoothing + float mapping significantly reduced stepping

**Sensor Latency (OPTIMIZED - October 27, 2025):**
- ✅ Reduced total latency by ~10ms (85ms → 75ms)
- High-speed timing budget: 20ms per sensor (vs 33ms default)
- Optimized reading architecture with updateReadings() caching
- User feedback: "much nicer!" - improved responsiveness confirmed
- Foundation laid for future non-blocking sensor implementation

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
