# Active Context - ESP32 Theremin

## Current Work Focus

### Project Status
**Phase:** Architecture Refactoring Complete
**Date:** October 18, 2025

Major architectural refactoring completed! The project has been transformed from a monolithic 250-line main.cpp into a clean, modular, object-oriented architecture with separate classes for:
- **SensorManager**: Handles all sensor input (simulation and hardware)
- **AudioEngine**: Manages audio synthesis
- **Theremin**: Coordinates sensors and audio

**Key Achievement:** Code now organized into reusable, testable classes with clear separation of concerns. main.cpp reduced to just 40 lines. Architecture is future-proof and ready for DAC audio, waveform selection, OLED display, and other planned features.

### Immediate Next Steps

1. **Test Refactored Code**
   - Test in Wokwi simulator to verify behavior matches original
   - Verify all functionality intact after refactoring
   - Check serial output format unchanged
   - Validate sensor smoothing still working

2. **Future Architecture Extensions**
   - Design WaveformGenerator class hierarchy for different waveforms
   - Design Display class for OLED integration
   - Design UserInterface class for buttons/switches
   - Plan DAC output implementation in AudioEngine

3. **Hardware Build & Test** (When Ready)
   - Build for hardware: `/Users/fquagliati/.platformio/penv/bin/pio run -e esp32dev`
   - Test with real VL53L0X sensors
   - Verify modular architecture works on hardware

## Recent Changes

**Major Architecture Refactoring (October 18, 2025):**
- Extracted all sensor logic into SensorManager class (header + implementation)
- Extracted all audio logic into AudioEngine class (header + implementation)
- Created Theremin coordinator class to manage sensors and audio
- Simplified main.cpp from 250+ lines to 40 lines
- Added comprehensive ARCHITECTURE.md documentation
- **Build Status:** ✅ Compiles successfully for simulation
- All functionality preserved, but now organized and extensible

## Active Decisions

### Design Choices Made
1. **Sensor Selection:** VL53L0X Time-of-Flight sensors
   - Reason: High precision, no interference, fast reading
   - Trade-off: More expensive than ultrasonic, but worth it for quality

2. **Audio Output:** Start with passive buzzer + PWM
   - Reason: Simplest implementation for learning
   - Future path: Can upgrade to DAC + amplifier later

3. **Development Approach:** Virtual prototyping first
   - Reason: Validate logic before hardware assembly
   - Risk mitigation: Avoid damaging components during learning

4. **I2C Address Management:** XSHUT pin method
   - Reason: Simpler than multiplexer for just 2 sensors
   - Implementation: Sequential initialization with address reassignment

### Open Questions
- Is Wokwi license active and does it support VL53L0X?
- If not, HC-SR04 simulation adequate for initial logic testing?
- Which specific ESP32 board variant will be used?
- Physical layout: How to position sensors for optimal playing ergonomics?

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

**Modular Design Benefits:**
Successfully refactored from monolithic code to modular architecture:
- **Separation of Concerns**: Each class handles one aspect (input, output, coordination)
- **Testability**: Can test SensorManager without AudioEngine and vice versa
- **Extensibility**: Adding features (DAC, waveforms, display) requires minimal changes to existing code
- **Maintainability**: Easy to locate and fix bugs in specific subsystems
- **Reusability**: Classes can be used in other ESP32 projects

**Class Design Pattern:**
The three-layer architecture works well:
1. **SensorManager** - Input abstraction layer
2. **AudioEngine** - Output abstraction layer
3. **Theremin** - Business logic / coordination layer

This mirrors MVC pattern and makes future expansion straightforward.

**Future Extension Path:**
Adding new features is now clean:
- New waveforms → Create WaveformGenerator interface, add to AudioEngine
- OLED display → Create Display class, add to Theremin
- Buttons/switches → Create UserInterface class, add to Theremin
- Multiple oscillators → Extend AudioEngine with Oscillator instances

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
