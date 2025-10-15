# Project Brief - ESP32 Theremin

## Core Identity
**Project Name:** ESP32 Theremin
**Type:** DIY Electronic Musical Instrument
**Primary Goal:** Create a functional, playable theremin using ESP32 and proximity sensors to control sound frequency and volume in real-time.

## Vision
Build an educational and playable digital theremin that demonstrates:
- Sensor-based gesture control
- Real-time audio synthesis on embedded systems
- I2C multi-device management
- PWM audio generation

This is an experimental educational project focused on learning ESP32 capabilities, not replicating professional theremin quality.

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
