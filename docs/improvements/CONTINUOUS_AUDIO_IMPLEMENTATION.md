# Continuous Audio Implementation - FreeRTOS Task

## Problem Solved
The I2S audio was "steppy" or "choppy" because audio buffers were only generated during `loop()` calls, which happened every ~80ms due to slow sensor reads. Each buffer only contained ~11.6ms of audio, leaving ~68ms gaps between buffers.

## Solution: FreeRTOS Audio Task
Implemented continuous audio generation using a dedicated FreeRTOS task that runs independently of sensor reading.

## Architecture Changes

### 1. AudioEngine.h
**Added:**
- FreeRTOS task support (`#include <freertos/FreeRTOS.h>` and `<freertos/task.h>`)
- Task management members:
  - `TaskHandle_t audioTaskHandle` - Handle to audio generation task
  - `SemaphoreHandle_t paramMutex` - Mutex for thread-safe parameter updates
  - `volatile bool taskRunning` - Flag to control task lifecycle
- New methods:
  - `startAudioTask()` - Starts continuous audio generation
  - `stopAudioTask()` - Stops audio generation task
  - `audioTaskFunction()` - Static wrapper for FreeRTOS
  - `audioTaskLoop()` - Instance method with continuous generation loop

**Modified:**
- `update()` - Now deprecated (kept for backward compatibility but does nothing)

### 2. AudioEngine.cpp
**Constructor:**
- Initializes task handle, mutex, and running flag
- Creates mutex with `xSemaphoreCreateMutex()`

**begin():**
- Now automatically calls `startAudioTask()` after I2S setup
- Audio task starts immediately on initialization

**setFrequency() and setAmplitude():**
- Now thread-safe using mutex locking
- Main loop (sensor task) can safely update parameters
- Audio task safely reads parameters

**generateAudioBuffer():**
- Reads parameters with non-blocking mutex attempt
- Applies amplitude smoothing
- Generates buffer and writes to I2S (blocks until DMA ready)

**startAudioTask():**
- Creates high-priority task on Core 1 (app core)
- Task configuration:
  - Name: "AudioTask"
  - Stack: 4096 bytes
  - Priority: 2 (higher than default 1)
  - Core: 1 (separate from sensor reads on Core 0)

**audioTaskLoop():**
- Runs continuously while `taskRunning` is true
- Calls `generateAudioBuffer()` repeatedly
- No explicit delay needed - `i2s_write()` naturally paces the loop
- Blocks until DMA buffer available (~11ms intervals)

## How It Works

### Dual-Core Architecture
```
Core 0 (Protocol CPU)        Core 1 (App CPU)
─────────────────────        ────────────────
Main loop():                 Audio Task:
  ├─ Read sensors (60ms)       ├─ Generate buffer (2ms)
  ├─ Update freq/amp           ├─ Write to I2S (blocks ~11ms)
  └─ delay(20ms)               ├─ Generate buffer (2ms)
Total: ~80ms/loop              ├─ Write to I2S (blocks ~11ms)
                               └─ Continuous...
```

### Thread-Safe Communication
- Sensor task (main loop) updates `currentFrequency` and `currentAmplitude` via mutex
- Audio task reads these values independently
- If sensors are slow, audio continues with last values (smooth!)
- When sensors update, audio picks up new values in ~11ms

### Timing
- **Buffer duration:** 256 samples ÷ 22050 Hz = 11.6ms
- **Generation time:** ~1-2ms (fast!)
- **I2S DMA pacing:** Automatic via `i2s_write()` blocking
- **Sensor update rate:** ~12.5 Hz (every 80ms) - more than adequate for musical control

## Benefits
1. **Eliminates audio gaps completely** - continuous buffer generation
2. **True real-time audio** - independent of sensor timing
3. **Proper CPU core usage** - sensors on Core 0, audio on Core 1
4. **Thread-safe** - mutex protects shared parameters
5. **Minimal overhead** - RAM usage actually decreased slightly
6. **Responsive control** - sensor updates appear in audio within ~11ms

## Testing Instructions

### Expected Behavior
1. **Smooth, continuous tone** - no gaps, stutters, or clicks
2. **Responsive control** - sensor changes heard immediately
3. **Stable operation** - no crashes or audio dropouts

### Serial Monitor
Look for these messages on startup:
```
[AUDIO] I2S DAC initialized on GPIO25 @ 22050 Hz
[AUDIO] Continuous audio task started on Core 1
[AUDIO] Audio task loop started
```

### Audio Quality Tests
1. **Steady hand test:** Hold hand at constant distance from pitch sensor
   - Should hear pure, continuous tone (no stepping)

2. **Slow sweep test:** Slowly move hand through sensor range
   - Frequency should change smoothly, no glitches

3. **Fast sweep test:** Quickly move hand back and forth
   - Audio should remain continuous, no gaps or clicks

4. **Volume control test:** Vary distance from volume sensor
   - Volume should fade smoothly (thanks to amplitude smoothing)

### Debug Mode
If issues arise, check serial output for:
- Task start/stop messages
- Any error messages from I2S driver
- Frequency/amplitude updates (if debug enabled in Theremin class)

## Resource Usage
- **RAM:** 47,584 bytes (14.5%) - within budget
- **Flash:** 856,877 bytes (65.4%) - within budget
- **CPU Core 1:** Audio task at priority 2
- **Mutex overhead:** Minimal (<100 bytes)
- **Task stack:** 4096 bytes

## Backward Compatibility
The deprecated `update()` method is kept as a no-op to maintain compatibility with existing code. However, it's no longer necessary - audio generation happens automatically after `begin()`.

## Future Enhancements
If needed, we could:
1. Add task priority adjustment methods
2. Implement dynamic buffer sizing based on CPU load
3. Add audio dropout detection and recovery
4. Expose task statistics (CPU usage, buffer fill rate)

## Code Location
- `include/AudioEngine.h` - Class declaration with task support
- `src/AudioEngine.cpp` - Implementation with FreeRTOS task
- Main loop (src/main.cpp) - No changes needed!
