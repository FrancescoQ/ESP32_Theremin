# System Patterns - ESP32 Theremin

## System Architecture

### High-Level Architecture - Modular Design
```
                ┌─────────────────────────────────────┐
                │   ESP32 Main Loop (setup/loop)      │
                │      Modular Class Structure         │
                └─────────────────────────────────────┘
                              │
                    ┌─────────┴─────────┐
                    │  Core Components  │
                    │  (always enabled) │
                    └─────────┬─────────┘
                              │
            ┌─────────────────┴─────────────────┐
            │                                   │
       ┌────▼────┐                        ┌─────▼─────┐
       │SIMULATION│                       │ HARDWARE  │
       │  MODE    │                       │   MODE    │
       └────┬─────┘                       └─────┬─────┘
            │                                   │
   ┌────────▼────────┐              ┌───────────▼──────────┐
   │ Potentiometers  │              │   VL53L0X Sensors    │
   │ GPIO34/35 (ADC) │              │   I2C @ 0x29/0x30    │
   │ SensorManager   │              │   SensorManager      │
   └────────┬────────┘              └───────────┬──────────┘
            │                                   │
            └──────────┬────────────────────────┘
                       │
          ┌────────────▼────────────┐
          │      AudioEngine        │
          │   Audio Synthesis       │
          └────────────┬────────────┘
                       │
                  ┌────▼────┐
                  │   PWM   │
                  │ GPIO25  │
                  └────┬────┘
                       │
                  ┌────▼────┐
                  │ Buzzer  │
                  └─────────┘

   ┌──────────────────────────────────────┐
   │   Optional: OTAManager               │
   │   (#ifdef ENABLE_OTA)                │
   │                                      │
   │   ┌────────────┐                     │
   │   │ WiFi AP    │                     │
   │   │ 192.168.4.1│                     │
   │   └─────┬──────┘                     │
   │         │                            │
   │   ┌─────▼──────┐                     │
   │   │ ElegantOTA │                     │
   │   │ Web Server │                     │
   │   └────────────┘                     │
   └──────────────────────────────────────┘
```

### Simulation Architecture (Wokwi)
```
┌────────────────────────────────────────────┐
│         ESP32 Virtual Circuit              │
│  ┌──────────┐              ┌──────────┐   │
│  │  Pitch   │              │  Volume  │   │
│  │   POT    │              │   POT    │   │
│  │ (Left)   │              │ (Right)  │   │
│  └────┬─────┘              └─────┬────┘   │
│       │                          │         │
│    GPIO34                     GPIO35       │
│       │                          │         │
│  ┌────▼──────────────────────────▼────┐   │
│  │      ESP32 (ADC1 channels)        │   │
│  │  12-bit ADC (0-4095) → mm        │   │
│  └────────────────┬──────────────────┘   │
│                   │GPIO25                 │
│              ┌────▼────┐                  │
│              │ 220Ω R  │                  │
│              └────┬────┘                  │
│              ┌────▼────┐                  │
│              │ Buzzer  │                  │
│              └─────────┘                  │
└────────────────────────────────────────────┘
```

### Hardware Architecture (Physical Theremin)
```
┌─────────────────┐         ┌──────────────┐
│   I2C Bus       │         │  PWM Channel │
│  (GPIO 21/22)   │         │  (GPIO 25)   │
└─────────────────┘         └──────────────┘
         │                           │
         ▼                           ▼
┌─────────────────────┐     ┌──────────────┐
│  VL53L0X Sensor #1  │     │    Passive   │
│  (Pitch @ 0x30)     │     │    Buzzer    │
│                     │     └──────────────┘
│  VL53L0X Sensor #2  │
│  (Volume @ 0x29)    │
└─────────────────────┘
```

### Component Responsibilities

**Main Loop:**
- Orchestrates sensor reading cycle
- Maps sensor data to audio parameters
- Manages timing and latency
- Handles error conditions

**Sensor Manager:**
- I2C communication with both VL53L0X sensors
- Address management (0x29 and 0x30)
- Reading smoothing/filtering
- Range validation and clamping

**Audio Output Manager:**
- PWM tone generation via ledcWriteTone()
- Volume control via ledcWrite()
- Frequency bounds enforcement
- Duty cycle calculation

## Key Technical Decisions

### Decision: VL53L0X Time-of-Flight Sensors
**Rationale:**
- Laser-based ToF provides high precision (±3%)
- No mutual interference between sensors
- Fast reading rate (<30ms)
- Wide usable range (50-500mm)
- Well-supported I2C library available

**Alternatives Considered:**
- HC-SR04 ultrasonic: Too slow, potential interference
- IR sensors: Too imprecise, affected by ambient light
- Capacitive sensing: Complex, would need custom antennas

### Decision: PWM Audio via Passive Buzzer
**Rationale:**
- Simplest implementation for educational project
- No additional audio circuitry required
- Adequate for demonstrating theremin concept
- Direct GPIO control, minimal latency

**Trade-offs:**
- Limited audio quality (square wave only)
- No true volume control (only duty cycle variation)
- Upgrade path: ESP32 DAC + amplifier for future enhancement

### Decision: Dual I2C Address Management via XSHUT
**Rationale:**
- Both VL53L0X modules have same default address (0x29)
- XSHUT pins allow sequential initialization
- One sensor gets new address (0x30), other keeps 0x29
- Both can then operate simultaneously on same I2C bus

**Implementation:**
1. Set both XSHUT pins LOW (disable both sensors)
2. Enable sensor #1 (XSHUT HIGH), configure to 0x30
3. Enable sensor #2 (XSHUT HIGH), keep at 0x29
4. Both sensors now addressable independently

**Alternative:** I2C multiplexer (TCA9548A) - more complex, overkill for 2 sensors

## Design Patterns in Use

### 1. Sensor Reading Pattern
```cpp
// Continuous polling with error handling
uint16_t readSensor(Adafruit_VL53L0X &sensor) {
    VL53L0X_RangingMeasurementData_t measure;
    sensor.rangingTest(&measure, false);

    if (measure.RangeStatus != 4) {  // 4 = out of range
        return measure.RangeMilliMeter;
    }
    return lastValidReading;  // Use last good value on error
}
```

### 2. Signal Smoothing Pattern
```cpp
// Exponential Weighted Moving Average (EWMA) - Current Implementation
// Provides better responsiveness than simple moving average
// Alpha controls balance: 0.0 = very smooth/slow, 1.0 = no smoothing/instant
const float SMOOTHING_ALPHA = 0.3f;
float smoothedPitchDistance = 0.0f;
float smoothedVolumeDistance = 0.0f;
bool firstReading = true;

int applyExponentialSmoothing(float& smoothedValue, int newReading, bool isFirstReading) {
    if (isFirstReading) {
        // On first reading, initialize the smoothed value
        smoothedValue = (float)newReading;
    } else {
        // Apply EWMA formula: smoothed = alpha * new + (1 - alpha) * previous
        smoothedValue = (SMOOTHING_ALPHA * newReading) + ((1.0f - SMOOTHING_ALPHA) * smoothedValue);
    }
    return (int)smoothedValue;
}

// Legacy: Simple moving average (replaced October 27, 2025)
// Kept for reference - had ~100ms latency vs ~35ms with EWMA
/*
const int SAMPLES = 5;
int readings[SAMPLES];
int index = 0;

int smoothedRead(int newValue) {
    readings[index] = newValue;
    index = (index + 1) % SAMPLES;

    int sum = 0;
    for (int i = 0; i < SAMPLES; i++) {
        sum += readings[i];
    }
    return sum / SAMPLES;
}
*/
```

### 2a. Multi-Oscillator Mixing Pattern (October 27, 2025)
```cpp
// Per-Oscillator Volume Control Pattern
// Each oscillator has independent volume (0.0-1.0)
// Volume applied before mixing in Oscillator::getNextSample()

class Oscillator {
private:
    float volume;  // 0.0 = silent, 1.0 = full volume

public:
    void setVolume(float vol) {
        volume = constrain(vol, 0.0f, 1.0f);
    }

    int16_t getNextSample(float sampleRate) {
        // ... generate waveform sample ...

        // Apply volume control before returning
        return (int16_t)(sample * volume);
    }
};

// Multi-Oscillator Mixing with Automatic Clipping Prevention
// Sum all active oscillators, then average by count
// Math guarantees no clipping: max = (32767 + 19660 + 13107) / 3 = 21,845

void generateAudioBuffer() {
    for (int i = 0; i < BUFFER_SIZE; i++) {
        int activeCount = 0;
        int32_t mixedSample = 0;  // Use int32_t to prevent overflow

        // Add samples from all active oscillators (already volume-scaled)
        if (oscillator.isActive()) {
            mixedSample += oscillator.getNextSample((float)SAMPLE_RATE);
            activeCount++;
        }
        if (oscillator2.isActive()) {
            mixedSample += oscillator2.getNextSample((float)SAMPLE_RATE);
            activeCount++;
        }
        if (oscillator3.isActive()) {
            mixedSample += oscillator3.getNextSample((float)SAMPLE_RATE);
            activeCount++;
        }

        // Average to prevent clipping and maintain consistent volume
        int16_t sample = (activeCount > 0) ? (mixedSample / activeCount) : 0;

        // Apply global amplitude/gain if needed
        // ...
    }
}

// Example Configuration:
// oscillator.setWaveform(Oscillator::SINE);
// oscillator.setVolume(1.0);     // 100% - Full volume
//
// oscillator2.setWaveform(Oscillator::SQUARE);
// oscillator2.setOctaveShift(-1); // One octave down (sub-bass)
// oscillator2.setVolume(0.6);     // 60% - Quieter
//
// oscillator3.setWaveform(Oscillator::OFF);
// oscillator3.setVolume(0.4);     // 40% - Quietest (when enabled)
```

### 3. Range Mapping Pattern
```cpp
// Floating-point map for smooth frequency transitions (October 27, 2025)
// Eliminates quantization from integer map() function
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Map sensor distance to frequency with bounds checking
int mapDistance(int distance, int minDist, int maxDist,
                int minFreq, int maxFreq) {
    // Use float mapping for sub-Hz precision
    float frequencyFloat = mapFloat((float)distance,
                                     (float)minDist,
                                     (float)maxDist,
                                     (float)maxFreq,  // Note: inverted for theremin
                                     (float)minFreq);

    // Constrain and convert to integer
    int freq = constrain((int)frequencyFloat, minFreq, maxFreq);

    return freq;
}
```

## Critical Implementation Paths

### Path 1: Initialization Sequence
```
setup() {
    1. Serial.begin(115200)
    2. Wire.begin(GPIO21, GPIO22)  // I2C init
    3. pinMode(XSHUT1_PIN, OUTPUT)
    4. pinMode(XSHUT2_PIN, OUTPUT)

    5. digitalWrite(XSHUT1_PIN, LOW)   // Disable both
    6. digitalWrite(XSHUT2_PIN, LOW)
    7. delay(10)

    8. digitalWrite(XSHUT1_PIN, HIGH)  // Enable sensor 1
    9. delay(10)
    10. sensor1.begin(0x30)            // Set new address
    11. sensor1.startContinuous()

    12. digitalWrite(XSHUT2_PIN, HIGH) // Enable sensor 2
    13. delay(10)
    14. sensor2.begin(0x29)            // Default address
    15. sensor2.startContinuous()

    16. ledcSetup(PWM_CHANNEL, BASE_FREQ, RESOLUTION)
    17. ledcAttachPin(BUZZER_PIN, PWM_CHANNEL)
}
```

### Path 2: Main Loop Execution
```
loop() {
    1. Read sensor1 @ 0x30 → pitchDistance
    2. Smooth pitchDistance → smoothedPitch
    3. Map smoothedPitch → frequency (100-2000 Hz)

    4. Read sensor2 @ 0x29 → volumeDistance
    5. Smooth volumeDistance → smoothedVolume
    6. Map smoothedVolume → dutyCycle (0-255)

    7. ledcWriteTone(PWM_CHANNEL, frequency)
    8. ledcWrite(PWM_CHANNEL, dutyCycle)

    9. Optional: Serial debug output
    10. delay(10) // ~100Hz update rate
}
```

### Path 3: Error Recovery
```
I2C Error Detected:
    1. Log error to Serial
    2. Use last valid reading
    3. Continue operation
    4. If persistent (>10 errors): attempt sensor reset

Sensor Out of Range:
    1. Check RangeStatus == 4
    2. Use last valid value OR
    3. Set to safe default (silence or min/max)

Power/Init Failure:
    1. Serial error message
    2. Flash LED if available
    3. Halt or retry initialization
```

## Component Relationships

### I2C Bus Hierarchy
```
ESP32 I2C Controller (Wire)
├── VL53L0X #1 (Pitch) @ 0x30
│   ├── Controlled via XSHUT on GPIO16
│   └── Continuous ranging mode
└── VL53L0X #2 (Volume) @ 0x29
    ├── Controlled via XSHUT on GPIO17
    └── Continuous ranging mode
```

### Data Flow
```
Sensor Distance (mm)
    ↓
Smoothing Filter
    ↓
Range Validation
    ↓
Map to Parameter Space
    ↓
Frequency (Hz) or Duty Cycle (0-255)
    ↓
PWM Output
    ↓
Audio/Buzzer
```

## Timing Considerations

### Target Latency Budget
- Sensor read: ~20ms (VL53L0X measurement time)
- Smoothing: <1ms (simple averaging)
- Mapping: <1ms (arithmetic operations)
- PWM update: <1ms (register write)
- **Total: ~25-30ms** (acceptable for musical control)

### Loop Frequency
- Target: ~100Hz (10ms delay in loop)
- With sensor reads: actual ~50Hz (20ms per sensor pair)
- Trade-off: responsiveness vs. stability

## Effects Architecture (Phase 4 - October 31, 2025)

### Effects System Overview

```
Audio Signal Flow with Effects:

Oscillator 1 ─┐
Oscillator 2 ─┼─→ Mixer ─→ EffectsChain ─→ DAC Output
Oscillator 3 ─┘               │
                              ↓
                    ┌─────────┴──────────┐
                    │                    │
              DelayEffect          ChorusEffect
              (circular buffer)    (modulated delay + LFO)
                    │                    │
                    └─────────┬──────────┘
                              ↓
                         Mixed Output
```

### Effects Chain Architecture

```cpp
// Stack Allocation Pattern (RAII)
// Effects are direct members of EffectsChain, not pointers
// Benefit: Automatic cleanup, no heap fragmentation

class EffectsChain {
private:
    uint32_t sampleRate;

    // Direct member initialization (stack allocation)
    DelayEffect delay;    // ~13KB for 300ms @ 22050 Hz
    ChorusEffect chorus;  // ~3KB for 50ms max depth

public:
    EffectsChain(uint32_t sampleRate)
        : sampleRate(sampleRate),
          delay(300, sampleRate),      // Constructor initializer list
          chorus(sampleRate) {

        // Configure effects
        delay.setFeedback(0.5f);
        delay.setMix(0.3f);
        delay.setEnabled(false);

        chorus.setRate(1.0f);
        chorus.setDepth(5.0f);
        chorus.setMix(0.2f);
        chorus.setEnabled(false);
    }

    // Signal flow: input → delay → chorus → output
    int16_t process(int16_t input) {
        int16_t output = input;
        output = delay.process(output);
        output = chorus.process(output);
        return output;
    }
};
```

### 4. Delay Effect Pattern

```cpp
// Digital Delay with Circular Buffer
// Classic feedback delay implementation

class DelayEffect {
private:
    int16_t* delayBuffer;  // Circular buffer
    size_t bufferSize;     // In samples
    size_t writeIndex;     // Current write position

    float feedback;        // 0.0-0.95 (prevent runaway)
    float wetDryMix;       // 0.0-1.0
    bool enabled;

public:
    int16_t process(int16_t input) {
        // Bypass optimization
        if (!enabled) return input;

        // Read delayed sample (circular buffer)
        int16_t delayedSample = delayBuffer[writeIndex];

        // Write with feedback: new = input + (delayed * feedback)
        int32_t newSample = input + (delayedSample * feedback);
        newSample = constrain(newSample, -32768, 32767);
        delayBuffer[writeIndex] = (int16_t)newSample;

        // Advance write pointer (circular)
        writeIndex = (writeIndex + 1) % bufferSize;

        // Mix dry and wet signals
        int32_t output = (input * (1.0f - wetDryMix)) + (delayedSample * wetDryMix);
        return constrain(output, -32768, 32767);
    }
};

// Memory calculation:
// Buffer size = (delay_ms / 1000.0) * sample_rate
// Example: 300ms @ 22050 Hz = 6615 samples = 13230 bytes
```

### 5. Chorus Effect Pattern with Oscillator-Based LFO

```cpp
// Modulated Delay for Chorus/Vibrato Effects
// Key Innovation: Reuses Oscillator class as LFO!

class ChorusEffect {
private:
    int16_t* delayBuffer;
    size_t bufferSize;
    size_t writeIndex;

    Oscillator lfo;        // Reuse Oscillator as LFO!
    float lfoDepthMs;      // Modulation depth
    float wetDryMix;

public:
    ChorusEffect(uint32_t sampleRate)
        : sampleRate(sampleRate) {

        // Configure LFO (using Oscillator class)
        lfo.setWaveform(Oscillator::SINE);
        lfo.setFrequency(2.0f);      // 2 Hz LFO rate
        lfo.setVolume(1.0f);

        // Allocate buffer for max depth (50ms)
        bufferSize = (50.0f / 1000.0f) * sampleRate + 100;
        delayBuffer = new int16_t[bufferSize];
        memset(delayBuffer, 0, bufferSize * sizeof(int16_t));
    }

    int16_t process(int16_t input) {
        if (!enabled) return input;

        // Write input
        delayBuffer[writeIndex] = input;

        // Get LFO value (sine LUT, ~100x faster than sin()!)
        float lfoValue = lfo.getNextSampleNormalized(sampleRate);

        // Calculate modulated delay time
        float delayTimeMs = lfoDepthMs + (lfoValue * lfoDepthMs);
        float delayInSamples = (delayTimeMs / 1000.0f) * sampleRate;

        // Read with linear interpolation
        int16_t delayedSample = readDelayBuffer(delayInSamples);

        // Mix
        int32_t output = (input * (1.0f - wetDryMix)) + (delayedSample * wetDryMix);

        writeIndex = (writeIndex + 1) % bufferSize;
        return constrain(output, -32768, 32767);
    }

    // Linear interpolation for fractional delay reads
    int16_t readDelayBuffer(float delayInSamples) {
        float readPos = (float)writeIndex - delayInSamples;

        // Wrap around
        while (readPos < 0) readPos += bufferSize;
        while (readPos >= bufferSize) readPos -= bufferSize;

        // Interpolate between two samples
        int index1 = (int)readPos;
        int index2 = (index1 + 1) % bufferSize;
        float fraction = readPos - index1;

        return (int16_t)(delayBuffer[index1] * (1.0f - fraction) +
                         delayBuffer[index2] * fraction);
    }
};
```

### 6. Oscillator-Based LFO Pattern

```cpp
// Design Pattern: Reuse Oscillator class for LFO
// Massive Performance Benefit: Sine LUT vs sin() calls

// BAD: Traditional LFO implementation (slow)
float getLFOValue_Slow(float phase) {
    return sin(phase * 2.0f * PI);  // ~100x slower!
}

// GOOD: Oscillator-based LFO (fast)
class ChorusEffect {
    Oscillator lfo;  // Reuse existing Oscillator class

    void configureLFO() {
        lfo.setWaveform(Oscillator::SINE);
        lfo.setFrequency(2.0f);  // Sub-audio rate (0.1-10 Hz)
        lfo.setVolume(1.0f);
    }

    float getLFOValue() {
        // Uses optimized sine LUT from Oscillator
        return lfo.getNextSampleNormalized(sampleRate);  // Fast!
    }
};

// Critical Bug Fix (October 31, 2025):
// Oscillator::setFrequency() was constraining to 20 Hz minimum (audio range)
// Updated to allow 0.1-20000 Hz range to support LFO use

// Before (bug):
void setFrequency(float freq) {
    frequency = constrain(freq, 20.0f, 20000.0f);  // Too restrictive!
}

// After (fixed):
void setFrequency(float freq) {
    frequency = constrain(freq, 0.1f, 20000.0f);  // Allows LFO range
}
```

### 7. Stack vs Heap Allocation Pattern

```cpp
// Pattern: RAII with Stack Allocation for Effects

// BAD: Heap allocation (fragmentation risk)
class EffectsChain_Bad {
    DelayEffect* delay;    // Pointers = heap allocation
    ChorusEffect* chorus;

    EffectsChain_Bad() {
        delay = new DelayEffect(300, 22050);    // Heap allocation
        chorus = new ChorusEffect(22050);       // Heap allocation
    }

    ~EffectsChain_Bad() {
        delete delay;    // Manual cleanup required
        delete chorus;   // Easy to forget or get wrong
    }
};

// GOOD: Stack allocation (RAII pattern)
class EffectsChain_Good {
    DelayEffect delay;     // Direct members = stack allocation
    ChorusEffect chorus;

    EffectsChain_Good(uint32_t sampleRate)
        : delay(300, sampleRate),    // Constructor initializer list
          chorus(sampleRate) {
        // Effects constructed on stack
        // Automatic cleanup when EffectsChain destroyed
    }

    // No destructor needed - automatic cleanup!
    // RAII: Resource Acquisition Is Initialization
};

// Benefits of Stack Allocation:
// ✓ No heap fragmentation (critical for embedded systems)
// ✓ Automatic cleanup (RAII pattern)
// ✓ Slightly faster allocation/deallocation
// ✓ Deterministic memory usage
// ✓ Simpler code (no manual new/delete)

// Trade-offs:
// ✗ Fixed at compile time (can't dynamically add/remove effects)
// ✗ Uses more stack space (but we have plenty)
```

### 8. Bypass Optimization Pattern

```cpp
// Pattern: Early return for disabled effects
// Minimizes CPU usage when effect is bypassed

class DelayEffect {
    bool enabled;

    int16_t process(int16_t input) {
        // Check enabled flag FIRST
        if (!enabled) {
            return input;  // Immediate return, no processing!
        }

        // Only execute expensive operations if enabled
        int16_t delayedSample = delayBuffer[writeIndex];
        // ... rest of delay processing ...

        return output;
    }
};

// Performance Impact:
// Enabled:  ~3-5% CPU (full delay processing)
// Disabled: ~0.1% CPU (single if-check + return)

// This is why our system uses only 9% CPU with 3 oscillators + 2 effects:
// - 3 oscillators: ~6%
// - Delay (enabled): ~1.5%
// - Chorus (enabled): ~1.5%
// - Total: ~9% (91% headroom!)
```

### Effects Integration in AudioEngine

```cpp
// Modified Audio Pipeline (October 31, 2025)

void AudioEngine::generateAudioBuffer() {
    for (size_t i = 0; i < BUFFER_SIZE; i++) {
        // Step 1: Mix oscillators
        int16_t sample = mixOscillators();

        // Step 2: Apply effects chain (NEW!)
        sample = effectsChain->process(sample);

        // Step 3: Apply amplitude smoothing
        smoothedAmplitude += (currentAmplitude - smoothedAmplitude) * SMOOTHING_FACTOR;
        sample = (int16_t)(sample * (smoothedAmplitude / 100.0f));

        // Step 4: Convert to DAC format (unsigned 8-bit)
        uint8_t dacValue = (sample >> 8) + 128;
        buffer[i] = dacValue;
    }
}

// Critical: Effects MUST be processed after mixing, before DAC conversion
// Order matters: Mix → Effects → Amplitude → DAC Format
```

### Performance Characteristics

**CPU Usage Breakdown (3 osc + delay + chorus):**
```
Component                   CPU %    Notes
─────────────────────────────────────────────────────────
Oscillator 1 (SINE)        ~2%      Phase accumulator + sine LUT
Oscillator 2 (SQUARE)      ~2%      Phase accumulator + comparison
Oscillator 3 (OFF)         ~0.1%    Early return optimization
Mixing (averaging)         ~1%      Integer math
DelayEffect (enabled)      ~1.5%    Circular buffer operations
ChorusEffect (enabled)     ~1.5%    Interpolation + LFO lookup
Amplitude smoothing        ~0.5%    EWMA calculation
DAC format conversion      ~0.5%    Bit shift + offset
─────────────────────────────────────────────────────────
Total                      ~9%      (91% headroom available!)
```

**Memory Usage:**
```
Component                Size        Location
──────────────────────────────────────────────────
DelayEffect buffer       ~13 KB      Stack (in EffectsChain)
ChorusEffect buffer      ~3 KB       Stack (in EffectsChain)
EffectsChain object      ~16 KB      Stack (in AudioEngine)
Oscillator sine LUT      512 bytes   PROGMEM (Flash)
──────────────────────────────────────────────────
Total RAM impact         ~16 KB      (from 314 KB free)
```

## Configuration Constants

```cpp
// Hardware pins
#define I2C_SDA 21
#define I2C_SCL 22
#define XSHUT1_PIN 16
#define XSHUT2_PIN 17
#define BUZZER_PIN 25

// I2C addresses
#define SENSOR1_ADDR 0x30  // Pitch
#define SENSOR2_ADDR 0x29  // Volume

// Operating ranges
#define PITCH_MIN_DIST 50    // mm
#define PITCH_MAX_DIST 400   // mm
#define VOLUME_MIN_DIST 50   // mm
#define VOLUME_MAX_DIST 300  // mm

// Audio ranges
#define MIN_FREQUENCY 100    // Hz
#define MAX_FREQUENCY 2000   // Hz
#define PWM_CHANNEL 0
#define PWM_RESOLUTION 8     // bits

// Effects parameters (October 31, 2025)
#define DELAY_TIME_DEFAULT 300      // ms
#define DELAY_FEEDBACK_DEFAULT 0.5f // 50%
#define DELAY_MIX_DEFAULT 0.3f      // 30% wet
#define CHORUS_RATE_DEFAULT 1.0f    // Hz (LFO frequency)
#define CHORUS_DEPTH_DEFAULT 5.0f   // ms (modulation depth)
#define CHORUS_MIX_DEFAULT 0.2f     // 20% wet
