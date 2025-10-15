# Product Context - ESP32 Theremin

## Why This Exists

### Problem Space
- Traditional theremins are expensive and complex to build (requiring RF oscillator circuits and capacitive antennas)
- Learning embedded audio synthesis and sensor integration needs practical projects
- Understanding I2C multi-device management requires hands-on experience
- Need accessible way to explore gesture-based instrument control

### Solution
A digital theremin using affordable, modern components (ESP32 + laser ToF sensors) that:
- Provides tangible learning experience with sensor-to-audio mapping
- Demonstrates real-time embedded control systems
- Makes theremin-like playing accessible for experimentation
- Serves as foundation for understanding more complex audio projects

## What This Is For

### Primary Purpose
**Educational instrument** for learning:
- ESP32 development with PlatformIO
- I2C sensor management (especially handling address conflicts)
- PWM audio generation
- Real-time signal processing and mapping
- Virtual prototyping with Wokwi

### Secondary Purpose
- Playable experimental instrument for creating eerie theremin-like sounds
- Platform for exploring embedded audio concepts
- Demonstration of gesture-controlled interfaces

## How It Should Work

### User Experience

**Setup Phase:**
1. Power on device via USB or battery
2. System initializes both sensors (brief startup sequence)
3. Ready indicator (optional LED or serial message)

**Playing Phase:**
1. User positions hands over two sensors
2. **Pitch Hand** (Sensor 1): Move hand vertically over first sensor
   - Closer = higher pitch
   - Further = lower pitch
   - Range: ~5-40cm for musical control
   - Frequency response: 100-2000 Hz (approximately 4 octaves)

3. **Volume Hand** (Sensor 2): Move hand over second sensor
   - Closer = louder
   - Further = quieter
   - Range: ~5-30cm
   - Volume response: 0-100%

4. Both hands work independently and simultaneously
5. Sound responds in real-time with imperceptible latency
6. Smooth, continuous control without jitter or jumps

### Expected Behavior

**Normal Operation:**
- Continuous tone generation based on pitch sensor
- Volume modulation responds immediately
- No audible latency between gesture and sound
- Stable readings without erratic jumping
- Predictable, musical response curves

**Edge Cases:**
- Hand too close (<5cm): Maintain minimum/maximum value
- Hand too far (>40cm pitch, >30cm volume): Maintain opposite extreme
- Hand removed completely: Silent or minimum volume
- Both sensors obstructed: System continues safely, returns to normal when clear

**Failure Modes:**
- Sensor init failure: Serial error message, retry or indicate failure
- I2C communication error: Attempt recovery, continue with last good values
- Power issues: Graceful degradation or shutdown

## User Goals

### Immediate Goals
- Generate theremin-like sounds through hand gestures
- Experiment with pitch and volume control independently
- Experience responsive, real-time musical control
- Understand sensor-to-sound mapping

### Learning Goals
- Grasp I2C multi-device addressing concepts
- Understand PWM audio generation
- See practical application of distance sensors
- Learn signal smoothing and calibration techniques
- Experience full embedded development workflow (simulation → prototype → refinement)

## Design Principles

1. **Responsiveness First**: Minimize latency above all else
2. **Stability Through Smoothing**: Balance responsiveness with reading stability
3. **Predictable Mapping**: Linear or intuitive curves for distance-to-parameter conversion
4. **Fail Gracefully**: Handle sensor errors without crashing
5. **Educational Transparency**: Code should be readable and well-documented for learning
6. **Iterative Development**: Build incrementally from simple to complex

## Quality Expectations

### Audio Quality
- **Acceptable**: Basic PWM tones through passive buzzer (square wave)
- **Not Expected**: High-fidelity audio, complex waveforms (without DAC upgrade)
- **Priority**: Responsiveness over audio richness

### Control Precision
- **Required**: Smooth, continuous frequency changes
- **Required**: Perceptible volume control across full range
- **Nice to Have**: Exponential frequency scaling for better musical intervals

### Reliability
- **Critical**: Stable, repeatable operation over extended playing
- **Critical**: No I2C bus conflicts or hangs
- **Important**: Sensor readings don't drift during session
