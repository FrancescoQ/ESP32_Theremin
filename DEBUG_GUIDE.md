# Debug System Guide - ESP32 Theremin

## Overview

The ESP32 Theremin uses a compile-time debug macro system that provides **zero-cost debugging** when disabled. All debug output can be completely removed from the compiled binary by setting a single flag.

## Quick Start

### Default Behavior

Debug output is **enabled by default** for development. All builds will show debug messages unless you explicitly use a production build configuration.

### Disabling Debug Output

#### Option 1: Use Production Build Environment (Recommended)

```bash
# Simulation - no debug output
pio run -e esp32dev-wokwi-production

# Hardware - no debug output
pio run -e esp32dev-production
```

#### Option 2: Edit Debug.h

Open `include/Debug.h` and change:
```cpp
#define DEBUG_MODE 1  // ← Change to 0
```

## Build Configurations

The `platformio.ini` file provides four build environments:

### Development Builds (Debug Enabled)
- **`esp32dev-wokwi`** - Simulation with debug output
- **`esp32dev`** - Hardware with debug output

### Production Builds (Debug Disabled)
- **`esp32dev-wokwi-production`** - Simulation, no debug output
- **`esp32dev-production`** - Hardware, no debug output

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
1. Keep `DEBUG_MODE=1` while developing
2. Use debug output to verify sensor readings
3. Monitor for errors and warnings
4. Use `esp32dev-wokwi` or `esp32dev` environments

### Production/Deployment
1. Build with production environments:
   - `pio run -e esp32dev-wokwi-production`
   - `pio run -e esp32dev-production`
2. Saves flash space for future features
3. Slightly faster startup (no Serial.begin delay)
4. Cleaner for battery-powered operation

### Battery Operation
For battery-powered theremin:
```bash
# Use production build to save power
pio run -e esp32dev-production -t upload
```

Debug output disabled saves:
- UART peripheral power (~1-2mA)
- Serial.begin() startup time (~100ms)
- String formatting overhead

## Testing Both Modes

### Test Debug Enabled
```bash
pio run -e esp32dev-wokwi
```
Should see debug output in serial monitor.

### Test Debug Disabled
```bash
pio run -e esp32dev-wokwi-production
```
Should compile successfully with smaller binary size, no debug output.

## Troubleshooting

### "Undefined reference to Serial"
If you get this error, you're using `Serial.print()` directly instead of `DEBUG_PRINT()`.

**Fix:** Replace all `Serial.*` calls with `DEBUG_*` macros.

### Debug Output Not Showing
1. Check you're using debug-enabled build: `esp32dev-wokwi` or `esp32dev`
2. Verify serial monitor is open at 9600 baud
3. Check `DEBUG_MODE` is `1` in `Debug.h` or `platformio.ini`

### Can't Disable Debug Output
1. Make sure you're using production build environment
2. Check for any direct `Serial.print()` calls (bypass the macros)
3. Rebuild from scratch: `pio run -t clean` then `pio run -e esp32dev-production`

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
