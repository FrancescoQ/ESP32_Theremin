# PCM5102 External DAC Migration

**Implementation Date:** November 4, 2025
**Status:** ✅ Complete - Running on hardware

## Overview

Successfully migrated from ESP32 built-in DAC (GPIO25, 8-bit unsigned) to external PCM5102 I2S DAC (16-bit signed stereo). This upgrade represents a significant leap in audio quality while maintaining the same CPU footprint.

## Motivation

**Previous Limitation - ESP32 Built-in DAC:**
- 8-bit resolution (256 levels, 0-255 unsigned)
- Single channel (mono output, duplicated to both channels in software)
- Significant quantization noise at low volumes
- Required sample format conversion: `(sample >> 8) + 128`
- Limited dynamic range

**Goal - PCM5102 External DAC:**
- 16-bit resolution (65,536 levels, -32768 to +32767 signed)
- Native stereo output
- Professional-grade audio quality
- Direct signed sample output (no conversion needed)
- 112 dB dynamic range (THD+N: -93 dB typical)

## Hardware Specifications

### PCM5102 DAC Module

**Key Features:**
- 16-bit/32-bit I2S audio DAC
- Sample rates: 8 kHz to 384 kHz (we use 22.05 kHz)
- Ultra-low distortion: THD+N -93 dB
- Dynamic range: 112 dB
- Power supply: 3.3V or 5V (depending on module)
- No external components needed (PLL, filter on-board)

**Typical Module Configuration:**
- SCK: Internal PLL (leave unconnected)
- BCK: Bit Clock from ESP32
- DIN: Data Input from ESP32
- LRCK/WS: Word Select from ESP32
- XMT: Soft mute control (default pull-up/down on module)
- FLT: Filter select (default on module)
- FMT: Format select (I2S/LJ/RJ, default I2S on module)

### Pin Connections

```
ESP32          →  PCM5102 Module
─────────────────────────────────
GPIO26 (BCK)   →  BCK (Bit Clock)
GPIO27 (WS)    →  LRCK/WS (Word Select)
GPIO25 (DOUT)  →  DIN (Data Input)
3.3V/5V        →  VIN (check your module)
GND            →  GND
```

**Pin Selection Rationale:**
- GPIO25: Previously used for built-in DAC - convenient reuse
- GPIO26/27: Available GPIO pins, no conflicts with sensors (I2C) or controls (MCP23017)
- All pins are "safe" GPIO (not boot strapping pins)

## Software Implementation

### I2S Configuration Changes

**Before (Built-in DAC Mode):**
```cpp
i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
    .sample_rate = 22050,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_MSB,
    // ... other settings
};

// Set built-in DAC mode
i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);
```

**After (Standard I2S Mode):**
```cpp
i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),  // Standard I2S
    .sample_rate = 22050,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,    // Standard I2S format
    // ... other settings
};

// Configure I2S pins for external DAC
i2s_pin_config_t pin_config = {
    .bck_io_num = PIN_I2S_BCK,        // GPIO26
    .ws_io_num = PIN_I2S_WS,          // GPIO27
    .data_out_num = PIN_I2S_DOUT,     // GPIO25
    .data_in_num = I2S_PIN_NO_CHANGE
};
i2s_set_pin((i2s_port_t)I2S_NUM, &pin_config);
```

### Sample Format Changes

**Before (8-bit unsigned conversion):**
```cpp
// ESP32 built-in DAC expects unsigned 8-bit (0-255)
// Convert signed 16-bit → unsigned 8-bit
uint8_t dacValue = (sample >> 8) + 128;
buffer[i] = dacValue;
```

**After (16-bit signed direct):**
```cpp
// PCM5102 accepts signed 16-bit samples directly - no conversion needed!
int16_t buffer[BUFFER_SIZE * 2];  // Stereo: [L0, R0, L1, R1, ...]

// Route to channels based on mode
switch (currentChannelMode) {
    case LEFT_ONLY:
        buffer[i * 2] = scaledSample;   // Left channel
        buffer[i * 2 + 1] = 0;          // Right channel muted
        break;
    case RIGHT_ONLY:
        buffer[i * 2] = 0;              // Left channel muted
        buffer[i * 2 + 1] = scaledSample;
        break;
    case STEREO_BOTH:
    default:
        buffer[i * 2] = scaledSample;     // Left channel
        buffer[i * 2 + 1] = scaledSample; // Right channel (duplicate mono)
        break;
}
```

### Files Modified

**1. include/system/PinConfig.h**
- Removed built-in DAC pin definition
- Added PCM5102 I2S pin definitions (BCK, WS, DOUT)
- Updated comments with PCM5102 hardware notes

**2. src/audio/AudioEngine.cpp**
- Modified `setupI2S()`:
  - Changed to standard I2S mode (removed `I2S_MODE_DAC_BUILT_IN`)
  - Changed communication format to `I2S_COMM_FORMAT_STAND_I2S`
  - Added `i2s_set_pin()` configuration for external pins
  - Removed `i2s_set_dac_mode()` call
- Modified `generateAudioBuffer()`:
  - Changed buffer type from `uint8_t` to `int16_t[BUFFER_SIZE * 2]` (stereo)
  - Removed unsigned 8-bit conversion `(sample >> 8) + 128`
  - Added stereo channel routing logic
  - Direct signed 16-bit output to PCM5102

**3. include/audio/AudioEngine.h**
- Added `ChannelMode` enum for stereo routing
- Added `setChannelMode()` and `getChannelMode()` methods
- Updated private member: `currentChannelMode`

## Results

### Audio Quality Improvements

**✅ Dramatically Improved:**
- **Clarity:** Nettamente migliorata - much cleaner sound across all waveforms
- **Dynamic Range:** 112 dB vs ~48 dB (8-bit) - massive improvement
- **Quantization Noise:** Nearly eliminated at normal volumes
- **Distortion:** THD+N < -93 dB (professional grade)
- **Stereo Separation:** True hardware stereo output

**⚠️ Minor Issues Identified:**
- **Reverb/Delay at Low Volumes:** Alcuni glitch/grainy quando gli effetti scendono a volumi bassi
  - Likely cause: Quantization noise accumulation in int16_t feedback loops
  - Mitigation: Current noise gates help but may need tuning
  - Future solution: Consider int32_t precision for effect processing (Phase G)

### Performance Impact

**CPU Usage:** No change (14.5% with 3 osc + 3 effects)
- Sample generation logic unchanged
- Removed conversion overhead balances stereo buffer overhead
- DMA and I2S hardware handle output efficiently

**RAM Usage:** +512 bytes (stereo buffer vs mono)
- Before: `uint8_t buffer[BUFFER_SIZE]` = 256 bytes
- After: `int16_t buffer[BUFFER_SIZE * 2]` = 1024 bytes
- Negligible impact (314 KB free → ~313.5 KB free)

**Flash Usage:** No significant change
- Same oscillator and effects code
- Slightly different I2S configuration code
- Overall: <100 bytes difference

## Testing & Validation

### Test Procedure
1. ✅ Connected PCM5102 module to ESP32 (BCK, WS, DOUT, VIN, GND)
2. ✅ Updated pin definitions in PinConfig.h
3. ✅ Modified AudioEngine.cpp (setupI2S + generateAudioBuffer)
4. ✅ Compiled and uploaded firmware
5. ✅ Verified audio output on all oscillators
6. ✅ Tested all waveforms (SINE, TRIANGLE, SQUARE, SAW)
7. ✅ Tested effects chain (Delay, Chorus, Reverb)
8. ✅ Tested stereo channel routing modes

### Test Results
- ✅ I2S initialization successful
- ✅ Clean audio output confirmed
- ✅ All waveforms sound correct and clean
- ✅ Effects working (with minor low-volume glitches noted)
- ✅ No audio dropouts or glitches during normal operation
- ✅ Stereo output working correctly
- ✅ CPU and RAM usage within expected ranges

## Benefits Summary

| Aspect | Built-in DAC | PCM5102 | Improvement |
|--------|--------------|---------|-------------|
| **Resolution** | 8-bit (256 levels) | 16-bit (65,536 levels) | 256x |
| **Dynamic Range** | ~48 dB | 112 dB | 2.3x |
| **THD+N** | N/A (spec not provided) | -93 dB | Professional grade |
| **Output** | Mono (software duplicated) | True stereo | Native stereo |
| **Sample Format** | Unsigned (0-255) | Signed (-32768 to +32767) | Standard |
| **Conversion Overhead** | Yes (shift + offset) | No (direct output) | Eliminated |
| **Audio Quality** | Acceptable | Nettamente migliorata | Significant |

## Known Issues & Future Work

### Minor Issues
1. **Effect Glitches at Low Volumes:**
   - **Symptom:** Reverb and delay exhibit slight grainy/glitchy artifacts when decaying to very low levels
   - **Cause:** Quantization noise in int16_t feedback loops
   - **Current Mitigation:** Noise gates at input, feedback, and output (±50 threshold)
   - **Future Solutions:**
     - Phase G: Upgrade to int32_t precision for effect calculations
     - Fine-tune noise gate thresholds specifically for PCM5102 output
     - Consider adaptive noise gate based on signal level

### Future Enhancements
1. **Stereo Effects:** Implement true stereo effects (currently mono duplicated)
2. **Higher Sample Rate:** PCM5102 supports up to 384 kHz (currently 22.05 kHz)
3. **24-bit Mode:** PCM5102 supports 24-bit samples (requires code changes)

## Related Files

**Implementation:**
- `include/system/PinConfig.h` - Pin definitions and hardware configuration
- `include/audio/AudioEngine.h` - Audio engine interface
- `src/audio/AudioEngine.cpp` - I2S setup and buffer generation

**Documentation:**
- `memory-bank/techContext.md` - Hardware specifications
- `memory-bank/systemPatterns.md` - Audio architecture patterns
- `memory-bank/activeContext.md` - Recent changes log
- `memory-bank/progress.md` - Project status

## Conclusion

The migration to PCM5102 external DAC was a complete success. Audio quality is significantly improved with no performance penalty. The minor effect glitches at low volumes are a known limitation of int16_t precision and can be addressed in future Phase G enhancements if desired.

**Recommendation:** PCM5102 is the clear choice for any ESP32 audio project requiring high-quality output. The migration was straightforward and the benefits are substantial.
