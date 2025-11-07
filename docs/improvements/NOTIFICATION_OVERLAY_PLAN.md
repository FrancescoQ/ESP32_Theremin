# Notification Overlay Implementation Plan

**Date:** November 7, 2025
**Status:** Planning Phase
**Goal:** Add time-limited status notifications for control changes (oscillators, effects, sensors)

---

## Overview

Implement a NotificationManager system that displays brief status messages when parameters change, showing information like:
- `OSC1:SIN` - Oscillator 1 set to Sine wave
- `OSC2:+1` - Oscillator 2 octave up
- `REV:LNG` - Reverb set to Long preset
- `DLY:OFF` - Delay turned off
- `SMT:MIN` - Smoothing set to minimum
- `RNG:NRM` - Frequency range set to normal

## Architecture Decision: NotificationManager Class

### Why NotificationManager?

**Rejected Options:**
1. ❌ **Centralized in GPIOControls** - Only works for GPIO changes, ignores WebUI/Serial
2. ❌ **Component-Level** - Each component manages its own overlay (inconsistent formatting)

**Selected Option:**
3. ✅ **NotificationManager Class** - Best of both worlds

### Benefits:
- **Control-source agnostic** - Works from GPIO, Serial, WebUI, or future MIDI
- **Centralized formatting** - Consistent look and positioning
- **Auto-hide timer** - Notifications disappear after 2-3 seconds
- **Queue support** - Handle multiple rapid changes gracefully
- **Decoupled** - Components don't need to know about control sources

---

## Implementation Plan

### Phase 1: Create NotificationManager Class

**Files to Create:**
- `include/system/NotificationManager.h`
- `src/system/NotificationManager.cpp`

**Key Methods:**
```cpp
class NotificationManager {
public:
    NotificationManager(DisplayManager* display);

    // Show notification for specified duration
    void show(String message, uint16_t durationMs = 2000);

    // Update - call in main loop to handle timing
    void update();

    // Clear current notification immediately
    void clear();

private:
    DisplayManager* displayManager;
    String currentMessage;
    unsigned long hideTime;  // millis() timestamp when to hide
    bool active;
};
```

**Overlay Registration:**
- Register single overlay callback with DisplayManager
- Draw notification in top-center or bottom-center position
- Use TomThumb font for compact display
- Optional: Background box for contrast

### Phase 2: Integration Points

**AudioEngine - Oscillator Changes:**
```cpp
// In AudioEngine::setOscillatorWaveform()
notificationManager->show("OSC" + String(oscNum) + ":" + getWaveformShortName(waveform));

// In AudioEngine::setOscillatorOctave()
notificationManager->show("OSC" + String(oscNum) + ":" +
                          (octave > 0 ? "+" : "") + String(octave));
```

**Effects - Preset Changes:**
```cpp
// In ReverbEffect::setPreset()
const char* presetNames[] = {"OFF", "SML", "NRM", "MAX"};
notificationManager->show("REV:" + String(presetNames[preset]));

// Similar for DelayEffect and ChorusEffect
```

**SensorManager/Theremin - Smoothing & Range:**
```cpp
// In Theremin::setPitchSmoothingPreset()
const char* smoothNames[] = {"MIN", "NRM", "MAX"};
notificationManager->show("SMT:" + String(smoothNames[preset]));

// In Theremin::setFrequencyRangePreset()
const char* rangeNames[] = {"NAR", "NRM", "WID"};
notificationManager->show("RNG:" + String(rangeNames[preset]));
```

### Phase 3: Visual Design

**Display Position Options:**
1. **Top-center** (below page indicator)
   - Pro: Doesn't interfere with page content
   - Con: Overlaps with page titles

2. **Bottom-center** (recommended)
   - Pro: Clear visibility, separated from page content
   - Con: May overlap with bottom content on some pages

**Formatting:**
- Font: TomThumb (3x5px) for compact display
- Background: Optional filled rectangle for contrast
- Border: Optional thin border
- Alignment: Centered horizontally

**Example Layout:**
```
┌──────────────────────────┐
│ SYSTEM            1/2    │ ← Page title + indicator
│ ──────────────────────── │
│                          │
│ Status: OK               │ ← Page content
│ Audio: 1.6ms/11ms        │
│ RAM: 314 KB free         │
│                          │
│                          │
│    ╔════════════╗        │
│    ║ OSC1:SIN   ║        │ ← Notification overlay
│    ╚════════════╝        │
└──────────────────────────┘
```

### Phase 4: Testing & Refinement

**Test Scenarios:**
1. Single change - notification appears and auto-hides
2. Rapid changes - queue or replace current notification
3. Page navigation - notification persists across pages
4. Long text - ensure it fits within display bounds
5. Timing - verify 2-3 second duration feels right

**Performance Considerations:**
- Minimal CPU overhead (simple string display)
- No heap allocation in update() (pre-allocated string)
- RAM impact: ~50 bytes for NotificationManager instance

---

## Notification Format Reference

### Oscillator Changes
| Action | Notification |
|--------|-------------|
| OSC1 → Sine | `OSC1:SIN` |
| OSC1 → Square | `OSC1:SQR` |
| OSC1 → Triangle | `OSC1:TRI` |
| OSC1 → Sawtooth | `OSC1:SAW` |
| OSC1 → Off | `OSC1:OFF` |
| OSC2 → Octave +1 | `OSC2:+1` |
| OSC3 → Octave -1 | `OSC3:-1` |

### Effect Changes
| Action | Notification |
|--------|-------------|
| Reverb → Off | `REV:OFF` |
| Reverb → Small | `REV:SML` |
| Reverb → Normal | `REV:NRM` |
| Reverb → Max | `REV:MAX` |
| Delay → Off | `DLY:OFF` |
| Delay → Short | `DLY:SHT` |
| Delay → Medium | `DLY:MED` |
| Delay → Long | `DLY:LNG` |
| Chorus → Off | `CHR:OFF` |
| Chorus → Min | `CHR:MIN` |
| Chorus → Medium | `CHR:MED` |
| Chorus → Max | `CHR:MAX` |

### Sensor/Control Changes
| Action | Notification |
|--------|-------------|
| Smoothing → None | `SMT:MIN` |
| Smoothing → Normal | `SMT:NRM` |
| Smoothing → Extra | `SMT:MAX` |
| Range → Narrow | `RNG:NAR` |
| Range → Normal | `RNG:NRM` |
| Range → Wide | `RNG:WID` |
| Mix → Equal | `MIX:EQL` |
| Mix → Primary | `MIX:PRI` |
| Mix → Gradient | `MIX:GRD` |

---

## Future Enhancements

### Queue System (Optional)
- Support showing multiple notifications in sequence
- Useful for preset changes that affect multiple parameters
- Example: Reverb preset change shows "REV:LNG" then "MIX:0.5"

### Icons (Optional)
- Add small icons next to text (waveform shapes, effect symbols)
- Requires custom bitmap fonts or sprite graphics

### Animation (Optional)
- Fade in/out effects
- Slide in from top/bottom
- May impact performance - test carefully

---

## Implementation Checklist

### Phase 1: Core Class
- [ ] Create `NotificationManager.h` header
- [ ] Create `NotificationManager.cpp` implementation
- [ ] Add constructor with DisplayManager pointer
- [ ] Implement `show()` method
- [ ] Implement `update()` method with timer logic
- [ ] Implement `clear()` method
- [ ] Register overlay callback in constructor

### Phase 2: Integration
- [ ] Add NotificationManager instance to Theremin class
- [ ] Pass to AudioEngine constructor
- [ ] Add notifications to `setOscillatorWaveform()`
- [ ] Add notifications to `setOscillatorOctave()`
- [ ] Add notifications to `setOscillatorVolume()` (mix presets)
- [ ] Pass to each Effect via EffectsChain
- [ ] Add notifications to ReverbEffect::setPreset()
- [ ] Add notifications to DelayEffect::setPreset()
- [ ] Add notifications to ChorusEffect::setPreset()
- [ ] Add notifications to smoothing preset changes
- [ ] Add notifications to frequency range changes

### Phase 3: Testing
- [ ] Build and test on hardware
- [ ] Verify notification appears on control changes
- [ ] Verify auto-hide timing (adjust if needed)
- [ ] Test rapid changes (ensure no crashes)
- [ ] Test across page navigation
- [ ] Verify no performance impact
- [ ] Get user feedback on visibility and timing

### Phase 4: Documentation
- [ ] Update memory-bank/activeContext.md
- [ ] Update memory-bank/progress.md
- [ ] Create usage guide in docs/guides/ (if needed)
- [ ] Update ARCHITECTURE.md with NotificationManager

---

## Notes

- **Memory Impact:** Minimal - one String instance + timestamps (~100 bytes)
- **CPU Impact:** Negligible - simple string rendering, only when active
- **Display Compatibility:** Works with existing overlay system
- **Control Source Agnostic:** Future-proof for WebUI, MIDI, etc.

---

## Related Files

- `include/system/DisplayManager.h` - Overlay registration system
- `src/controls/GPIOControls.cpp` - Physical switch handlers
- `src/audio/AudioEngine.cpp` - Oscillator control
- `include/audio/effects/ReverbEffect.h` - Effect preset system
- `include/system/Theremin.h` - Smoothing/range presets
