# Reverb Precision Enhancement Plan

**Created:** November 5, 2025
**Status:** Planning Phase
**Goal:** Eliminate graininess in reverb decay tail at low volumes

## Problem Statement

After migrating to PCM5102 16-bit DAC, reverb (and to a lesser extent, delay) exhibits slight graininess/buzzing when decaying to very low volumes. The issue is most noticeable in the reverb tail as it approaches silence.

**Current Mitigation:**
- Three noise gates (input: ±100, filter: ±1.0, output: ±100)
- int32_t precision in comb filter feedback calculations (PRECISION_SHIFT = 8)
- Helps but doesn't fully eliminate the grain

**Root Cause:**
Quantization noise accumulation in feedback loops when using int16_t sample format, particularly at very low signal levels where quantization errors become proportionally significant.

---

## Current Implementation Analysis

### What's Already Using int32_t Precision ✅

**Comb Filter Feedback (in `processComb()`):**
```cpp
// Scale input UP to int32_t with 8-bit fractional precision (256x)
int32_t input32 = (int32_t)input << PRECISION_SHIFT;  // PRECISION_SHIFT = 8

// High-precision feedback calculation
int32_t feedback32 = (int32_t)(comb.filterStore * comb.feedback * (1 << PRECISION_SHIFT));
int32_t newValue32 = input32 + feedback32;

// Scale DOWN and store as int16_t
comb.buffer[bufferIndex] = (int16_t)(newValue32 >> PRECISION_SHIFT);
```

This provides **24-bit effective precision** (16-bit + 8-bit fractional) for comb filter feedback.

### What's NOT Using Enhanced Precision ⚠️

1. **Allpass Filters** - Simple int16_t/int32_t math without PRECISION_SHIFT
2. **Damping Filter** - Uses `float filterStore` which quantizes when affecting int16_t samples
3. **Noise Gates** - Fixed thresholds that might be too aggressive

---

## Enhancement Plan

### Phase A: Quick Wins (Test First) ⚡

**Goal:** Test if simple threshold adjustments solve the issue

#### Step A1: Reduce Noise Gate Thresholds

**Hypothesis:** Current thresholds (±100 for samples, ±1.0 for float) may be too coarse for 16-bit DAC, causing abrupt cutoffs that sound grainy.

**File:** `include/audio/effects/ReverbEffect.h`

**Current values:**
```cpp
static constexpr int16_t NOISE_GATE_THRESHOLD = 100;       // Input/output gate
static constexpr float FILTER_NOISE_GATE_THRESHOLD = 1.0f; // Comb filter gate
```

**Test values:**
```cpp
static constexpr int16_t NOISE_GATE_THRESHOLD = 30;        // 100 → 30 (67% reduction)
static constexpr float FILTER_NOISE_GATE_THRESHOLD = 0.3f; // 1.0 → 0.3 (proportional)
```

**Alternative values to try:**
- Very conservative: 20 / 0.2
- Aggressive: 50 / 0.5

**Testing procedure:**
1. Change constants in header
2. Compile and upload to hardware
3. Enable reverb: `reverb:on`, `reverb:room:0.5`, `reverb:mix:0.3`
4. Play note and listen to complete tail decay
5. Test at different room sizes (0.3, 0.5, 0.8)
6. Document results before proceeding

**Estimated time:** 5 minutes
**CPU impact:** None
**Risk:** Very low - easy rollback

---

#### Step A2: Increase Precision Shift

**Hypothesis:** More fractional bits in feedback calculations will smooth the decay curve.

**File:** `include/audio/effects/ReverbEffect.h`

**Current value:**
```cpp
static constexpr int PRECISION_SHIFT = 8;  // 256x precision
```

**Recommended test sequence:**
```cpp
// Test 1:
static constexpr int PRECISION_SHIFT = 10;  // 1024x precision (4x improvement)

// Test 2 (if Test 1 helps but not enough):
static constexpr int PRECISION_SHIFT = 12;  // 4096x precision (16x improvement)
```

**Overflow verification:**
- Max scaled input: `32767 << 12 = 134,213,632` ✅ Fits in int32_t (max ~2.1 billion)
- Max feedback: `134M * 0.94 = ~126M` ✅ Still safe
- No code changes needed - just adjust constant

**Testing procedure:**
1. Change PRECISION_SHIFT constant
2. Compile and test
3. Listen for smoother decay
4. Monitor for any overflow artifacts (clipping, sudden jumps)
5. If 10 helps, try 12 for even better results

**Estimated time:** 2 minutes per test
**CPU impact:** Minimal (same operations, different shift amount)
**Risk:** Low - overflow verified safe

---

### Phase B: Allpass Filter Precision Enhancement 🎯

**Goal:** Apply same PRECISION_SHIFT technique to allpass stage for smoother diffusion

#### Step B1: Enhance Allpass Filter Calculations

**Hypothesis:** Allpass filters are where the "shimmer" and diffusion happen. Quantization here is very audible in the tail.

**File:** `src/audio/effects/ReverbEffect.cpp`

**Current implementation:**
```cpp
int16_t ReverbEffect::processAllpass(AllpassFilter& allpass, int16_t input) {
    int16_t bufferOut = allpass.buffer[allpass.bufferIndex];

    // Simple int32_t math (no precision scaling)
    int32_t output = -input + bufferOut;
    int32_t store = input + (bufferOut >> 1);  // 0.5 feedback

    // Clamp and store
    store = constrain(store, Audio::SAMPLE_MIN, Audio::SAMPLE_MAX);
    allpass.buffer[allpass.bufferIndex] = (int16_t)store;

    // Advance index
    allpass.bufferIndex++;
    if (allpass.bufferIndex >= allpass.bufferSize) {
        allpass.bufferIndex = 0;
    }

    // Clamp output
    output = constrain(output, Audio::SAMPLE_MIN, Audio::SAMPLE_MAX);
    return (int16_t)output;
}
```

**Enhanced implementation with PRECISION_SHIFT:**
```cpp
int16_t ReverbEffect::processAllpass(AllpassFilter& allpass, int16_t input) {
    // Read from buffer at int16_t scale
    int16_t bufferOut = allpass.buffer[allpass.bufferIndex];

    // Scale UP to int32_t with precision bits
    int32_t input32 = (int32_t)input << PRECISION_SHIFT;
    int32_t bufferOut32 = (int32_t)bufferOut << PRECISION_SHIFT;

    // Allpass calculation at high precision
    // output = -input + bufferOut
    int32_t output32 = -input32 + bufferOut32;

    // store = input + (bufferOut * 0.5)
    int32_t store32 = input32 + (bufferOut32 >> 1);

    // Clamp at scaled range
    int32_t maxVal = (int32_t)Audio::SAMPLE_MAX << PRECISION_SHIFT;
    int32_t minVal = (int32_t)Audio::SAMPLE_MIN << PRECISION_SHIFT;
    store32 = constrain(store32, minVal, maxVal);

    // Scale DOWN and store as int16_t
    allpass.buffer[allpass.bufferIndex] = (int16_t)(store32 >> PRECISION_SHIFT);

    // Advance buffer index
    allpass.bufferIndex++;
    if (allpass.bufferIndex >= allpass.bufferIndex) {
        allpass.bufferIndex = 0;
    }

    // Clamp output and scale DOWN
    output32 = constrain(output32, minVal, maxVal);
    return (int16_t)(output32 >> PRECISION_SHIFT);
}
```

**Changes summary:**
- Scale input and buffer output UP by PRECISION_SHIFT
- Perform calculations at high precision
- Scale result DOWN before storing
- All intermediate math at int32_t with fractional bits

**Testing procedure:**
1. Modify `processAllpass()` function
2. Compile and test
3. Focus on listening to diffusion and shimmer in tail
4. Compare to Phase A results
5. Test all 4 allpass filters working correctly

**Estimated time:** 20-30 minutes
**CPU impact:** +1-2% (4 allpass filters with slightly more operations)
**Risk:** Medium - more complex change, test thoroughly
**Benefit:** Should significantly smooth the diffusion stage

---

### Phase C: Advanced Optimization (If Needed)

**Goal:** Completely eliminate float→int16_t quantization in damping filter

#### Step C1: Convert Damping Filter to Full int32_t

**Hypothesis:** The damping filter uses `float filterStore` which creates quantization when it affects int16_t samples. Converting to full int32_t eliminates this.

**Files:** `include/audio/effects/ReverbEffect.h` and `src/audio/effects/ReverbEffect.cpp`

**Current damping filter (in `processComb()`):**
```cpp
// Damping filter state is float
comb.filterStore = (output * comb.damp2) + (comb.filterStore * comb.damp1);

// Noise gate for float
if (comb.filterStore > -FILTER_NOISE_GATE_THRESHOLD &&
    comb.filterStore < FILTER_NOISE_GATE_THRESHOLD) {
    comb.filterStore = 0.0f;
}

// Used in feedback calculation
int32_t feedback32 = (int32_t)(comb.filterStore * comb.feedback * (1 << PRECISION_SHIFT));
```

**Header change (`include/audio/effects/ReverbEffect.h`):**
```cpp
struct CombFilter {
    int16_t* buffer;
    size_t bufferSize;
    size_t bufferIndex;
    float feedback;
    int32_t filterStore;  // CHANGE: float → int32_t (scaled by PRECISION_SHIFT)
    float damp1;
    float damp2;
};
```

**Implementation change (`src/audio/effects/ReverbEffect.cpp` in `processComb()`):**

**Before:**
```cpp
// Apply damping filter (one-pole lowpass) at NORMAL scale
comb.filterStore = (output * comb.damp2) + (comb.filterStore * comb.damp1);

// Noise gate
if (comb.filterStore > -FILTER_NOISE_GATE_THRESHOLD &&
    comb.filterStore < FILTER_NOISE_GATE_THRESHOLD) {
    comb.filterStore = 0.0f;
}
```

**After:**
```cpp
// Scale output to match filterStore precision
int32_t output32 = (int32_t)output << PRECISION_SHIFT;

// Damping filter at full int32_t precision
// filterStore = (output * damp2) + (filterStore * damp1)
int32_t dampedOutput = (int32_t)(output32 * comb.damp2);
int32_t dampedStore = (int32_t)(comb.filterStore * comb.damp1);
comb.filterStore = dampedOutput + dampedStore;

// Noise gate (threshold also scaled)
int32_t gateThreshold = (int32_t)(FILTER_NOISE_GATE_THRESHOLD * (1 << PRECISION_SHIFT));
if (comb.filterStore > -gateThreshold && comb.filterStore < gateThreshold) {
    comb.filterStore = 0;
}

// Use filterStore in feedback calculation (already scaled)
int32_t feedback32 = (int32_t)(comb.filterStore * comb.feedback);
```

**Also update `initCombFilter()` in same file:**
```cpp
void ReverbEffect::initCombFilter(CombFilter& comb, float delayMs) {
    comb.bufferSize = msToSamples(delayMs);
    comb.buffer = new int16_t[comb.bufferSize];
    memset(comb.buffer, 0, comb.bufferSize * sizeof(int16_t));
    comb.bufferIndex = 0;
    comb.filterStore = 0;  // CHANGE: 0.0f → 0 (int32_t)
    comb.feedback = 0.5f;
    comb.damp1 = 0.5f;
    comb.damp2 = 0.5f;
}
```

**Testing procedure:**
1. Modify CombFilter struct in header
2. Update `processComb()` damping calculations
3. Update `initCombFilter()` initialization
4. Compile and test thoroughly
5. Listen for complete elimination of grain
6. Long-term stability test (1+ minutes continuous reverb)

**Estimated time:** 30-40 minutes
**CPU impact:** Might be faster (no float math), or +1-2%
**Risk:** Medium-high - affects core reverb algorithm
**Benefit:** Complete elimination of float→int quantization

---

## Testing Strategy

### Baseline Measurement (Before Any Changes)

1. Enable reverb with standard settings: `reverb:on`, `reverb:room:0.5`, `reverb:damp:0.5`, `reverb:mix:0.3`
2. Play a sustained note and release
3. Listen carefully to tail decay (last 2-3 seconds before silence)
4. Note where graininess is most audible (volume level, frequency)
5. Record CPU usage from performance monitor
6. Try different settings:
   - Large room (0.8) with low damping (0.2) - longest tail
   - Small room (0.3) with high damping (0.8) - shortest tail
   - High mix (0.6) - emphasizes reverb artifacts
7. **Document baseline observations**

### Phase Testing Procedure

**After Each Phase:**
1. Flash updated firmware to hardware
2. Repeat baseline test procedure
3. Compare to previous phase (or baseline if Phase A)
4. Document improvements or regressions
5. Check CPU usage hasn't exceeded budget (< 25% total)
6. Check RAM stable (no leaks)
7. **Only proceed to next phase if previous didn't fully solve issue**

### Success Criteria

**Primary goal:**
- ✅ Reverb tail decays smoothly to silence with no audible grain/buzz

**Secondary goals:**
- ✅ No new artifacts introduced (clicks, pops, distortion)
- ✅ CPU usage remains below 25% total (currently 14.5%, so +10% headroom)
- ✅ RAM usage stable over time (no leaks)
- ✅ Reverb still sounds musical and natural
- ✅ All reverb parameters still work correctly (room, damp, mix)

### Failure Modes to Watch For

1. **Overflow artifacts:** Sudden clipping, distortion, or jumps in level
2. **Instability:** Reverb runaway (feedback explosion)
3. **Performance degradation:** CPU spikes, audio dropouts
4. **Memory leaks:** RAM slowly decreasing over time
5. **Unnatural sound:** Reverb becomes too "digital" or loses character

---

## Implementation Order (Recommended)

### Incremental Approach

**Round 1: Phase A (Quick Wins) - 10 minutes**
1. A1: Reduce noise gate thresholds (100→30, 1.0→0.3)
2. A2: Increase PRECISION_SHIFT (8→10)
3. Test and document results
4. **STOP HERE if graininess is eliminated** ✅

**Round 2: Phase B (Allpass Precision) - 30 minutes**
5. B1: Enhance allpass filter calculations
6. Test and compare to Round 1
7. **STOP HERE if satisfied with results** ✅

**Round 3: Phase C (Advanced) - 40 minutes**
8. C1: Convert damping filter to full int32_t
9. Final comprehensive testing
10. Long-term stability validation

**Total time if all phases needed:** ~1 hour 20 minutes

---

## Rollback Plan

### Version Control Strategy

**Before starting:**
```bash
git status  # Verify clean working directory
git checkout -b reverb-precision-enhancement
git commit -m "Baseline before reverb precision work"
```

**After each phase:**
```bash
git add include/audio/effects/ReverbEffect.h src/audio/effects/ReverbEffect.cpp
git commit -m "Phase A: Reduced noise gate thresholds and increased precision shift"
# Or similar for Phase B, C
```

**If phase fails:**
```bash
git checkout HEAD~1  # Revert last commit
# Or to abandon entire branch:
git checkout main
git branch -D reverb-precision-enhancement
```

### File Backup (Alternative)

If not using git, back up these files before starting:
- `include/audio/effects/ReverbEffect.h`
- `src/audio/effects/ReverbEffect.cpp`

Save as: `ReverbEffect.h.backup`, `ReverbEffect.cpp.backup`

---

## Expected Outcomes

### Phase A Outcome Predictions

**If A1 succeeds (noise gate reduction):**
- Tail decay becomes smoother
- Less abrupt cutoff at silence
- Grain might still be present but less pronounced
- **Likelihood:** 30% - might help but probably not full solution

**If A2 succeeds (PRECISION_SHIFT increase):**
- Decay curve becomes mathematically smoother
- Feedback quantization errors reduced 4x (if shift 8→10) or 16x (if 8→12)
- **Likelihood:** 40% - should provide noticeable improvement

**Combined A1+A2:**
- **Likelihood:** 60% - good chance of acceptable results

### Phase B Outcome Predictions

**If B1 succeeds (allpass precision):**
- Diffusion stage becomes much smoother
- "Shimmer" in tail loses graininess
- Combined with Phase A improvements should be very noticeable
- **Likelihood:** 85% - high probability of solving issue

### Phase C Outcome Predictions

**If C1 needed (full int32 damping):**
- Complete elimination of all quantization sources
- Perfect mathematical precision in feedback loops
- Should produce absolutely smooth decay
- **Likelihood:** 95% - nearly certain to solve if A+B didn't

---

## Performance Impact Estimates

### CPU Usage Projections

**Baseline:** 14.5% (3 osc + 3 effects)
**Phase A:** 14.5% (no change - just constant adjustments)
**Phase B:** 15.5-16.5% (+1-2% for allpass precision)
**Phase C:** 17-18% (+1-2% more for int32 damping)

**Worst case total:** ~18% CPU (still 57% below 75% limit)
**Headroom remaining:** Excellent - plenty of room for future features

### RAM Impact

All phases: **No additional RAM** (no new buffers, just calculation changes)

---

## Documentation Requirements

### During Implementation

**Create/Update:**
- This plan document (update status as phases complete)
- Code comments explaining precision enhancements
- Memory bank files (activeContext.md, progress.md)

### After Completion

**Final Documentation:**
- Update `docs/improvements/REVERB_PRECISION_ENHANCEMENT.md` with:
  - Which phases were implemented
  - Before/after results
  - Final CPU/RAM measurements
  - Lessons learned
  - Recommendations for future similar work

**Update Memory Bank:**
- `memory-bank/activeContext.md` - Document completion
- `memory-bank/progress.md` - Mark Phase G (quality polish) as complete
- `memory-bank/systemPatterns.md` - Add precision enhancement patterns

---

## Future Considerations

### If Graininess Persists After All Phases

**Other potential causes to investigate:**
1. **I2S DAC configuration** - Check bit depth, sample rate alignment
2. **Buffer underruns** - Monitor for audio task timing issues
3. **PCM5102 hardware** - Test with different DAC module or settings
4. **Power supply noise** - Check for electrical interference
5. **Sample rate mismatch** - Verify 22050 Hz throughout pipeline

### Related Improvements

**Delay Effect:**
- Apply similar precision enhancements to DelayEffect
- Currently uses simpler feedback calculation
- Graininess also noted in delay (less than reverb)
- Same techniques should work

**Chorus Effect:**
- Check if interpolation needs higher precision
- Modulated delay might benefit from smoother calculations

---

## Approval & Sign-off

**Plan created:** November 5, 2025
**Created by:** Cline AI Assistant
**Reviewed by:** [User to confirm]

**Approved to proceed:** [ ] Yes / [ ] No / [ ] With modifications

**Modifications requested:**
- [Space for user notes]

---

## Implementation Log

**Phase A1 (Noise Gate Reduction):**
- [x] Status: Complete
- [x] Date: November 5, 2025
- [x] Result: Reduced thresholds from 100→30 (samples) and 1.0→0.3 (filter)
- [x] CPU impact: None (constant change only)

**Phase A2 (PRECISION_SHIFT Increase):**
- [x] Status: Complete
- [x] Date: November 5, 2025
- [x] Result: Increased PRECISION_SHIFT from 8→10 (256x to 1024x precision)
- [x] CPU impact: None (same operations, different shift amount)

**Phase B1 (Allpass Precision):**
- [x] Status: Complete
- [x] Date: November 5, 2025
- [x] Result: Applied PRECISION_SHIFT scaling to all 4 allpass filters for smoother diffusion
- [x] CPU impact: Minimal - only +12 bytes Flash, RAM unchanged at 14.6%

**Phase C1 (Full int32 Damping):**
- [x] Status: Complete
- [x] Date: November 5, 2025
- [x] Result: Converted damping filter from float to full int32_t precision (scaled by PRECISION_SHIFT)
- [x] CPU impact: Likely faster (eliminated float math) - RAM unchanged at 14.6%, Flash -40 bytes (951,425)

**Final Outcome:**
- [ ] Graininess eliminated: Yes / No / Improved
- [ ] Total CPU usage: ___%
- [ ] RAM usage: ___ KB free
- [ ] Recommended for production: Yes / No

---

## References

**Related Files:**
- `include/audio/effects/ReverbEffect.h` - Reverb class header
- `src/audio/effects/ReverbEffect.cpp` - Reverb implementation
- `docs/improvements/DAC_MIGRATION_PCM5102.md` - Background on graininess issue
- `memory-bank/activeContext.md` - Project context
- `memory-bank/progress.md` - Phase G (quality polish) documentation

**External Resources:**
- Freeverb algorithm: https://ccrma.stanford.edu/~jos/pasp/Freeverb.html
- Fixed-point arithmetic best practices
- ESP32 I2S precision considerations

---

**Next Steps:** Review this plan, then toggle to Act Mode to begin Phase A implementation.
