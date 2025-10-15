# Active Context - ESP32 Theremin

## Current Work Focus

### Project Status
**Phase:** Initial setup - Memory Bank initialization
**Date:** October 15, 2025

This is a brand new project. The memory bank has just been initialized to establish the foundational documentation structure. No code has been written yet, and no hardware has been assembled.

### Immediate Next Steps

1. **Development Environment Setup**
   - Install PlatformIO extension in VSCode (if not already installed)
   - Verify Wokwi plugin and active license status
   - Create new PlatformIO project for ESP32
   - Configure platformio.ini with required settings

2. **Virtual Prototyping with Wokwi**
   - Create diagram.json for virtual circuit
   - Create wokwi.toml configuration
   - Test basic ESP32 functionality in simulation
   - Prototype sensor reading logic (possibly with HC-SR04 if VL53L0X unavailable)
   - Validate audio generation concept

3. **Component Acquisition**
   - Order/acquire ESP32 dev board
   - Order/acquire 2x VL53L0X breakout modules
   - Order/acquire passive piezo buzzer
   - Gather breadboard and jumper wires
   - Get 100-220Ω resistor for buzzer

## Recent Changes

**Initial Memory Bank Creation:**
- Created complete memory bank structure
- Documented project brief, product context, system patterns, and technical context
- Established clear architecture and design decisions
- Defined development phases and success criteria

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
- Clear, educational code over clever optimizations
- Extensive comments explaining I2C addressing and PWM concepts
- Meaningful variable names (e.g., `pitchDistance`, `volumeDutyCycle`)
- Modular functions for sensor reading, mapping, audio output

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
