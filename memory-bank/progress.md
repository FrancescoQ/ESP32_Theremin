# Progress - ESP32 Theremin

## Current Status

**Project Phase:** Phase 0 - Complete ✅
**Overall Completion:** 25% (Simulation environment fully functional)
**Last Updated:** October 15, 2025

### Status Summary
Wokwi simulation environment is complete and ready for testing! The project successfully implements a clean architecture using conditional compilation to support both simulation (potentiometers) and hardware (VL53L0X sensors) from a single source file. The firmware builds successfully and is ready to run in Wokwi simulator.

**Current State:** Ready to test simulation, then transition to hardware when components are acquired

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

### Phase 0: Environment Setup and Simulation ✅ COMPLETE
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

- [x] **Code Implementation with Clean Architecture**
  - [x] Implemented conditional compilation with `#ifdef WOKWI_SIMULATION`
  - [x] Created simulation-specific functions: `simulationSetup()`, `simulationReadPitch()`, `simulationReadVolume()`
  - [x] Created hardware-specific functions: `hardwareSetup()`, `hardwareReadPitch()`, `hardwareReadVolume()`
  - [x] Implemented shared audio synthesis logic in `processAndPlayAudio()`
  - [x] Added smoothing filter (5-sample moving average)
  - [x] Implemented ADC-to-distance conversion for simulation
  - [x] Clean 10-line setup() and loop() functions
  - [x] Comprehensive serial debug output

### Phase 1: Real Hardware Setup and Basic Testing (Not Started)
- [ ] **Component Acquisition**
  - [ ] Acquire ESP32 development board
  - [ ] Acquire 2x VL53L0X ToF sensor modules
  - [ ] Acquire passive piezo buzzer
  - [ ] Acquire breadboard, jumper wires, and resistor

- [ ] **Initial Hardware Tests**
  - [ ] Connect ESP32 to computer, test upload
  - [ ] Test single VL53L0X sensor on breadboard
  - [ ] Run Adafruit VL53L0X example code
  - [ ] Test PWM tone generation with buzzer
  - [ ] Verify I2C bus functionality

### Phase 2: Dual Sensor Control (Not Started)
- [ ] **I2C Address Management**
  - [ ] Connect both sensors to I2C bus
  - [ ] Implement XSHUT pin initialization sequence
  - [ ] Configure sensor #1 to address 0x30
  - [ ] Keep sensor #2 at default address 0x29
  - [ ] Verify both sensors readable simultaneously

- [ ] **Reading Loop**
  - [ ] Implement continuous reading from both sensors
  - [ ] Add error handling for I2C timeouts
  - [ ] Display readings on serial monitor
  - [ ] Verify no bus conflicts or hangs

### Phase 3: Audio Mapping and Integration (Not Started)
- [ ] **Pitch Control Implementation**
  - [ ] Map sensor #1 distance → frequency (100-2000 Hz)
  - [ ] Implement range clamping
  - [ ] Test frequency changes with hand movement
  - [ ] Calibrate distance range for musical playability

- [ ] **Volume Control Implementation**
  - [ ] Map sensor #2 distance → PWM duty cycle (0-255)
  - [ ] Implement range clamping
  - [ ] Test volume changes independently
  - [ ] Calibrate distance range

- [ ] **Integration**
  - [ ] Combine pitch and volume control in main loop
  - [ ] Verify simultaneous independent control
  - [ ] Test latency and responsiveness
  - [ ] Ensure stable operation

### Phase 4: Refinement and Optimization (Not Started)
- [ ] **Signal Smoothing**
  - [ ] Implement moving average filter for pitch sensor
  - [ ] Implement moving average filter for volume sensor
  - [ ] Tune SAMPLES constant for best responsiveness/stability balance
  - [ ] Test jitter elimination

- [ ] **Calibration**
  - [ ] Fine-tune distance ranges for optimal playing
  - [ ] Test different frequency mapping curves (linear vs. exponential)
  - [ ] Adjust timing parameters if needed
  - [ ] Optimize loop delay for best latency/stability

- [ ] **Error Handling**
  - [ ] Implement robust sensor timeout handling
  - [ ] Add recovery from I2C errors
  - [ ] Add initialization retry logic
  - [ ] Test failure modes

### Phase 5: Enhancement (Optional - Not Started)
- [ ] LED visual feedback for sensor status
- [ ] Serial commands for runtime calibration
- [ ] EEPROM preset saving/loading
- [ ] Waveform selection (requires DAC upgrade)
- [ ] Physical enclosure design
- [ ] Ergonomic sensor positioning

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
