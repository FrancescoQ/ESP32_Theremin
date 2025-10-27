# Active Context - ESP32 Theremin

> **📌 Context Alignment:**
> This tracks current work on the **Phase 1 foundation**.
> For the full v2.0 roadmap (Phases 0-7), see `/productbrief.md` and `progress.md`.

## Current Work Focus

### Project Status
**Phase:** Phase 2 - I2S DAC + Oscillator Implementation ✅ (with known issue)
**Date:** October 27, 2025
**v2.0 Vision:** Multi-oscillator synthesizer with effects, professional I/O, and visual feedback

**Major Milestone Achieved:** I2S DAC + Oscillator Architecture Complete!

Successfully transitioned from PWM buzzer to professional I2S DAC output with modular oscillator architecture:
- **Oscillator Class** (include/Oscillator.h + src/Oscillator.cpp): Digital oscillator with phase accumulator, square wave generation, octave shifting
- **AudioEngine Updated**: Now uses I2S in built-in DAC mode (GPIO25 @ 22050 Hz)
- **PWM Removed**: All legacy PWM code cleanly removed
- **Frequency Range**: 220-880 Hz (A3-A5, 2 octaves exactly)

**Current Status:** ✅ Audio working, ⚠️ Steppy/choppy sound (known issue)

**Test Results (October 27, 2025):**
- ✅ I2S DAC outputs audio on GPIO25
- ✅ Oscillator generates square wave
- ✅ Frequency control works (sensors → frequency changes)
- ✅ Volume control works (sensors → amplitude changes)
- ⚠️ Audio is "steppy" - gaps between buffer fills cause discontinuities

**Root Cause of Steppy Audio:**
Current architecture fills I2S buffer only when `update()` is called from main loop. Between calls, there are gaps (~60ms for sensor reads + processing), causing audible stuttering.

**Next Priority:** Implement continuous audio generation via FreeRTOS task or timer interrupt.

**Major Milestone Achieved:** Architecture Refactoring Complete!

The project has been transformed from a monolithic 250-line main.cpp into a clean, modular, object-oriented architecture with separate classes:
- **SensorManager** (include/SensorManager.h + src/SensorManager.cpp): Handles all sensor input (simulation and hardware)
- **AudioEngine** (include/AudioEngine.h + src/AudioEngine.cpp): Manages audio synthesis (currently PWM, designed for DAC upgrade)
- **Theremin** (include/Theremin.h + src/Theremin.cpp): Coordinates sensors and audio

**Key Achievement:**
- Code now organized into reusable, testable classes with clear separation of concerns
- main.cpp reduced to just ~40 lines
- Architecture is future-proof and ready for v2.0 features:
  - DAC audio with wavetable synthesis
  - Multiple oscillators with AudioMixer
  - Effects chain (Delay, Chorus, Reverb)
  - OLED display with DisplayManager
  - LED meters and visual feedback
  - Professional I/O (line-out, amp control)

### Immediate Next Steps

1. **Test Phase 1 Foundation**
   - Test in Wokwi simulator to verify refactored code works
   - Verify all functionality intact after refactoring
   - Check serial output format unchanged
   - Validate sensor smoothing still working
   - Build for hardware when components acquired

2. **Prepare for Phase 2 (v2.0 Begins)**
   - Design Oscillator class with wavetable generation
   - Design AudioMixer class for multiple oscillators
   - Plan DAC output implementation in AudioEngine
   - Research SSD1306 display integration
   - Order Phase 2 components (PAM8403, speaker, display)

3. **Future v2.0 Architecture Extensions**
   - Oscillator class hierarchy for different waveforms (sine/square/saw)
   - AudioMixer for summing multiple oscillator outputs
   - EffectsChain class (Delay, Chorus, optional Reverb)
   - DisplayManager class for OLED integration
   - SwitchController class for MCP23017 GPIO expander
   - LEDMeter class for WS2812B visual feedback

## Recent Changes

**OTA Firmware Update System Implemented (October 20, 2025):**
- Created OTAManager class (include/OTAManager.h + src/OTAManager.cpp)
- Implemented WiFi Access Point mode (ESP32 creates "Theremin-OTA" network)
- Integrated ElegantOTA library v3.1.7 for web interface
- Fixed IP address: 192.168.4.1/update
- HTTP Basic Authentication for security (admin/theremin)
- Conditional compilation with `#ifdef ENABLE_OTA` for zero-overhead when disabled
- Non-blocking operation - theremin continues playing during OTA
- **Optional button activation feature** - saves RAM when OTA not needed
  - OTA_ENABLE_PIN macro in main.cpp (-1 = always on, >=0 = check button)
  - Button logic encapsulated in OTAManager.begin()
  - Saves ~50-70KB RAM when button not pressed during boot
  - Active LOW with internal pullup (connect button between GPIO and GND)
- Created comprehensive OTA_SETUP.md documentation
- Updated all project documentation (ARCHITECTURE.md, productbrief.md, progress.md, README.md)
- **Build Status:** ✅ Compiles successfully (RAM: 48KB, Flash: 847KB)
- **Key Benefits:** Wireless updates when enclosed + RAM savings with button activation

**Product Brief Updated to v2.0 (October 19, 2025):**
- Root `/productbrief.md` now describes complete v2.0 vision (Phases 0-7)
- Includes multi-oscillator architecture, effects chain, professional I/O
- BOM, performance budgets, and detailed technical specifications
- Memory-bank files updated to reflect v1.0 → v2.0 evolution
- Current Phase 1 foundation positioned as stepping stone to v2.0

**Major Architecture Refactoring (October 18, 2025):**
- Extracted all sensor logic into SensorManager class (header + implementation)
- Extracted all audio logic into AudioEngine class (header + implementation)
- Created Theremin coordinator class to manage sensors and audio
- Simplified main.cpp from 250+ lines to ~60 lines (with OTA)
- Added comprehensive ARCHITECTURE.md documentation
- **Build Status:** ✅ Compiles successfully for simulation
- All functionality preserved, but now organized and extensible
- Foundation ready for v2.0 expansion

## Active Decisions

### Phase 1 Design Choices (Current)
1. **Sensor Selection:** VL53L0X Time-of-Flight sensors
   - Reason: High precision (±3%), no interference, fast reading (<30ms)
   - Trade-off: More expensive than ultrasonic, but worth it for quality
   - v2.0 note: Same sensors will work for advanced features

2. **Audio Output:** Start with passive buzzer + PWM
   - Reason: Simplest implementation for learning Phase 1 fundamentals
   - **v2.0 upgrade path:** ESP32 DAC → PAM8403 amp → speaker + line-out
   - Clear migration strategy in AudioEngine class

3. **Development Approach:** Virtual prototyping first
   - Reason: Validate logic before hardware assembly
   - Risk mitigation: Avoid damaging components during learning
   - **Status:** ✅ Wokwi simulation ready

4. **I2C Address Management:** XSHUT pin method
   - Reason: Simpler than multiplexer for 2 sensors
   - Implementation: Sequential initialization with address reassignment
   - v2.0 note: Will add MCP23017 expander, SSD1306 display on same bus

### v2.0 Strategic Decisions
5. **Modular Architecture:** Separate classes for each subsystem
   - Reason: Easier to add v2.0 features without rewriting
   - **Benefit:** Can test SensorManager, AudioEngine independently
   - Future: Easy to add Oscillator, EffectsChain, DisplayManager classes

6. **Incremental Feature Addition:** Checkpoints after each phase
   - Reason: Monitor CPU/RAM usage before adding more features
   - Critical checkpoints: After Phase 2 (1 osc), Phase 3 (2-3 osc), Phase 4 (effects)
   - Plan B ready: Drop from 3 to 2 oscillators if CPU >80%

7. **Performance Budget:** Reserve CPU headroom
   - Target: <75% CPU usage sustained
   - Latency: <20ms sensor-to-sound
   - Audio dropout rate: 0 (critical)
   - See `/productbrief.md` Section 5 for detailed budgets

### Open Questions
- When to acquire Phase 2 components (DAC amp, speaker, OLED)?
- Which specific ESP32 board variant (standard DevKit sufficient)?
- Physical layout: Sensor positioning for ergonomics?
- Enclosure design: Wait until Phase 6 or prototype earlier?
- MCP23017 vs direct GPIO: When to add expander?

## Important Patterns & Preferences

### Code Style
- Object-oriented design with clear separation of concerns
- Each class has single, well-defined responsibility
- Public interfaces documented with comments
- Meaningful class and method names (e.g., `SensorManager::getPitchDistance()`)
- Clean abstractions that hide implementation details
- Future-proof design allowing easy feature additions

### Development Philosophy
- Build incrementally: test each component before integration
- Fail gracefully: handle errors without crashing
- Debug visibility: comprehensive serial output during development
- Documentation first: understand requirements before coding

### Error Handling Strategy
- Log all errors to Serial
- Use last valid reading on sensor timeout
- Continue operation despite single sensor failure if possible
- Clear initialization failure messages

## Learnings & Project Insights

### Architectural Insights

**Modular Design Benefits (Phase 1 Foundation Complete):**
Successfully refactored from monolithic code to modular architecture:
- **Separation of Concerns**: Each class handles one aspect (input, output, coordination)
- **Testability**: Can test SensorManager without AudioEngine and vice versa
- **Extensibility**: Adding v2.0 features requires minimal changes to existing code
- **Maintainability**: Easy to locate and fix bugs in specific subsystems
- **Reusability**: Classes can be used in other ESP32 projects

**Class Design Pattern:**
The three-layer architecture works well:
1. **SensorManager** - Input abstraction layer
2. **AudioEngine** - Output abstraction layer
3. **Theremin** - Business logic / coordination layer

This mirrors MVC pattern and makes v2.0 expansion straightforward.

**v2.0 Extension Path (Planned Architecture):**
Adding new features is now clean:
- **Phase 2:** Oscillator class with wavetable generation → Integrate into AudioEngine
- **Phase 2:** DisplayManager class for OLED → Add to Theremin coordinator
- **Phase 3:** AudioMixer class → Sum multiple Oscillator outputs in AudioEngine
- **Phase 3:** SwitchController class for MCP23017 → Add to Theremin for user controls
- **Phase 4:** EffectsChain class (Delay, Chorus, Reverb) → Insert between Oscillator and output
- **Phase 4:** LEDMeter class for WS2812B strips → Add to Theremin for visual feedback

See `/productbrief.md` Section 4.4 for complete v2.0 class structure.

### Key Technical Insights

**I2C Multi-Device Challenge:**
Both VL53L0X sensors ship with identical I2C address (0x29). This is a common issue with I2C sensors. The XSHUT (shutdown) pin provides elegant solution:
1. Keep both sensors in shutdown
2. Enable first sensor, change its address to 0x30
3. Enable second sensor at default 0x29
4. Both now independently addressable

**PWM Audio Limitations:**
Passive buzzer with PWM provides only square wave. This means:
- Harsh, buzzy sound (not smooth like sine wave)
- Limited volume control (duty cycle has minimal effect on perceived loudness)
- Still adequate for demonstrating theremin concept
- Clear upgrade path exists (DAC + amplifier)

**Latency Considerations:**
Each VL53L0X reading takes ~20-30ms. With two sensors in sequence, that's ~50ms minimum loop time. This is acceptable for musical control (humans perceive latency >50-100ms as laggy). No optimization needed at this stage.

**Smoothing vs. Responsiveness Trade-off:**
Moving average filter (5 samples) provides stability but adds ~100ms latency (5 readings × 20ms each). This is acceptable trade-off for eliminating jitter. Can adjust SAMPLES constant if too sluggish.

### Development Strategy Insights

**Simulation Benefits:**
Wokwi allows testing without hardware:
- Validate I2C communication logic
- Test distance-to-frequency mapping
- Debug initialization sequence
- Build confidence before hardware investment

**Limitation:** If VL53L0X not available in Wokwi, HC-SR04 serves as functional equivalent for logic testing. Core concepts (read sensor → map value → generate audio) remain identical.

**Phased Approach Rationale:**
Don't attempt everything at once:
1. Single sensor + serial output = validate sensor reading
2. Single sensor + audio = validate audio generation
3. Dual sensors = solve I2C addressing
4. Full integration = combine all parts
5. Refinement = smoothing, calibration, optimization

This approach isolates problems and makes debugging tractable.

## Context for Next Session

When resuming work on this project:

1. **Read this activeContext.md first** - it contains current state
2. **Check progress.md** - see what's been completed
3. **Review systemPatterns.md** - understand architecture before coding
4. **Refer to techContext.md** - for library usage and configuration details

### Quick Start Checklist
- [ ] Verify PlatformIO is installed
- [ ] Verify Wokwi access (if using simulation)
- [ ] Check if hardware has been acquired
- [ ] Review which phase we're in (see progress.md)
- [ ] Check for any code files in project directory

### Key Files to Monitor
- `platformio.ini` - PlatformIO configuration (will be created)
- `src/main.cpp` - Main Arduino code (will be created)
- `diagram.json` - Wokwi circuit diagram (will be created if using Wokwi)
- `wokwi.toml` - Wokwi configuration (will be created if using Wokwi)

## Notes & Reminders

**Critical Path Items:**
- XSHUT initialization MUST happen before sensor.begin() calls
- Sensor addresses must be set in correct order (0x30 first, then 0x29)
- PWM channel must be set up before ledcWriteTone() calls
- Serial.begin() should be first line in setup() for debug output

**Common Pitfalls to Avoid:**
- Don't confuse active vs. passive buzzers (need PASSIVE)
- Don't forget 100Ω series resistor for buzzer protection
- Don't use delay() in tight loops (increases latency)
- Don't call ledcWriteTone() without ledcSetup() first
- Don't assume VL53L0X readings are instantaneous (they take time)

**Success Indicators:**
- Both sensors initialize without errors
- Serial output shows stable distance readings
- Frequency changes smoothly with hand movement
- Volume control works independently
- No I2C bus hangs or timeouts
- System runs continuously without crashing
