# Debug System Guide - ESP32 Theremin

## Overview

The ESP32 Theremin uses a compile-time debug macro system that provides **zero-cost debugging** when disabled. All debug output can be completely removed from the compiled binary by setting a single flag.

## Quick Start

### Default Behavior

Debug output is **enabled by default** for development. All builds will show debug messages unless you explicitly use a production build configuration.

### Disabling Debug Output

#### Option 1: Edit platformio.ini (Recommended)

Open `platformio.ini` and change the `esp32dev` environment:
```ini
[env:esp32dev]
build_flags =
    -DDEBUG_MODE=0  # ← Change from 1 to 0
```

#### Option 2: Edit Debug.h

Open `include/Debug.h` and change:
```cpp
#define DEBUG_MODE 1  // ← Change to 0
```

**Note:** The simulation environment (`esp32dev-wokwi`) always has debug enabled since it's used for development and testing.

## Build Configurations

The `platformio.ini` file provides two build environments:

### Available Environments
- **`esp32dev-wokwi`** - Simulation with debug output (always enabled)
- **`esp32dev`** - Hardware with debug output (enabled by default)

**For production builds:** Simply change `DEBUG_MODE=1` to `DEBUG_MODE=0` in the `esp32dev` environment.

## Memory Savings

When `DEBUG_MODE=0`, the compiler completely removes all debug code:

| Build Type | Flash Usage | RAM Usage |
|-----------|-------------|-----------|
| Debug (MODE=1) | 283,341 bytes | 21,808 bytes |
| Production (MODE=0) | 253,325 bytes | 21,472 bytes |
| **Savings** | **~30 KB** | **~336 bytes** |

## Available Debug Macros

### DEBUG_INIT(baudrate)
Initializes serial communication.

**Usage:**
```cpp
void setup() {
    DEBUG_INIT(9600);  // Replaces Serial.begin(9600)
}
```

**Expands to:**
- `DEBUG_MODE=1`: `Serial.begin(9600)`
- `DEBUG_MODE=0`: `((void)0)` (removed entirely)

### DEBUG_PRINT(value)
Prints value without newline.

**Usage:**
```cpp
DEBUG_PRINT("Frequency: ");
DEBUG_PRINT(frequency);
DEBUG_PRINT(" Hz");
```

**Expands to:**
- `DEBUG_MODE=1`: `Serial.print(...)`
- `DEBUG_MODE=0`: `((void)0)` (removed entirely)

### DEBUG_PRINTLN(value)
Prints value with newline.

**Usage:**
```cpp
DEBUG_PRINTLN("System initialized");
DEBUG_PRINTLN(temperature);
```

**Expands to:**
- `DEBUG_MODE=1`: `Serial.println(...)`
- `DEBUG_MODE=0`: `((void)0)` (removed entirely)

### DEBUG_PRINTF(format, ...)
Formatted print (like C printf).

**Usage:**
```cpp
DEBUG_PRINTF("Freq: %d Hz, Vol: %d%%\n", freq, volume);
```

**Expands to:**
- `DEBUG_MODE=1`: `Serial.printf(...)`
- `DEBUG_MODE=0`: `((void)0)` (removed entirely)

## How It Works

### Conditional Compilation

The debug macros use C preprocessor conditional compilation:

```cpp
#if DEBUG_MODE
  #define DEBUG_PRINT(x) Serial.print(x)
#else
  #define DEBUG_PRINT(x) ((void)0)
#endif
```

When `DEBUG_MODE=0`:
1. Preprocessor replaces `DEBUG_PRINT("Hello")` with `((void)0)`
2. Compiler optimizes away `((void)0)` (it does nothing)
3. Final binary has **no trace** of the debug code

### Zero Runtime Overhead

Unlike runtime flags (e.g., `if (debugEnabled)`), these macros have:
- ✅ **Zero CPU overhead** - code doesn't exist
- ✅ **Zero RAM overhead** - no variables or strings
- ✅ **Zero flash overhead** - not compiled into binary
- ✅ **No if-checks** - completely eliminated

## Examples

### Before: Direct Serial Calls

```cpp
void setup() {
    Serial.begin(9600);
    Serial.println("Starting...");
}

void loop() {
    Serial.print("Value: ");
    Serial.println(value);
}
```

**Problem:** Code always compiled, always uses memory.

### After: Debug Macros

```cpp
#include "Debug.h"

void setup() {
    DEBUG_INIT(9600);
    DEBUG_PRINTLN("Starting...");
}

void loop() {
    DEBUG_PRINT("Value: ");
    DEBUG_PRINTLN(value);
}
```

**Benefit:** Set `DEBUG_MODE=0` and all debug code vanishes!

## Current Debug Output

When debug is enabled, you'll see output like:

```
=== ESP32 Theremin Initializing ===
[MODE] Simulation (Potentiometers)
[SENSOR] Analog inputs configured (GPIO34, GPIO35)
[AUDIO] PWM configured on GPIO25
=== Initialization Complete ===

=== Ready to Play! ===

[PITCH] 150mm → 1500Hz  |  [VOLUME] 200mm → 45%
[PITCH] 148mm → 1520Hz  |  [VOLUME] 195mm → 48%
[PITCH] 145mm → 1550Hz  |  [VOLUME] 190mm → 52%
```

This output is throttled (every 10th loop) to avoid flooding the serial monitor.

## Best Practices

### Development
1. Keep `DEBUG_MODE=1` in `platformio.ini` while developing
2. Use debug output to verify sensor readings
3. Monitor for errors and warnings
4. Use `esp32dev-wokwi` for simulation or `esp32dev` for hardware

### Production/Deployment
1. Change `DEBUG_MODE=1` to `DEBUG_MODE=0` in `platformio.ini` for the `esp32dev` environment
2. Build with: `pio run -e esp32dev`
3. Saves ~30KB flash space for future features
4. Slightly faster startup (no Serial.begin delay)
5. Cleaner for battery-powered operation

### Production Build Checklist
When preparing for final deployment:
- [ ] Change `DEBUG_MODE=1` to `DEBUG_MODE=0` in `platformio.ini`
- [ ] Clean build: `pio run -t clean`
- [ ] Build production firmware: `pio run -e esp32dev`
- [ ] Test on real hardware
- [ ] Verify no serial output appears
- [ ] Upload to ESP32: `pio run -e esp32dev -t upload`

### Battery Operation
For battery-powered theremin:
1. Set `DEBUG_MODE=0` in `platformio.ini`
2. Build and upload:
```bash
pio run -e esp32dev -t upload
```

Debug output disabled saves:
- UART peripheral power (~1-2mA)
- Serial.begin() startup time (~100ms)
- String formatting overhead

## Testing Both Modes

### Test Debug Enabled (Default)
```bash
pio run -e esp32dev
```
Should compile and show debug output when uploaded.

### Test Debug Disabled
1. Edit `platformio.ini`, change `DEBUG_MODE=1` to `DEBUG_MODE=0`
2. Build:
```bash
pio run -e esp32dev
```
Should compile successfully with smaller binary size (~30KB less flash).

## Troubleshooting

### "Undefined reference to Serial"
If you get this error, you're using `Serial.print()` directly instead of `DEBUG_PRINT()`.

**Fix:** Replace all `Serial.*` calls with `DEBUG_*` macros.

### Debug Output Not Showing
1. Check you're using debug-enabled build: `esp32dev-wokwi` or `esp32dev`
2. Verify serial monitor is open at 9600 baud
3. Check `DEBUG_MODE` is `1` in `Debug.h` or `platformio.ini`

### Can't Disable Debug Output
1. Make sure `DEBUG_MODE=0` is set in `platformio.ini`
2. Check for any direct `Serial.print()` calls (bypass the macros)
3. Rebuild from scratch: `pio run -t clean` then `pio run -e esp32dev`

## Advanced Usage

### Custom Debug Levels

You could extend `Debug.h` to support log levels:

```cpp
#define DEBUG_LEVEL_NONE  0
#define DEBUG_LEVEL_ERROR 1
#define DEBUG_LEVEL_INFO  2
#define DEBUG_LEVEL_DEBUG 3

#define DEBUG_LEVEL DEBUG_LEVEL_INFO

#if DEBUG_LEVEL >= DEBUG_LEVEL_ERROR
  #define DEBUG_ERROR(x) Serial.println(x)
#else
  #define DEBUG_ERROR(x) ((void)0)
#endif

#if DEBUG_LEVEL >= DEBUG_LEVEL_INFO
  #define DEBUG_INFO(x) Serial.println(x)
#else
  #define DEBUG_INFO(x) ((void)0)
#endif
```

Then use:
```cpp
DEBUG_ERROR("Critical failure!");  // Always shows
DEBUG_INFO("Sensor initialized");  // Shows at INFO and DEBUG
DEBUG_DEBUG("Raw ADC: 2048");      // Only shows at DEBUG
```

## Summary

The debug macro system provides professional-grade debugging capabilities with **zero overhead** when disabled. It's the same pattern used in production embedded systems worldwide.

**Key Points:**
- ✅ Zero cost when disabled
- ✅ Simple on/off switch
- ✅ ~30KB flash savings
- ✅ Professional embedded practice
- ✅ Easy to use - just replace `Serial.*` with `DEBUG_*`
