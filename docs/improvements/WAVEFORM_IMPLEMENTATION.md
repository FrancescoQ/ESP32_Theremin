# Waveform Implementation - Multiple Oscillator Shapes

**Implementation Date:** October 27, 2025
**Status:** ✅ Complete - Sine, Triangle, and Sawtooth waveforms added

## Overview

Expanded the Oscillator class to support multiple waveform types beyond the initial square wave. The theremin now supports four distinct sound characters through square, sine, triangle, and sawtooth waveforms.

## Implemented Waveforms

### 1. Square Wave (Original)
- **Sound Character:** Hollow, clarinet-like, buzzy
- **Harmonic Content:** Only odd harmonics (1st, 3rd, 5th, 7th...)
- **Implementation:** Simple phase comparison (phase < 0.5 ? max : min)
- **CPU Cost:** Minimal (one conditional)
- **Use Case:** Classic 8-bit game sounds, aggressive tones

### 2. Sine Wave (New)
- **Sound Character:** Pure, smooth, classical theremin sound
- **Harmonic Content:** Only fundamental frequency (no harmonics)
- **Implementation:** 256-entry lookup table (LUT) in PROGMEM
- **Memory Cost:** 512 bytes Flash (0 bytes RAM)
- **CPU Cost:** Very low (array lookup + PROGMEM read)
- **Use Case:** Traditional theremin, smooth melodic lines, testing pure tones

**Technical Details:**
- LUT stored in Flash using `PROGMEM` directive
- Phase (0.0-1.0) maps directly to table index (0-255)
- Values pre-calculated for full cycle (-32768 to 32767)
- `pgm_read_word()` used for Flash memory access

### 3. Triangle Wave (New)
- **Sound Character:** Mellow, flute-like, woody
- **Harmonic Content:** Only odd harmonics (decreasing rapidly)
- **Implementation:** Mathematical generation (piecewise linear)
- **Memory Cost:** 0 bytes (calculated on-the-fly)
- **CPU Cost:** Low (2-3 floating-point operations + one conditional)
- **Use Case:** Soft leads, woodwind-like tones, vintage synth sounds

**Technical Details:**
```cpp
// Rising edge (phase 0.0-0.5): -32768 → 32767
sample = (phase * 2.0f * 65535.0f) - 32768.0f;

// Falling edge (phase 0.5-1.0): 32767 → -32768
sample = ((1.0f - phase) * 2.0f * 65535.0f) - 32768.0f;
```

### 4. Sawtooth Wave (New)
- **Sound Character:** Bright, brassy, aggressive, buzzy
- **Harmonic Content:** ALL harmonics (both even and odd)
- **Implementation:** Mathematical generation (direct linear mapping)
- **Memory Cost:** 0 bytes (calculated on-the-fly)
- **CPU Cost:** Minimal (~15 cycles - one float multiply + cast)
- **Use Case:** Synthesizer brass, aggressive leads, rich bass tones

**Technical Details:**
```cpp
// Direct linear mapping: phase (0.0-1.0) → amplitude (-32768 to 32767)
return (int16_t)((phase * 65535.0f) - 32768.0f);
```

**Why Sawtooth is the Simplest:**
Despite containing the richest harmonic content, sawtooth is the simplest to generate - just a single line of code! It's a direct linear ramp from minimum to maximum amplitude.

## Usage

### Compile-Time Waveform Selection

To test different waveforms, modify `src/AudioEngine.cpp` line 14:

```cpp
// Square wave (buzzy, hollow)
oscillator.setWaveform(Oscillator::SQUARE);

// Sine wave (smooth, pure)
oscillator.setWaveform(Oscillator::SINE);

// Triangle wave (mellow, flute-like)
oscillator.setWaveform(Oscillator::TRIANGLE);

// Sawtooth wave (bright, brassy)
oscillator.setWaveform(Oscillator::SAW);
```

Recompile and upload to test each waveform.

### Future: Runtime Waveform Switching

For dynamic waveform selection (e.g., via button press), add to your main loop:

```cpp
// Example: Cycle through waveforms with a button
static int currentWaveform = Oscillator::SQUARE;
if (buttonPressed) {
  currentWaveform = (currentWaveform + 1) % 5; // Cycle through all waveforms
  if (currentWaveform == Oscillator::OFF) currentWaveform++; // Skip OFF
  audioEngine.getOscillator().setWaveform((Oscillator::Waveform)currentWaveform);
}
```

## Performance Impact

### Memory Usage
- **RAM:** 0 bytes additional (LUT stored in Flash)
- **Flash:** +512 bytes (sine LUT only)
- **Impact:** Negligible (< 0.5% of available Flash)

### CPU Usage
All waveforms are highly efficient:
- **Square:** ~5 CPU cycles (one comparison)
- **Sawtooth:** ~15 CPU cycles (one float multiply + cast)
- **Sine:** ~20 CPU cycles (index calculation + PROGMEM read)
- **Triangle:** ~30 CPU cycles (float operations + conditional)

At 22,050 Hz sample rate, total CPU overhead is < 1% for any waveform.

## Sound Comparison

| Waveform | Brightness | Harmonics | Character | Theremin Use |
|----------|-----------|-----------|-----------|--------------||
| **Sine** | Darkest | None | Pure, smooth | Classical theremin |
| **Triangle** | Mild | Odd (weak) | Mellow, woody | Soft melodies |
| **Square** | Bright | Odd (strong) | Buzzy, hollow | Aggressive leads |
| **Sawtooth** | Brightest | All (even+odd) | Brassy, rich | Synthesizer brass, aggressive bass |


## Implementation Files

### Modified Files
- `include/Oscillator.h`
  - Added SINE, TRIANGLE, and SAW to Waveform enum
  - Added sine LUT declaration (`SINE_TABLE[256]`)
  - Added `generateSineWave()`, `generateTriangleWave()`, and `generateSawtoothWave()` declarations

- `src/Oscillator.cpp`
  - Added 256-entry sine lookup table in PROGMEM
  - Implemented `generateSineWave()` using LUT
  - Implemented `generateTriangleWave()` using piecewise linear math
  - Implemented `generateSawtoothWave()` using direct linear mapping
  - Updated switch statement in `getNextSample()` for all waveforms

### No Changes Required
- AudioEngine integration works automatically (uses oscillator polymorphically)
- No changes to Theremin or main.cpp needed
- Waveform selection happens at AudioEngine initialization

## Testing Recommendations

1. **Test each waveform individually:**
   - Verify audio output is smooth and continuous
   - Check frequency range (220-880 Hz) works correctly
   - Confirm volume control functions properly

2. **Listen for artifacts:**
   - **Sine:** Should be perfectly smooth, no buzzing
   - **Triangle:** Slightly hollow but not harsh
   - **Square:** Buzzy but not distorted
   - **Sawtooth:** Bright and brassy but clean

3. **Performance check:**
   - Monitor serial output for timing issues
   - Verify no audio dropouts or gaps
   - Check CPU usage remains low

4. **Frequency sweep test:**
   - Sweep through entire range slowly
   - Listen for discontinuities or clicks
   - Verify phase accumulator wrapping is smooth

## Known Limitations

1. **Compile-time selection only:** Currently requires recompile to change waveforms (runtime switching planned for Phase 3)

2. **No anti-aliasing:** High frequencies may show slight aliasing artifacts (acceptable for 220-880 Hz range)

3. **Fixed amplitude:** All waveforms generate full-scale samples (amplitude control applied separately in AudioEngine)

## Related Documentation

- `/docs/architecture/ARCHITECTURE.md` - Overall system design
- `/docs/improvements/CONTINUOUS_AUDIO_IMPLEMENTATION.md` - Audio task architecture
- `/include/Oscillator.h` - Oscillator API reference

## Next Steps (Phase 3 - Future)

1. **Runtime waveform switching:** Add button input for live waveform changes
2. **Waveform morphing:** Blend between waveforms for unique timbres
3. **Multiple oscillators:** Mix different waveforms simultaneously
4. **Pulse width modulation:** Variable duty cycle for square wave
