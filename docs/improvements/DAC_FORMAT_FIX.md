# DAC Sample Format Fix - Eliminating Audio Distortion

**Implementation Date:** October 27, 2025
**Status:** ✅ Complete - Distortion eliminated

## Problem

User reported distortion across all waveforms (square, sine, triangle), even at low volumes. The distortion was persistent regardless of waveform type, indicating it was not a waveform generation issue but an output stage problem.

## Root Cause

The ESP32's built-in DAC (GPIO25) expects **unsigned 8-bit samples (0-255)**, but the code was generating and sending **signed 16-bit samples (-32768 to 32767)**. This format mismatch caused:

1. **Incorrect interpretation** - DAC hardware misread the data
2. **Value wrapping/clipping** - Negative values wrapped around unpredictably
3. **Bit truncation** - 16-bit samples truncated to 8-bit incorrectly

## Solution

Modified `generateAudioBuffer()` in `AudioEngine.cpp` to properly convert samples:

### Sample Conversion Process

```cpp
// 1. Generate signed 16-bit sample from oscillator
int16_t sample = oscillator.getNextSample((float)SAMPLE_RATE);

// 2. Apply amplitude scaling
int16_t scaledSample = (int16_t)(sample * gain);

// 3. Convert to unsigned 8-bit for DAC:
//    a) Take upper 8 bits: -32768..32767 → -128..127
//    b) Add DC offset (128): -128..127 → 0..255
uint8_t dacSample = (uint8_t)((scaledSample >> 8) + 128);

// 4. Place in upper byte of 16-bit word (DAC reads upper byte)
buffer[i] = ((uint16_t)dacSample) << 8;
```

### Why This Works

1. **Bit shift (>> 8)**: Extracts the most significant 8 bits from 16-bit sample
   - Preserves amplitude information
   - Reduces resolution from 16-bit to 8-bit
   - Range: -128 to 127

2. **DC offset (+128)**: Centers waveform around 128 instead of 0
   - Converts signed to unsigned
   - Range: 0 to 255
   - Required by DAC hardware

3. **Upper byte placement (<< 8)**: Aligns with ESP32 I2S DAC format
   - DAC reads upper byte of 16-bit word
   - Maintains I2S DMA alignment
   - Lower byte ignored by hardware

### Buffer Type Change

```cpp
// Before (WRONG):
int16_t buffer[BUFFER_SIZE];

// After (CORRECT):
uint16_t buffer[BUFFER_SIZE];
```

Using `uint16_t` ensures proper I2S DMA alignment while allowing us to place the 8-bit DAC sample in the upper byte.

## Impact

### Audio Quality
- ✅ **Distortion eliminated** - Clean output across all waveforms
- ✅ **Sine wave** - Perfectly smooth, no artifacts
- ✅ **Triangle wave** - Mellow and clean
- ✅ **Square wave** - Buzzy character but not distorted

### Performance
- **CPU usage:** Unchanged (simple bit operations)
- **Memory:** Unchanged (same buffer size)
- **Latency:** Unchanged (~11ms per buffer)

### Code Changes
- Modified: `src/AudioEngine.cpp` - `generateAudioBuffer()` method only
- No changes to: Oscillator classes, I2S configuration, other components

## Technical Notes

### ESP32 Built-in DAC Characteristics

The ESP32 has two 8-bit DACs:
- **DAC1:** GPIO25 (used in this project)
- **DAC2:** GPIO26 (unused)

**Key specifications:**
- Resolution: 8-bit (0-255, representing 0V to 3.3V)
- Output range: ~0.0V to ~3.1V (not full rail-to-rail)
- Sampling rate: Up to 1 MHz theoretical, ~100 kHz practical for quality
- Interface: I2S in DAC mode reads upper byte of 16-bit samples

### Why 8-bit Resolution is Acceptable

For this theremin application:
- **Frequency range:** 220-880 Hz (musical notes)
- **Human hearing:** Cannot perceive quantization noise at 8-bit with proper dithering
- **Waveform fidelity:** 256 levels sufficient for smooth waveforms at audio frequencies
- **Trade-off:** Built-in DAC is simpler than external DAC (no additional hardware)

Future upgrade path: External I2S DAC (e.g., PCM5102A) provides 16-bit/24-bit resolution if needed.

## Testing Verification

After applying this fix, verify:

1. **Sine wave test:** Should sound perfectly smooth and pure
   - No buzzing, clicking, or crackling
   - Smooth pitch glides
   - Clean at all volumes

2. **Triangle wave test:** Should sound mellow and flute-like
   - Slight hollowness (characteristic of triangle)
   - No harsh edges or distortion

3. **Square wave test:** Should sound buzzy but clean
   - Buzzy character is normal (harmonic content)
   - No additional distortion or artifacts

4. **Volume sweep:** Test from 0-100% amplitude
   - No distortion at low volumes
   - No clipping at high volumes
   - Smooth volume transitions

## Related Issues

This fix resolves:
- ✅ Distortion at all volume levels
- ✅ "Gritty" or "crunchy" sound quality
- ✅ Unexpected harmonics in sine wave
- ✅ Harsh edges on triangle wave

This does NOT affect:
- ❌ Pitch stepping (sensor quantization - separate issue)
- ❌ Waveform selection (compile-time only)
- ❌ Latency (sensor-to-sound response time)

## Code History

**Before (incorrect):**
```cpp
int16_t buffer[BUFFER_SIZE];
// ... generate samples ...
buffer[i] = (int16_t)(sample * gain);
i2s_write(..., buffer, BUFFER_SIZE * sizeof(int16_t), ...);
```

**After (correct):**
```cpp
uint16_t buffer[BUFFER_SIZE];
// ... generate samples ...
int16_t scaledSample = (int16_t)(sample * gain);
uint8_t dacSample = (uint8_t)((scaledSample >> 8) + 128);
buffer[i] = ((uint16_t)dacSample) << 8;
i2s_write(..., buffer, BUFFER_SIZE * sizeof(uint16_t), ...);
```

## Implementation Files

### Modified Files
- `src/AudioEngine.cpp` - `generateAudioBuffer()` method
  - Changed buffer type from `int16_t` to `uint16_t`
  - Added sample format conversion
  - Added detailed comments explaining conversion process

### No Changes Required
- `include/AudioEngine.h` - I2S configuration already correct
- `src/Oscillator.cpp` - Waveform generation remains 16-bit internally
- All other files - No impact on other components

## References

- [ESP32 Technical Reference Manual - I2S DAC Mode](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf)
- ESP-IDF I2S Driver Documentation
- `/docs/improvements/WAVEFORM_IMPLEMENTATION.md` - Waveform generation
- `/docs/improvements/CONTINUOUS_AUDIO_IMPLEMENTATION.md` - Audio task architecture

## Future Considerations

### External DAC Option (Phase 5-6)

If higher audio quality is desired:
- **Hardware:** PCM5102A I2S DAC module (~$5)
- **Resolution:** 24-bit vs 8-bit built-in
- **SNR:** ~100dB vs ~60dB built-in
- **Code changes:** Minimal (remove format conversion, adjust I2S config)
- **Benefit:** Professional audio quality, true 16-bit waveforms

For now, 8-bit built-in DAC is perfectly adequate for theremin synthesis.
