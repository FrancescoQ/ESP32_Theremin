Active Context - ESP32 Theremin

> **📌 Context Alignment:**
> This tracks current work on **Phase 2: PWM → DAC Migration**
> For the full v2.0 roadmap (Phases 0-7), see `/productbrief.md` and `progress.md`

## Current Work Focus

### Project Status
**Phase:** Phase 2 - DAC Migration (IN PROGRESS - DEBUGGING)
**Date:** October 25, 2025
**Goal:** Replace PWM square wave with DAC sine wave synthesis

### Critical Issue: PWM → DAC Migration Debugging Session

**Hardware Status:** ✅ DAC → Amplifier → Speaker verified working
- Simple 440Hz test produces tone (F/F# due to timing, but confirms audio chain works)

**Current Problem:** Audio task running on Core 1 - now completely silent

### What We've Tried (October 25, 2025 Session)

**Attempt 1: Timer Interrupts (FAILED - Crashes)**
- Used `hw_timer_t` with ISR at 10-22 kHz
- Tried static DRAM_ATTR for all oscillator data
- Result: Guru Meditation Error (LoadProhibited) - ISR accessing non-DRAM memory
- Root cause: ESP32 ISR can ONLY access DRAM, but complex class structures still in Flash
- Abandoned approach: Too complex to make all data ISR-safe

**Attempt 2: Loop-Based Generation (FAILED - Ticking)**
- Main loop generates samples, no interrupts
- Generated 10, then 500, then 2000 samples per `update()` call
- Result: "Ticking" sound - audio bursts with gaps
- Root cause: Main loop ~14 Hz (sensor I2C delays) creates gaps between sample bursts
- Even with 2000 samples, tremolo/ticking effect audible

**Attempt 3: Dual-Core Audio Task on Core 0 (PARTIAL - Ticking Layer)**
- Created FreeRTOS task on Core 0 for continuous audio generation
- Main loop (Core 1) handles sensors/OTA
- Result: Background tone with "ticking second layer"
- Theory: Core 0 handles WiFi/BT protocol stack, causing interference

**Attempt 4: Dual-Core Audio Task on Core 1 (CURRENT - SILENT)**
- Moved audio task to Core 1 with priority 2 (higher than main loop priority 1)
- Theory: Avoid WiFi interference from Core 0
- **Result: Completely silent - no audio output at all**
- Status: Need to debug why audio stopped

### Current Code State

**Files modified:**
- `include/Oscillator.h` - Oscillator class with static DRAM wavetable, 10 kHz sample rate
- `src/Oscillator.cpp` - Sine wavetable generation, phase accumulation
- `include/AudioEngine.h` - Removed timer, added FreeRTOS task, volatile state variables
- `src/AudioEngine.cpp` - Audio task on Core 1 priority 2, continuous `generateSample()` loop
- `src/main.cpp` - OTA restored with proper constructor

**Architecture:**
```
Core 1 (Application Core):
├── Audio Task (Priority 2) - audioTask() infinite loop calling generateSample()
└── Main Loop (Priority 1) - Theremin.update() → sensors + AudioEngine.update() smoothing

Core 0 (Protocol Core):
└── WiFi/BT stack (OTA when active)
```

**Key Implementation Details:**
- Oscillator: 256-sample sine LUT, static DRAM_ATTR for ISR safety (legacy from interrupt attempt)
- Sample rate: 10,000 Hz configured (SAMPLE_RATE constant)
- AudioEngine: `volatile` state variables for thread safety
- Audio task: Infinite `while(true)` loop, no delay, maximum sample rate
- Main loop: Only calls `AudioEngine::update()` for amplitude smoothing

**Build Status:**
- ✅ Compiles: RAM 14.6% (47,808 bytes), Flash 64.6% (846,941 bytes)
- ✅ Uploads successfully
- ❌ Audio: Silent (no output)

## Next Session Actions

### Immediate Debug Steps

1. **Check if audio task is actually running:**
   - Add debug prints in `audioTask()` loop (e.g., every 10000 samples)
   - Verify task created successfully
   - Check for watchdog timeouts

2. **Verify generateSample() is being called:**
   - Add counter in `generateSample()` and print periodically
   - Check if DAC writes are happening
   - Monitor with oscilloscope if available

3. **Check for task starvation:**
   - Audio task priority 2 might be too high, starving watchdog
   - Try priority 1 (same as main loop)
   - Add `taskYIELD()` or tiny `delayMicroseconds(1)` in audio task

4. **Verify oscillator state:**
   - Check if `frequency` is set correctly (not 0)
   - Check if `smoothedAmplitude` is non-zero
   - Print oscillator values from audio task

5. **Try removing OTA temporarily:**
   - Disable `#ifdef ENABLE_OTA` to see if WiFi interfering
   - Test with minimal main loop

### Alternative Approaches to Consider

**Option A: Hybrid Timer + Core 1**
- Use timer interrupt BUT with minimal ISR (just set flag)
- Audio task on Core 1 waits for flag, generates batch of samples
- Avoids DRAM issues while maintaining timing

**Option B: ESP32 I2S DAC Mode**
- Use I2S peripheral in DAC mode (DMA-driven)
- Pre-fill buffer, DMA handles output
- Professional approach, more complex setup

**Option C: Accept Lower Sample Rate**
- Use timer interrupt at 2-4 kHz (lower than Nyquist but might work)
- Simpler interrupt, less DRAM pressure
- Test if 4 kHz is "good enough" for 220-880 Hz range

**Option D: Go Back to PWM**
- Accept that PWM square wave is "good enough" for Phase 2
- Move forward with other v2.0 features (oscillators, UI, effects)
- Revisit DAC in later phase when more experienced

## Important Patterns & Preferences

### Development Philosophy
- **Incremental progress:** Each step should produce testable result
- **Hardware verification first:** Simple tests (like 440Hz) before complex features
- **When stuck:** Step back, simplify, verify fundamentals
- **Documentation:** Capture what we tried so we don't repeat mistakes

### Code Style
- Object-oriented with clear separation of concerns
- Volatile variables for shared data between tasks/cores
- Static DRAM_ATTR for ISR-accessed data (if we return to interrupts)
- Debug prints during development (can remove later)

### Error Handling Strategy
- If audio fails, log to Serial and continue (don't crash)
- Graceful degradation (e.g., fall back to PWM if DAC fails)
- Clear initialization messages

## Learnings & Project Insights

### ESP32 Audio Architecture Challenges

**ISR Memory Access is HARD:**
- ESP32 ISRs can ONLY access DRAM (not Flash)
- C++ class instances often stored in Flash by default
- Making everything ISR-safe requires:
  - Static class members with DRAM_ATTR
  - All accessed data explicitly placed in DRAM
  - Complex and error-prone

**FreeRTOS Task Considerations:**
- ESP32 dual-core allows parallel processing
- Core 0: Protocol CPU (WiFi/BT)
- Core 1: Application CPU (Arduino code)
- Task priorities matter (1 = default loop, 2 = higher priority)
- Watchdog must be fed (either by yielding or short execution time)

**Sample Rate Reality Check:**
- Theoretical max: ~240 MHz CPU / instructions per sample
- Practical max: ~100-200 kHz with simple synthesis
- Our target: 10 kHz (more than enough for 880 Hz max frequency)
- Nyquist theorem: Need 2× max frequency (so 1.76 kHz minimum)

### What Worked vs. What Didn't

**✅ Worked:**
- Simple 440Hz test in main loop (verified hardware chain)
- Oscillator class with wavetable generation
- Dual-core task creation (task starts, just doesn't output audio)

**❌ Didn't Work:**
- Timer interrupts (DRAM access violations)
- Loop-based generation (gaps create tremolo)
- Core 0 audio task (WiFi interference?)
- Core 1 audio task (silent - current mystery)

**🤔 Learned:**
- ESP32 audio is non-trivial (professionals use I2S + DMA for good reason)
- Our oscillator code is solid (verified in simple test)
- Issue is in audio task scheduling/execution, not synthesis logic

## Context for Next Session

**Current Mystery: Why is Core 1 audio task silent?**

Possibilities:
1. Task not actually running (created but suspended?)
2. Task running but generateSample() not being called
3. generateSample() called but DAC not updating
4. Watchdog killing task (no yield, tight loop)
5. Priority issue (task starving or being starved)
6. Core 1 contention with main loop

**Key Files:**
- `src/AudioEngine.cpp` - Audio task implementation (line 60: audioTask())
- `include/AudioEngine.h` - Task handle and declarations
- `src/Oscillator.cpp` - Sample generation (should be working, tested separately)

**Quick Resume Checklist:**
- [ ] Read this activeContext.md fully
- [ ] Review src/AudioEngine.cpp audioTask() function
- [ ] Add debug prints to verify task is running
- [ ] Test with oscilloscope if available
- [ ] Consider simpler approaches if debugging takes too long

## Notes & Reminders

**What We Know For Sure:**
- ✅ DAC hardware works (simple 440Hz test confirmed)
- ✅ Oscillator synthesis works (wavetable generation correct)
- ✅ Code compiles and uploads successfully
- ✅ No crashes (unlike timer interrupt approach)
- ❌ Audio task on Core 1 produces no sound

**Don't Forget:**
- User has working hardware (DAC → amp → speaker)
- User is patient but getting fatigued with debugging
- Simple working solution > complex broken solution
- Can always return to PWM if DAC proves too complex

**Success Criteria:**
- Continuous smooth tone (no ticking/tremolo)
- Hand movement controls pitch (220-880 Hz)
- Volume sensor controls amplitude
- No crashes, no watchdog resets
- Acceptable latency (<50ms)
