# Active Context - ESP32 Theremin

> **📌 Context Alignment:**
> This tracks current work on the **Phase 1 foundation**.
> For the full v2.0 roadmap (Phases 0-7), see `/productbrief.md` and `progress.md`.

## Current Work Focus

### Project Status
**Phase:** Phase 2 ✅ COMPLETE - Ready for Phase 3
**Date:** October 29, 2025
**v2.0 Vision:** Multi-oscillator synthesizer with effects, professional I/O, and visual feedback

**Major Milestone Achieved:** Complete Waveform Synthesis Implementation!

Successfully implemented professional-grade audio synthesis with multiple waveforms:
- **Oscillator Class** (include/Oscillator.h + src/Oscillator.cpp): Digital oscillator with phase accumulator, **4 waveform types**, octave shifting
- **Waveforms Available**: Square, Sine (LUT-based), Triangle (mathematical), Sawtooth (mathematical)
- **AudioEngine Updated**: I2S DAC (GPIO25 @ 22050 Hz) + FreeRTOS audio task on Core 1 + proper sample format conversion
- **DAC Format Fix**: Correct unsigned 8-bit output (0-255) for ESP32 built-in DAC
- **PWM Removed**: All legacy PWM code cleanly removed
- **Frequency Range**: 220-880 Hz (A3-A5, 2 octaves exactly)
- **Continuous Audio**: High-priority task generates buffers continuously, independent of sensor timing
- **Thread-Safe**: Mutex-protected parameter updates between sensor and audio tasks

**Current Status:** ✅ Phase 2 complete and stable! Software foundation for 3 oscillators ready. Hardware running perfectly. Ready for Phase 3 expansion (runtime controls + display).

**Test Results (October 27, 2025):**
- ✅ I2S DAC outputs audio on GPIO25
- ✅ Oscillator generates 4 waveforms (square, sine, triangle, sawtooth)
- ✅ Frequency control works (sensors → frequency changes)
- ✅ Volume control works correctly (near sensor = quiet, far = loud - traditional theremin behavior)
- ✅ **Audio is smooth and continuous - NO stepping/gaps!**
- ✅ **NO distortion - proper DAC format conversion**
- ⚠️ Minor pitch stepping (caused by sensor quantization, not audio generation)

**Build Results:**
- RAM: 47,584 bytes (14.5%) - actually improved!
- Flash: 856,877 bytes (65.4%)
- Compiles without errors or warnings

**Pitch Stepping - IMPROVED (October 27, 2025):**
- **Status:** ✅ Significantly improved with exponential smoothing + float mapping
- **User Feedback:** "Still a bit stepping, but I can live with it for now" → "much nicer!" after sensor optimizations
- **Implementation:** EWMA (alpha=0.3) + floating-point frequency calculation
- **Improvement:** 65ms latency reduction (100ms → 35ms smoothing lag)
- **Remaining Issue:** Minor stepping still audible (sensor quantization inherent to 1mm resolution)
- **Future Tuning:** SMOOTHING_ALPHA adjustable in SensorManager.h (0.15-0.4 range)

**Sensor Latency Optimizations - COMPLETED (October 27, 2025):**
- **Status:** ✅ Implemented high-speed sensor mode + optimized reading architecture
- **User Feedback:** "much nicer!" - improved responsiveness confirmed
- **Implementation:**
  - High-speed timing budget: 20ms per sensor (vs 33ms default)
  - Optimized reading architecture with updateReadings() caching method
  - Prevents redundant sensor reads
- **Improvement:** ~10ms latency reduction in sensor reading time
- **Total Latency:** ~75ms (down from ~85ms)
  - Sensor reads: ~40ms (2 sensors × ~20ms each)
  - Smoothing: ~35ms (EWMA with alpha=0.3)
- **Future Optimizations:** Non-blocking sensor API, predictive filtering, adaptive smoothing

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

**Phase 2 Complete - Planning Phase 3**

Current state: Stable, working system with professional audio quality and 3-oscillator software foundation. No immediate work required - system is fully functional.

**When Ready for Phase 3 (Runtime Controls + Display):**

1. **Option A: Add Display First** (simpler, immediate debugging value)
   - Connect SSD1306 OLED (I2C 0x3C)
   - Create DisplayManager class
   - Show real-time CPU/RAM metrics
   - Show current oscillator states
   - Visual feedback for current compile-time settings
   - Helps debug future switch implementation

2. **Option B: Add Runtime Controls** (more ambitious)
   - Order MCP23017 GPIO expander module
   - Acquire rotary switches (3x 4-position) for waveform selection
   - Acquire toggle switches (3x 3-position) for octave control
   - Create SwitchController class with interrupt handling
   - Enable runtime parameter changes instead of compile-time only

3. **Option C: Combined Approach** (likely path)
   - Implement both display and controls together
   - Physical assembly convenience (layout and wiring done once)
   - Test I2C bus with all devices (sensors + expander + display)
   - Complete Phase 3 in one integrated session

**Future Architecture Extensions (Phase 4+):**
   - EffectsChain class (Delay, Chorus, optional Reverb)
   - LEDMeter class for WS2812B visual feedback
   - Professional I/O refinements

## Recent Changes

**Multi-Oscillator Volume Control Implementation (October 27, 2025 - Latest):**
- **Achievement:** Implemented per-oscillator volume control with intelligent mixing!
- **Architecture:** 3 oscillators with independent volume levels
  - Oscillator 1 (SINE): 100% volume (full)
  - Oscillator 2 (SQUARE, -1 octave): 60% volume (quieter sub-bass)
  - Oscillator 3 (OFF): 40% volume (quietest when enabled)
- **Implementation:**
  - Added volume member variable (float, 0.0-1.0) to Oscillator class
  - Added setVolume() method with constraint checking
  - Volume applied in getNextSample() before returning sample
  - Each oscillator scales its own output independently
- **Mixing Strategy:** Simple averaging with automatic clipping prevention
  - Sum all active oscillator samples (after volume scaling)
  - Divide by number of active oscillators
  - No complex normalization needed - math guarantees no clipping
  - Example: Max possible = (32767 + 19660 + 13107) / 3 = 21,845 (well within ±32768 range)
- **Benefits:**
  - Modular design - each oscillator self-contained
  - Runtime adjustable - setVolume() can be called anytime
  - Perfect for presets - combine waveform + volume changes
  - Thread-safe - simple atomic writes
  - Maintains consistent loudness regardless of oscillator count
- **Results:**
  - ✅ Rich, layered sound with balanced mix
  - ✅ No clipping or distortion
  - ✅ Volume ratios preserved correctly
  - ✅ Clean architecture for future expansion
- **Files Modified:**
  - include/Oscillator.h: Added volume member, setVolume() declaration
  - src/Oscillator.cpp: Implemented setVolume(), volume application in getNextSample()
  - src/AudioEngine.cpp: Set initial volumes in constructor
- **Build Status:** ✅ Compiles successfully, no errors/warnings

**Multiple Waveform Implementation (October 27, 2025):**
- **Achievement:** Expanded oscillator to support 4 distinct waveform types!
- **Waveforms Implemented:**
  - **Square**: Hollow, buzzy, only odd harmonics (original)
  - **Sine**: Pure, smooth, no harmonics - 256-entry LUT in PROGMEM (512 bytes Flash)
  - **Triangle**: Mellow, flute-like, weak odd harmonics - mathematical generation
  - **Sawtooth**: Bright, brassy, ALL harmonics - simplest implementation (1 line!)
- **Implementation:**
  - Modified include/Oscillator.h: Added SINE, TRIANGLE, SAW to enum
  - Modified src/Oscillator.cpp: Implemented all waveform generators
  - Sine uses lookup table for efficiency, triangle/saw use direct mathematical formulas
- **Performance:** All waveforms <1% CPU overhead, negligible memory impact
- **Results:**
  - ✅ All 4 waveforms sound clean and distinct
  - ✅ No additional distortion or artifacts
  - ✅ Easy waveform selection via compile-time enum
  - ✅ Foundation for future runtime waveform switching
- **Files Modified:**
  - include/Oscillator.h: Added waveform enum values, method declarations, sine LUT
  - src/Oscillator.cpp: Implemented generateSineWave(), generateTriangleWave(), generateSawtoothWave()
  - docs/improvements/WAVEFORM_IMPLEMENTATION.md: Complete documentation
- **Documentation:** Created comprehensive WAVEFORM_IMPLEMENTATION.md with sound characteristics, technical details, usage guide
- **Build Status:** ✅ Compiles successfully, no errors/warnings

**DAC Format Fix - Distortion Elimination (October 27, 2025):**
- **Problem:** Distortion across all waveforms due to sample format mismatch
- **Root Cause:** ESP32 built-in DAC expects unsigned 8-bit (0-255), code was sending signed 16-bit (-32768 to 32767)
- **Solution:** Proper sample format conversion in AudioEngine.generateAudioBuffer()
  - Convert signed 16-bit to unsigned 8-bit: `(sample >> 8) + 128`
  - Place in upper byte of 16-bit word for I2S DMA alignment
  - Generic fix applies to ALL waveforms equally
- **Results:**
  - ✅ Complete distortion elimination
  - ✅ Clean output on sine, triangle, square, and sawtooth
  - ✅ No performance impact (simple bit operations)
- **Files Modified:**
  - src/AudioEngine.cpp: Updated generateAudioBuffer() with proper sample conversion
  - docs/improvements/DAC_FORMAT_FIX.md: Technical documentation
- **Documentation:** Created DAC_FORMAT_FIX.md explaining ESP32 DAC requirements, conversion process, testing guide
- **Build Status:** ✅ Compiles successfully, audio quality excellent

**Volume Mapping Fix - Traditional Theremin Behavior (October 27, 2025):**
- **Problem:** Volume control reversed (near = loud, far = quiet)
- **Solution:** Inverted amplitude mapping to match traditional theremin
  - Near volume sensor (50mm) = 0% volume (quiet)
  - Far from volume sensor (400mm) = 100% volume (loud)
- **Files Modified:**
  - src/Theremin.cpp: Swapped map() output values (0 and 100) with explanatory comment
- **Results:** ✅ Volume control now matches classic theremin behavior
- **Build Status:** ✅ Compiles successfully, behavior correct

**OTA Preprocessor Fix (October 27, 2025):**
- **Problem:** OTA initialized even when `-DENABLE_OTA=0` was set
- **Root Cause:** `#ifdef ENABLE_OTA` checks if defined (any value), not if true/false
- **Solution:** Changed all `#ifdef ENABLE_OTA` to `#if ENABLE_OTA` in main.cpp
- **Results:**
  - ✅ OTA can be enabled/disabled via build flag value
  - ✅ `-DENABLE_OTA=1` enables OTA
  - ✅ `-DENABLE_OTA=0` disables OTA
- **Files Modified:**
  - src/main.cpp: Changed preprocessor directives
- **Build Status:** ✅ Compiles successfully, OTA enable/disable working correctly

**I2S Error Handling Improvements (October 27, 2025):**
- **Problem:** Poor error visibility if I2S initialization fails
- **Solution:** Enhanced error handling in AudioEngine
  - Changed setupI2S() to return bool (success/failure)
  - Added delay before I2S init to ensure Serial is ready
  - Audio task won't start if I2S initialization fails
  - Clear error messages for I2S driver and DAC mode failures
- **Files Modified:**
  - include/AudioEngine.h: Updated setupI2S() return type
  - src/AudioEngine.cpp: Implemented error handling and reporting
- **Results:** ✅ Better diagnostics, prevents cascading failures
- **Build Status:** ✅ Compiles successfully


**Sensor Latency Optimizations (October 27, 2025 - Latest):**
- **Problem:** Total latency of ~85ms felt slightly sluggish
- **Solution:** High-speed sensor timing + optimized reading architecture
- **Implementation:**
  - Configured both VL53L0X sensors for 20ms timing budget (vs 33ms default)
  - Added SensorManager::updateReadings() method for optimized reading pattern
  - Reads both sensors once per loop, caches results
  - getPitchDistance() and getVolumeDistance() use cached values with smoothing
- **Results:**
  - ✅ ~10ms latency reduction (85ms → 75ms total)
  - ✅ More responsive feel confirmed by user ("much nicer!")
  - ✅ Cleaner code architecture (single update point)
  - ✅ Foundation for future non-blocking sensor implementation
  - ✅ No RAM/Flash increase (47,560 bytes / 857,041 bytes)
- **Files Modified:**
  - include/SensorManager.h: Added updateReadings() method, caching variables
  - src/SensorManager.cpp: Implemented timing budget config and caching
  - src/Theremin.cpp: Updated to call updateReadings() before getting distances
  - PITCH_SMOOTHING_IMPROVEMENTS.md: Documented sensor optimizations
- **Trade-offs:**
  - Slightly reduced max range (still adequate for 50-400mm)
  - Minimal accuracy reduction (negligible for gesture control)
- **Build Status:** ✅ Compiles successfully, no errors/warnings

**Pitch Smoothing Improvements (October 27, 2025):**
- **Problem:** Audible pitch stepping from sensor quantization (1mm = ~1.9 Hz jumps)
- **Solution:** Exponential smoothing + floating-point frequency mapping
- **Implementation:**
  - Replaced 5-sample moving average with EWMA (Exponential Weighted Moving Average)
  - Set SMOOTHING_ALPHA = 0.3 for balanced responsiveness/smoothness
  - Added floating-point mapFloat() function in Theremin class
  - Frequency calculation uses float throughout, only casts to int at final step
- **Results:**
  - ✅ 65ms latency reduction (100ms → 35ms smoothing lag)
  - ✅ Sub-Hz frequency precision (eliminates integer map() quantization)
  - ✅ More responsive than moving average
  - ✅ Reduced RAM usage (removed circular buffer arrays)
  - ✅ User confirmed improvement: "Still a bit stepping, but I can live with it"
- **Files Modified:**
  - include/SensorManager.h: Added EWMA parameters, removed moving average code
  - src/SensorManager.cpp: Implemented applyExponentialSmoothing() method
  - include/Theremin.h: Added mapFloat() declaration
  - src/Theremin.cpp: Implemented mapFloat() for frequency calculation
- **Documentation:** Created PITCH_SMOOTHING_IMPROVEMENTS.md with tuning guide
- **Tuning Options:** SMOOTHING_ALPHA adjustable (0.15-0.4 range)
- **Future Optimizations:** Parallel sensor reading, high-speed mode, predictive filtering

**Continuous Audio Generation via FreeRTOS Task (October 27, 2025):**
- **Problem Solved:** Audio was steppy/choppy with ~68ms gaps between 11.6ms buffers
- **Solution:** Dedicated high-priority audio task on Core 1
- **Implementation:**
  - Modified AudioEngine.h: Added FreeRTOS task support, mutex, task handle
  - Modified AudioEngine.cpp: Audio task continuously generates buffers
  - Thread-safe parameter updates with mutex protection
  - Audio task blocks on i2s_write(), naturally paced at ~11ms intervals
  - Sensor reads on Core 0 update parameters asynchronously
- **Volume Reversal Fix:** Inverted amplitude mapping (near=loud, far=quiet)
- **Results:**
  - ✅ Smooth, continuous audio with zero gaps
  - ✅ RAM improved to 14.5% (47,584 bytes)
  - ✅ Flash: 65.4% (856,877 bytes)
  - ✅ Volume control works correctly
- **Documentation:** Created CONTINUOUS_AUDIO_IMPLEMENTATION.md
- **Minor Issue:** Pitch stepping from sensor quantization (1mm = ~1.9 Hz)
  - Not audio generation issue - sensor resolution limitation
  - Future solutions: increase smoothing, exponential smoothing, interpolation

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

### Documentation Organization

**Documentation Structure (October 27, 2025):**
The project uses an organized `docs/` directory structure to keep the root clean and professional:

```
theremin/
├── README.md                    # Project overview (root only)
├── productbrief.md              # Project vision (root only)
├── docs/
│   ├── README.md                # Documentation index
│   ├── architecture/            # System design decisions
│   ├── guides/                  # How-to and setup instructions
│   └── improvements/            # Implementation notes & optimizations
└── memory-bank/                 # Project knowledge base
```

**File Placement Rules:**
- **Architecture documentation** → `docs/architecture/`
  - System design decisions
  - Class structure explanations
  - Design pattern rationale
  - Example: `ARCHITECTURE.md`

- **Setup/How-to guides** → `docs/guides/`
  - Installation instructions
  - Configuration guides
  - Troubleshooting steps
  - Examples: `OTA_SETUP.md`, `DEBUG_GUIDE.md`, `WOKWI_SIMULATION_PLAN.md`

- **Implementation notes** → `docs/improvements/`
  - Feature implementation details
  - Optimization documentation
  - Issue analysis and solutions
  - Examples: `CONTINUOUS_AUDIO_IMPLEMENTATION.md`, `PITCH_SMOOTHING_IMPROVEMENTS.md`

**Naming Conventions:**
- Use `SCREAMING_SNAKE_CASE.md` for all documentation files
- Be descriptive but concise
- Include date in file content (not filename)

**When Creating New Documentation:**
1. Choose the appropriate `docs/` subdirectory
2. Create the file with clear structure:
   - Title and implementation date
   - Problem/goal statement
   - Solution details with code examples
   - Results and measurements
   - Related files
3. **Update `docs/README.md`** to add the new file to the index
4. Cross-reference from related docs if applicable

**Root Directory:**
Only `README.md` and `productbrief.md` belong in project root. All other documentation goes in `docs/`.

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
