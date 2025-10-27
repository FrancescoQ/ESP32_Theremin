# Pitch Stepping Issue - Future Enhancement

## Status
⚠️ **Known Issue** - Low Priority (Polish/Refinement)

## Problem Description
When moving hand smoothly through the pitch sensor range, audible "steps" can be heard in the pitch transitions rather than perfectly smooth frequency changes.

## Symptoms
- Discrete pitch jumps instead of continuous glissando
- Most noticeable in slow, steady hand movements
- Not an audio generation issue - audio itself is perfectly smooth

## Root Cause Analysis

### Primary Cause: Sensor Quantization
**VL53L0X returns integer millimeters**
- Sensor reports: 50mm, 51mm, 52mm... (no fractional values)
- Frequency mapping: 50-400mm → 220-880Hz (660Hz range over 350mm)
- **Each 1mm step = ~1.9 Hz frequency jump**
- This is perceptible to human hearing, especially in sustained tones

### Secondary Causes
1. **Integer map() function**
   - Arduino's `map()` uses integer math
   - Compounds the quantization effect

2. **Insufficient smoothing**
   - Current: 5-sample moving average
   - Smooths noise but doesn't interpolate between readings
   - Acts as low-pass filter on values, not interpolation

## Impact Assessment
- **Audio Quality:** Vastly improved overall (continuous audio generation working perfectly)
- **User Experience:** Minor annoyance, noticeable but not breaking
- **Priority:** Low - this is a polish/refinement task
- **Comparison:** Much better than previous steppy audio issue

## Proposed Solutions

### Solution 1: Increase Smoothing Samples (Easiest)
**Approach:** Change `SAMPLES` constant in SensorManager from 5 to 10-20

**Pros:**
- Minimal code change (one constant)
- No new algorithms needed
- Already tested architecture

**Cons:**
- Increases latency (~100ms → 200-400ms)
- May feel sluggish/laggy to play
- Doesn't truly interpolate, just averages more

**Implementation:**
```cpp
// In include/SensorManager.h
static const int SAMPLES = 15;  // Increased from 5
```

### Solution 2: Exponential Smoothing (Better)
**Approach:** Replace moving average with exponential weighted moving average (EWMA)

**Pros:**
- Smoother transitions than simple averaging
- Minimal latency increase
- Responds faster to large changes, slower to small changes
- Common in audio/control applications

**Cons:**
- Requires algorithm change
- Need to tune smoothing factor

**Implementation:**
```cpp
// In SensorManager.h, add:
private:
  float smoothedPitchDistance;
  float smoothedVolumeDistance;
  static constexpr float SMOOTHING_ALPHA = 0.2f;  // 0.0-1.0, lower=smoother

// In SensorManager.cpp:
int SensorManager::getPitchDistance() {
  int raw = readPitchRaw();
  smoothedPitchDistance = (SMOOTHING_ALPHA * raw) + ((1.0f - SMOOTHING_ALPHA) * smoothedPitchDistance);
  return (int)smoothedPitchDistance;
}
```

### Solution 3: Linear Interpolation (Best Quality)
**Approach:** Interpolate between sensor readings over time

**Pros:**
- Truly smooth transitions
- No perceptible steps
- Professional quality
- Can adjust interpolation speed independently of sensor rate

**Cons:**
- Most complex implementation
- Requires time tracking
- Need to handle direction changes

**Implementation:**
```cpp
// In SensorManager:
private:
  float interpolatedPitchDistance;
  float targetPitchDistance;
  unsigned long lastPitchUpdate;
  static constexpr float INTERPOLATION_RATE = 5.0f;  // mm per millisecond

// In getPitchDistance():
int raw = readPitchRaw();
targetPitchDistance = raw;
unsigned long now = millis();
float dt = (now - lastPitchUpdate) / 1000.0f;  // seconds
lastPitchUpdate = now;

// Interpolate toward target
float delta = targetPitchDistance - interpolatedPitchDistance;
float maxChange = INTERPOLATION_RATE * dt;
if (abs(delta) < maxChange) {
  interpolatedPitchDistance = targetPitchDistance;
} else {
  interpolatedPitchDistance += (delta > 0 ? maxChange : -maxChange);
}
return (int)interpolatedPitchDistance;
```

### Solution 4: Floating-Point Frequency (Advanced)
**Approach:** Use float for frequency throughout the chain, not just in Oscillator

**Pros:**
- Most accurate
- No quantization from map() function
- Smooth at every stage

**Cons:**
- Requires changes to multiple classes
- float map() implementation needed
- More complex

**Implementation:**
```cpp
// In Theremin.cpp:
float frequency = mapFloat(pitchDistance,
    (float)SensorManager::PITCH_MIN_DIST,
    (float)SensorManager::PITCH_MAX_DIST,
    (float)AudioEngine::MIN_FREQUENCY,
    (float)AudioEngine::MAX_FREQUENCY);

audio.setFrequencyFloat(frequency);  // New method

// Helper function:
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
```

## Recommended Approach

**Start with Solution 2 (Exponential Smoothing):**
1. Simple to implement
2. Significant improvement over moving average
3. Minimal latency impact
4. Easy to tune

**If still not smooth enough, combine with Solution 4:**
- Exponential smoothing on sensor readings
- Floating-point frequency throughout chain
- Best balance of simplicity and quality

**Reserve Solution 3 (Interpolation) for v2.0:**
- When adding OLED display, can show interpolation settings
- User-adjustable interpolation speed
- Part of "professional polish" phase

## Testing Plan

When implementing fix:
1. **Baseline test:** Record current behavior (1mm = 1.9 Hz steps)
2. **Implementation test:** Measure improvement with each solution
3. **Latency test:** Ensure control doesn't feel sluggish
4. **Playability test:** Get user feedback on responsiveness

## Related Files
- `include/SensorManager.h` - Smoothing implementation
- `src/SensorManager.cpp` - Smoothing algorithm
- `src/Theremin.cpp` - Frequency mapping
- `include/AudioEngine.h` - Frequency parameter type

## References
- **Exponential Smoothing:** https://en.wikipedia.org/wiki/Exponential_smoothing
- **Audio Smoothing Techniques:** Common in synthesizer control voltage processing
- **Similar Projects:** Most DIY theremins face this issue; solutions vary

## Conclusion
This issue is minor compared to the major audio improvements achieved. The pitch stepping is noticeable but doesn't prevent the instrument from being playable. Solutions are well-understood and straightforward to implement when time allows.

**Priority:** Address after Phase 3 or 4, as part of refinement/polish cycle.
