# Pitch Quantization Implementation Plan

## Overview

Add musical pitch quantization to the theremin, allowing users to snap continuous pitch to discrete musical notes. This makes melodic playing significantly easier while maintaining the option for expressive continuous pitch.

**Difficulty:** ⭐ Easy (1-2 hours for basic implementation)
**Impact:** 🎵 High - Dramatically improves playability for melodic performance

---

## What is Pitch Quantization?

Converts continuous frequency output to the nearest musical note:

```
Without Quantization: 442 Hz, 444 Hz, 446 Hz, 448 Hz... (continuous)
With Quantization:    440 Hz (A4), 466 Hz (A#4), 494 Hz (B4)... (discrete notes)
```

Think of it as adding "invisible frets" to the theremin.

---

## Current Signal Flow

```
Sensor Reading → Distance Mapping → Frequency Calculation → Audio Output
     ↓                                        ↓
  50-400mm                               220-880 Hz (continuous)
```

## With Quantization

```
Sensor Reading → Distance Mapping → Frequency Calculation → Quantization → Audio Output
     ↓                                        ↓                    ↓
  50-400mm                               220-880 Hz        Nearest Note → 220, 233, 247...
```

---

## Basic Implementation (Phase 1)

### 1. Core Quantization Algorithm

**Location:** Add to `Theremin.h` and `Theremin.cpp`

**Algorithm:**
```cpp
/**
 * Quantize frequency to nearest chromatic note
 * Uses equal temperament tuning (12-TET)
 * @param freq Raw frequency in Hz
 * @return Quantized frequency (nearest semitone)
 */
float Theremin::quantizeToNote(float freq) {
  // A4 (440 Hz) is the reference pitch
  // Each semitone is 2^(1/12) ≈ 1.05946 ratio

  // Convert frequency to semitones from A4
  // Formula: semitones = 12 * log2(freq / 440)
  float semitones = 12.0f * log2f(freq / 440.0f);

  // Round to nearest integer (nearest semitone)
  int roundedSemitones = roundf(semitones);

  // Convert back to frequency
  // Formula: freq = 440 * 2^(semitones/12)
  return 440.0f * powf(2.0f, roundedSemitones / 12.0f);
}
```

**Math Explanation:**
- **Equal Temperament:** Each octave divided into 12 equal semitones
- **Frequency Ratio:** Each semitone = 2^(1/12) ≈ 1.059463
- **Logarithmic Mapping:** `log2(f1/f2)` gives octaves difference
- **Multiply by 12:** Converts octaves to semitones

### 2. Add State Variables

**Theremin.h:**
```cpp
private:
  bool quantizationEnabled;  // Master enable/disable flag

public:
  /**
   * Enable or disable pitch quantization
   * @param enabled True to quantize to chromatic scale, false for continuous pitch
   */
  void setQuantizationEnabled(bool enabled);

  /**
   * Check if quantization is enabled
   * @return True if quantization active
   */
  bool isQuantizationEnabled() const { return quantizationEnabled; }
```

**Theremin.cpp Constructor:**
```cpp
Theremin::Theremin(PerformanceMonitor* perfMon, DisplayManager* displayMgr)
    : sensors(), audio(perfMon), serialControls(this), gpioControls(this, displayMgr),
      display(displayMgr), debugEnabled(false), quantizationEnabled(false) {
  // ... existing code ...
}
```

### 3. Integrate into Update Loop

**Theremin.cpp - `update()` method:**
```cpp
// Current code (simplified):
if (sensors.isPitchEnabled()) {
  int pitchDistance = sensors.getPitchDistance();

  float frequencyFloat = mapFloat((float)pitchDistance,
                                   (float)sensors.getPitchMinDist(),
                                   (float)sensors.getPitchMaxDist(),
                                   (float)audio.getMaxFrequency(),
                                   (float)audio.getMinFrequency());

  // **NEW: Apply quantization if enabled**
  if (quantizationEnabled) {
    frequencyFloat = quantizeToNote(frequencyFloat);
  }

  int frequency = constrain((int)frequencyFloat, audio.getMinFrequency(), audio.getMaxFrequency());
  audio.setFrequency(frequency);
}
```

### 4. Add GPIO Control Mapping

**Option A: Use Unused Oscillator Waveform Switch**

Map to one of the waveform switches during modifier mode. Since all 3 waveform switches already control effects, you could:
- **Repurpose** one effect control temporarily
- **Add** a new modifier mode combination

**GPIOControls.cpp - Add new secondary control:**
```cpp
void GPIOControls::oscXWaveformSecondaryControl() {
  unsigned long now = millis();
  Oscillator::Waveform waveformValue = readWaveform(PIN_OSCX_WAVE_A, PIN_OSCX_WAVE_B, PIN_OSCX_WAVE_C);

  // Track last state for debouncing
  static Oscillator::Waveform lastQuantizationWaveform = Oscillator::Waveform::OFF;
  static unsigned long lastQuantizationChangeTime = 0;

  if (waveformValue != lastQuantizationWaveform) {
    if (now - lastQuantizationChangeTime > DEBOUNCE_MS) {
      lastQuantizationWaveform = waveformValue;
      lastQuantizationChangeTime = now;

      // Map waveform switch to quantization state
      bool enabled = (waveformValue != Oscillator::Waveform::OFF);
      theremin->setQuantizationEnabled(enabled);

      DEBUG_PRINT("[GPIO] Quantization: ");
      DEBUG_PRINTLN(enabled ? "ENABLED" : "DISABLED");
    }
  }
}
```

**Option B: Serial Command (Simpler for Testing)**

**SerialControls.cpp:**
```cpp
else if (cmd == "quantize") {
  if (args == "on") {
    theremin->setQuantizationEnabled(true);
    Serial.println("Quantization: ON (chromatic scale)");
  } else if (args == "off") {
    theremin->setQuantizationEnabled(false);
    Serial.println("Quantization: OFF (continuous pitch)");
  }
}
```

---

## Testing the Basic Implementation

### Test Cases

1. **Verify Note Accuracy:**
   - Set frequency to 440 Hz → Should stay 440 Hz (A4)
   - Set frequency to 442 Hz → Should snap to 440 Hz (A4)
   - Set frequency to 458 Hz → Should snap to 466 Hz (A#4)
   - Set frequency to 494 Hz → Should output 494 Hz (B4)

2. **Verify Scale Coverage:**
   - Test across full range (220-880 Hz)
   - Verify no skipped notes
   - Check octave boundaries are correct

3. **Performance Test:**
   - Monitor CPU usage with/without quantization
   - Verify no audio glitches or latency
   - Check smooth switching on/off

### Debug Output Example

```cpp
if (quantizationEnabled) {
  float rawFreq = frequencyFloat;
  frequencyFloat = quantizeToNote(frequencyFloat);

  DEBUG_PRINT("[QUANT] Raw: ");
  DEBUG_PRINT(rawFreq);
  DEBUG_PRINT(" Hz → Quantized: ");
  DEBUG_PRINT(frequencyFloat);
  DEBUG_PRINTLN(" Hz");
}
```

---

## Advanced Features (Phase 2)

### 1. Scale-Based Quantization

Instead of all 12 chromatic notes, quantize to specific musical scales.

**Add Enum:**
```cpp
enum QuantizationMode {
  QUANT_OFF,          // Continuous pitch (no quantization)
  QUANT_CHROMATIC,    // All 12 notes (default)
  QUANT_MAJOR,        // Major scale (7 notes: W-W-H-W-W-W-H)
  QUANT_MINOR,        // Natural minor scale (7 notes)
  QUANT_PENTATONIC,   // Pentatonic scale (5 notes)
  QUANT_BLUES,        // Blues scale (6 notes)
  QUANT_WHOLE_TONE    // Whole tone scale (6 notes)
};
```

**Scale Lookup Tables:**
```cpp
// Semitone offsets from root note for each scale
// 0 = root, 12 = octave
const int SCALE_MAJOR[]       = {0, 2, 4, 5, 7, 9, 11, 12};  // W-W-H-W-W-W-H
const int SCALE_MINOR[]       = {0, 2, 3, 5, 7, 8, 10, 12};  // W-H-W-W-H-W-W
const int SCALE_PENTATONIC[]  = {0, 2, 4, 7, 9, 12};         // W-W-m3-W-m3
const int SCALE_BLUES[]       = {0, 3, 5, 6, 7, 10, 12};     // m3-W-H-H-m3-W
const int SCALE_WHOLE_TONE[]  = {0, 2, 4, 6, 8, 10, 12};     // W-W-W-W-W-W
```

**Enhanced Quantization Function:**
```cpp
float Theremin::quantizeToScale(float freq, const int* scale, int scaleSize) {
  // Convert to semitones from A4
  float semitones = 12.0f * log2f(freq / 440.0f);

  // Determine which octave we're in
  int octave = floorf(semitones / 12.0f);
  float semitoneInOctave = semitones - (octave * 12.0f);

  // Find nearest note in scale
  int closestIdx = 0;
  float minDist = fabsf(semitoneInOctave - scale[0]);

  for (int i = 1; i < scaleSize; i++) {
    float dist = fabsf(semitoneInOctave - scale[i]);
    if (dist < minDist) {
      minDist = dist;
      closestIdx = i;
    }
  }

  // Calculate final semitone (scale note + octave offset)
  float quantizedSemitones = (octave * 12.0f) + scale[closestIdx];

  // Convert back to frequency
  return 440.0f * powf(2.0f, quantizedSemitones / 12.0f);
}
```

### 2. Key/Root Note Selection

Allow transposing the scale to any key (C, D, E, etc.).

```cpp
enum RootNote {
  ROOT_C = 0,
  ROOT_Cs_Db = 1,
  ROOT_D = 2,
  ROOT_Ds_Eb = 3,
  ROOT_E = 4,
  ROOT_F = 5,
  ROOT_Fs_Gb = 6,
  ROOT_G = 7,
  ROOT_Gs_Ab = 8,
  ROOT_A = 9,   // Default (440 Hz)
  ROOT_As_Bb = 10,
  ROOT_B = 11
};

private:
  QuantizationMode quantizationMode;
  RootNote rootNote;

// Adjust reference frequency based on root note
float getReferenceFreq() {
  // A4 = 440 Hz is index 9
  // Shift reference based on chosen root
  int semitonesFromA = rootNote - ROOT_A;
  return 440.0f * powf(2.0f, semitonesFromA / 12.0f);
}
```

### 3. Portamento/Glide Between Notes

Add smooth sliding between quantized notes instead of instant snapping.

```cpp
private:
  float targetQuantizedFreq;   // Destination frequency
  float currentQuantizedFreq;  // Current frequency (smoothly approaching target)
  float glideSpeed;            // 0.0-1.0 (0=instant, lower=slower glide)

void Theremin::update() {
  // ... sensor reading ...

  if (quantizationEnabled) {
    // Calculate target quantized frequency
    targetQuantizedFreq = quantizeToNote(rawFrequency);

    // Smooth transition using exponential glide
    currentQuantizedFreq += (targetQuantizedFreq - currentQuantizedFreq) * glideSpeed;

    audio.setFrequency(currentQuantizedFreq);
  } else {
    audio.setFrequency(rawFrequency);
  }
}
```

**Glide Speed Presets:**
- **0.0:** Infinite glide (never reaches target)
- **0.1:** Slow glide (~2 seconds)
- **0.3:** Medium glide (~0.5 seconds) - good for expression
- **0.7:** Fast glide (~0.1 seconds) - subtle smoothing
- **1.0:** Instant snap (no glide)

### 4. Visual Feedback on Display

Show current note name on the OLED display.

```cpp
// Note names lookup table
const char* NOTE_NAMES[] = {
  "C", "C#", "D", "D#", "E", "F",
  "F#", "G", "G#", "A", "A#", "B"
};

// Get note name from frequency
String getNoteName(float freq) {
  float semitones = 12.0f * log2f(freq / 440.0f);
  int semitoneIndex = roundf(semitones);
  int octave = 4 + (semitoneIndex / 12);
  int noteIndex = ((semitoneIndex % 12) + 9) % 12;  // Offset from A

  String noteName = NOTE_NAMES[noteIndex];
  noteName += octave;
  return noteName;
}

// Display on OLED
void drawQuantizationPage(Adafruit_SSD1306& oled) {
  oled.setCursor(0, 0);
  oled.print("Note: ");
  oled.println(getNoteName(currentFrequency));

  oled.print("Mode: ");
  oled.println(quantizationMode == QUANT_CHROMATIC ? "Chromatic" : "Major");

  oled.print("Key: ");
  oled.println(rootNote);
}
```

---

## Control Integration Options

### Option A: Dedicated Quantization Controls

**Modifier Mode Mapping:**
- **OSC1 Waveform:** Quantization Mode (OFF/CHROMATIC/MAJOR/MINOR/etc.)
- **OSC1 Octave:** Root Note Selection (cycle through C, D, E, F, G, A, B)
- **OSC2 Octave:** Glide Speed (DOWN=instant, CENTER=medium, UP=slow)

### Option B: Serial Commands (Best for Development)

```
quantize on          - Enable chromatic quantization
quantize off         - Disable quantization
quantize mode 0      - Set mode (0=off, 1=chromatic, 2=major, etc.)
quantize key C       - Set root note to C
quantize key D       - Set root note to D
quantize glide 0.5   - Set glide speed
```

### Option C: Preset System

```cpp
enum QuantizationPreset {
  PRESET_CONTINUOUS,      // No quantization
  PRESET_CHROMATIC,       // All notes, instant snap
  PRESET_CHROMATIC_GLIDE, // All notes, smooth glide
  PRESET_C_MAJOR,         // C major scale, instant
  PRESET_C_MAJOR_GLIDE,   // C major scale, smooth
  PRESET_A_MINOR,         // A minor scale
  // ... more presets ...
};
```

---

## Implementation Roadmap

### Phase 1: Basic Chromatic Quantization (1-2 hours)
- [x] Understand current frequency mapping in Theremin.cpp
- [ ] Implement `quantizeToNote()` function
- [ ] Add `quantizationEnabled` flag and getter/setter
- [ ] Integrate into `Theremin::update()`
- [ ] Add serial command for testing
- [ ] Test across full frequency range
- [ ] Verify CPU performance impact
- [ ] Document in code comments

### Phase 2: Scale-Based Quantization (2-3 hours)
- [ ] Create scale lookup tables
- [ ] Implement `quantizeToScale()` function
- [ ] Add QuantizationMode enum
- [ ] Add scale selection methods
- [ ] Add GPIO control mapping (or serial commands)
- [ ] Test all scales
- [ ] Document scale patterns

### Phase 3: Key Selection (1 hour)
- [ ] Add RootNote enum
- [ ] Implement root note transposition
- [ ] Add control interface
- [ ] Test key transposition accuracy
- [ ] Document musical theory

### Phase 4: Portamento/Glide (1-2 hours)
- [ ] Add glide state variables
- [ ] Implement exponential smoothing
- [ ] Add glide speed parameter
- [ ] Test musical expressiveness
- [ ] Tune glide speed presets

### Phase 5: Display Integration (1 hour)
- [ ] Create note name lookup function
- [ ] Design display page layout
- [ ] Add to display page rotation
- [ ] Test readability

**Total Estimated Time:**
- **Basic (Phase 1):** 1-2 hours
- **Full Implementation (All Phases):** 8-10 hours

---

## Performance Considerations

### CPU Usage Analysis

**Operations per sensor update (~50 Hz):**
- `log2f()`: ~20 CPU cycles (fast on ESP32)
- `powf()`: ~30 CPU cycles
- `roundf()`: ~5 CPU cycles
- **Total:** ~55 CPU cycles ≈ **0.3 microseconds @ 240 MHz**

**Comparison:**
- Sensor reading: ~20ms (I2C communication)
- Audio buffer generation: ~100-500 microseconds
- **Quantization overhead: Negligible (<0.001%)**

### Memory Usage

```
State variables:
  - quantizationEnabled (bool): 1 byte
  - quantizationMode (enum): 4 bytes
  - rootNote (enum): 4 bytes
  - glideSpeed (float): 4 bytes
  - targetFreq (float): 4 bytes
  - currentFreq (float): 4 bytes

Scale lookup tables:
  - 6 scales × 8 notes × 4 bytes = 192 bytes

Total: ~225 bytes (0.007% of ESP32 RAM)
```

**Verdict:** Zero performance impact, minimal memory usage.

---

## Musical Benefits

### Beginner-Friendly
- **Easier to play in tune** - notes snap to correct pitch
- **Reduced frustration** - no micro-adjustments needed
- **Faster learning curve** - focus on hand motion, not precision

### Advanced Expression
- **Clean melodic lines** - perfect for lead melodies
- **Chord-friendly** - can hold steady pitches for harmonies
- **Scale exploration** - experiment with different musical modes
- **Glide for emotion** - add expressiveness with portamento

### Performance Modes
- **Continuous (OFF):** Traditional theremin, full expressiveness
- **Chromatic:** Jazz, complex melodies, any key
- **Major/Minor:** Folk, pop, classical pieces
- **Pentatonic:** Blues, rock, improvisational jamming

---

## Future Enhancements

### Harmony/Chord Mode
Quantize to intervals (3rds, 5ths, octaves) for automatic harmonies.

### Microtonal Scales
Support non-Western tuning systems (31-TET, 19-TET, etc.).

### MIDI Output
Send quantized note data via MIDI for recording/sequencing.

### Arpeggiator Integration
Automatically cycle through scale notes.

### Vibrato Depth Control
Scale quantization boundaries based on vibrato depth.

---

## References

### Equal Temperament Math
- **Frequency Ratio:** f₂/f₁ = 2^(n/12) where n = semitones
- **A4 Reference:** 440 Hz (ISO 16 standard)
- **Octave:** Frequency doubles every 12 semitones

### Musical Scales
- **Major:** W-W-H-W-W-W-H (Ionian mode)
- **Minor:** W-H-W-W-H-W-W (Aeolian mode)
- **Pentatonic:** W-W-m3-W-m3 (5-note scale)
- **Blues:** m3-W-H-H-m3-W (blues scale)

Where: W = whole step (2 semitones), H = half step (1 semitone), m3 = minor 3rd (3 semitones)

---

## Code Example: Complete Basic Implementation

### Theremin.h
```cpp
class Theremin {
public:
  // ... existing methods ...

  /**
   * Enable or disable pitch quantization
   */
  void setQuantizationEnabled(bool enabled);

  /**
   * Check if quantization is enabled
   */
  bool isQuantizationEnabled() const { return quantizationEnabled; }

private:
  bool quantizationEnabled;  // Quantization master switch

  /**
   * Quantize frequency to nearest chromatic note
   * @param freq Raw frequency in Hz
   * @return Quantized frequency (nearest semitone)
   */
  float quantizeToNote(float freq);
};
```

### Theremin.cpp
```cpp
// Constructor
Theremin::Theremin(PerformanceMonitor* perfMon, DisplayManager* displayMgr)
    : sensors(), audio(perfMon), serialControls(this),
      gpioControls(this, displayMgr), display(displayMgr),
      debugEnabled(false), quantizationEnabled(false) {
  // ... existing initialization ...
}

// Setter
void Theremin::setQuantizationEnabled(bool enabled) {
  quantizationEnabled = enabled;
  DEBUG_PRINT("[THEREMIN] Quantization: ");
  DEBUG_PRINTLN(enabled ? "ENABLED" : "DISABLED");
}

// Quantization algorithm
float Theremin::quantizeToNote(float freq) {
  // A4 (440 Hz) is the reference pitch
  // Convert frequency to semitones from A4
  float semitones = 12.0f * log2f(freq / 440.0f);

  // Round to nearest integer (nearest semitone)
  int roundedSemitones = roundf(semitones);

  // Convert back to frequency
  return 440.0f * powf(2.0f, roundedSemitones / 12.0f);
}

// Update loop integration
void Theremin::update() {
  // ... existing code ...

  // Only apply pitch sensor if enabled
  if (sensors.isPitchEnabled()) {
    int pitchDistance = sensors.getPitchDistance();

    // Map pitch to frequency
    float frequencyFloat = mapFloat((float)pitchDistance,
                                     (float)sensors.getPitchMinDist(),
                                     (float)sensors.getPitchMaxDist(),
                                     (float)audio.getMaxFrequency(),
                                     (float)audio.getMinFrequency());

    // **NEW: Apply quantization if enabled**
    if (quantizationEnabled) {
      frequencyFloat = quantizeToNote(frequencyFloat);
    }

    // Constrain and set frequency
    int frequency = constrain((int)frequencyFloat,
                              audio.getMinFrequency(),
                              audio.getMaxFrequency());
    audio.setFrequency(frequency);
  }

  // ... rest of update code ...
}
```

---

## Conclusion

Pitch quantization is a **high-impact, low-effort** feature that will dramatically improve your theremin's playability. The basic chromatic implementation is straightforward and can be completed in an afternoon, while the advanced features (scales, keys, glide) offer rich musical possibilities for future expansion.

**Start with Phase 1** (basic chromatic quantization) and test it out. You'll immediately hear the difference in playability, and you can decide if the advanced features are worth pursuing based on your musical needs.

Good luck, and enjoy playing in tune! 🎵
