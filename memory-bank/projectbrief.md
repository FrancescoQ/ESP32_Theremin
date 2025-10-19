# Project Brief - ESP32 Theremin

> **📌 Note on Project Evolution:**
> This memory-bank documents the **current implementation** (v1.0 foundation, Phase 1).
> For the full **v2.0 vision and roadmap** (Phases 0-7), see `/productbrief.md` in the project root.
> The current work focuses on building a solid Phase 1 foundation that will evolve into the v2.0 multi-oscillator synthesizer.

## Core Identity
**Project Name:** ESP32 Advanced Theremin
**Type:** DIY Performance Electronic Musical Instrument
**Current Phase:** Phase 1 - Foundation Implementation
**Primary Goal:** Create a functional, playable theremin using ESP32 and proximity sensors to control sound frequency and volume in real-time.

## Vision

**v1.0 Foundation (Current):**
Build an educational and playable digital theremin that demonstrates:
- Sensor-based gesture control
- Real-time audio synthesis on embedded systems
- I2C multi-device management
- PWM audio generation
- Clean, modular architecture for future expansion

**v2.0 Evolution (Roadmap):**
Transform into a professional-grade performance instrument with:
- Multiple oscillators (2-3) with selectable waveforms
- Effects chain (Delay, Chorus, optional Reverb)
- Professional I/O (DAC, line-out, amp control)
- Visual feedback (OLED display, LED meters)
- Preset system and advanced controls

See `/productbrief.md` for complete v2.0 specifications and development roadmap.

## Core Requirements

### Must Have
1. **Dual Sensor Control**
   - One sensor controls pitch (frequency)
   - Second sensor controls volume (amplitude)
   - Independent, non-interfering operation

2. **Real-time Audio Output**
   - Minimal latency (<50ms)
   - Continuous frequency range (100-2000 Hz)
   - Volume control (0-100%)

3. **Hardware Configuration**
   - ESP32 microcontroller (any dev board variant)
   - 2x VL53L0X Time-of-Flight laser sensors
   - Passive piezoelectric buzzer output
   - USB or battery power

4. **Stable Operation**
   - Reproducible sensor readings
   - No mutual sensor interference
   - Predictable pitch/volume response

### Should Have
- Reading smoothing for stable control
- Calibrated operating ranges (5-40cm for pitch, 5-30cm for volume)
- Serial debug output for monitoring

### Could Have (Future)
- Different waveform selection
- Preset saving to EEPROM
- LED visual feedback
- Manual calibration interface
- External amplifier output via DAC

## Success Criteria
Project is complete when:
- ✓ Both sensors detect distance accurately
- ✓ Pitch changes continuously and predictably with hand movement
- ✓ Volume is independently controllable
- ✓ Response latency is imperceptible (<50ms)
- ✓ System operates stably and reproducibly

## Technical Constraints
- Power consumption must stay under ESP32 USB limits (~500mA)
- I2C bus speed adequate for <30ms sensor readings
- Must handle dual sensors with identical default I2C addresses
- Audio quality limited by passive buzzer capabilities

## Development Approach
- Use PlatformIO with Arduino framework for development
- Virtual prototyping with Wokwi simulator before hardware assembly
- Phased development: setup → single sensor → dual sensor → audio mapping → refinement
- Prioritize functionality over audio quality
