# OTA (Over-The-Air) Update Setup

## Overview

Your theremin now has OTA firmware update capability using ElegantOTA. The ESP32 creates its own WiFi Access Point, allowing you to upload new firmware wirelessly without opening the box.

## What Was Added

### New Files
- `include/OTAManager.h` - OTA manager class definition
- `src/OTAManager.cpp` - OTA implementation with AP mode
- `OTA_SETUP.md` - This documentation

### Modified Files
- `platformio.ini` - Added ElegantOTA library and ENABLE_OTA flag
- `src/main.cpp` - Integrated OTA manager with conditional compilation

## How to Use

### 1. Upload Firmware with OTA Enabled
```bash
pio run -e esp32dev -t upload
```

### 2. Power On Your Theremin
The ESP32 will:
- Start the theremin normally
- Create WiFi network: **"Theremin-OTA"** (open, no password)
- Enable OTA web interface at: **http://192.168.4.1/update**

### 3. Connect and Upload
1. **Connect** your phone/computer to WiFi network: `Theremin-OTA`
2. **Open browser** and go to: `http://192.168.4.1/update`
3. **Login** with:
   - Username: `admin`
   - Password: `theremin`
4. **Select firmware** file (`.pio/build/esp32dev/firmware.bin`)
5. **Upload** - Progress will be shown
6. **Wait** for automatic reboot

## Configuration

### Change WiFi AP Name
Edit in `src/main.cpp`:
```cpp
OTAManager ota("YourNewName", "");
```

### Add WiFi Password
Edit in `src/main.cpp` (minimum 8 characters):
```cpp
OTAManager ota("Theremin-OTA", "your_password");
```

### Change OTA Login Credentials
Edit in `src/main.cpp`:
```cpp
ota.begin("new_username", "new_password");
```

### Remove OTA Authentication (not recommended)
Edit in `src/main.cpp`:
```cpp
ota.begin("", "");  // No authentication
```

## Disabling OTA

### Option 1: Compile-Time Disable
Edit `platformio.ini` and remove the `-DENABLE_OTA=1` line:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
build_flags =
    -DDEBUG_MODE=1
    # -DENABLE_OTA=1  <- Comment out or remove this line
lib_deps =
    adafruit/Adafruit_VL53L0X@^1.2.0
    # ayushsharma82/ElegantOTA@^3.1.0  <- Optional: also comment this
```

This completely removes OTA code from the binary (saves ~100-150 KB Flash).

### Option 2: Runtime Button Activation (Implemented)

OTA now supports **optional button activation** to save RAM when not needed.

**How It Works:**
- If button not pressed during boot → OTA disabled, saves ~50-70 KB RAM
- If button pressed during boot → OTA enabled at 192.168.4.1/update
- Currently set to always-on for testing (see below)

**Configure the Enable Pin:**

Edit `OTA_ENABLE_PIN` macro in `src/main.cpp`:

```cpp
#ifdef ENABLE_OTA
#include "OTAManager.h"

// Set to -1 to always enable OTA (no button check)
// Set to GPIO pin number to require button press during boot
#define OTA_ENABLE_PIN -1  // Change to your chosen GPIO (e.g., 2, 4, 15)
#endif
```

**Pin Configuration:**
```cpp
#define OTA_ENABLE_PIN -1   // Always enable OTA (current default)
#define OTA_ENABLE_PIN 2    // Check GPIO 2 (active LOW with pullup)
#define OTA_ENABLE_PIN 4    // Check GPIO 4
// etc.
```

**Usage:**
1. Connect button between your chosen GPIO and GND
2. Hold button while powering on ESP32
3. Serial output will show: `[OTA] Enable button detected, starting OTA...`
4. Connect to "Theremin-OTA" and upload firmware

**Benefits:**
- **RAM savings:** ~50-70 KB when disabled (WiFi stack not loaded)
- **Flexibility:** Enable OTA only when needed
- **Simple:** One macro to configure, button logic handled internally

## Resource Usage

### With OTA Enabled
- **RAM**: ~48 KB (14.5% of 320 KB)
- **Flash**: ~847 KB (64.6% of 1.3 MB)
- **WiFi Active**: Uses some CPU for handling requests

### With OTA Disabled
- **RAM**: Saves ~10-15 KB
- **Flash**: Saves ~100-150 KB
- **WiFi Off**: No background tasks

## Troubleshooting

### Can't See WiFi Network
- Check serial output for "Access Point IP: 192.168.4.1"
- Try power cycling the ESP32
- Ensure OTA is enabled in platformio.ini

### Can't Connect to WiFi
- Network is open (no password by default)
- Some phones may warn about unsecured network
- Try forgetting and reconnecting

### Upload Fails
- Ensure you're connected to "Theremin-OTA" network
- Check you're using correct credentials (admin/theremin)
- Verify firmware.bin file is not corrupted
- Try smaller firmware file if timeout occurs

### IP Address Not Working
- ESP32 AP always uses 192.168.4.1
- If changed, check serial output for actual IP
- Ensure browser is not using HTTPS (use http://)

## Security Notes

- **AP is open** (no password) - anyone nearby can connect
- **OTA page is protected** with username/password
- For production, consider:
  - Adding AP password
  - Stronger OTA credentials
  - Runtime enable/disable via button
  - Timeout after X minutes of inactivity

## Build Commands

```bash
# Build with OTA enabled (default)
pio run -e esp32dev

# Upload to hardware
pio run -e esp32dev -t upload

# Monitor serial output
pio device monitor

# Clean build
pio run -t clean
```

## Technical Details

- **Library**: ElegantOTA v3.1.7
- **Web Server**: Standard ESP32 WebServer (port 80)
- **WiFi Mode**: Access Point (AP)
- **Default IP**: 192.168.4.1
- **Update Method**: HTTP POST with chunked upload
- **Authentication**: HTTP Basic Auth
- **Auto-Reboot**: Yes (after successful upload)

## Future Enhancements

Ideas for improvement:
- Button-activated OTA (hold during boot)
- Timeout and auto-disable after 5 minutes
- LED indicator when OTA is active
- Station mode as alternative (connect to existing WiFi)
- mDNS support (access via theremin.local)
- Progress LED during upload
