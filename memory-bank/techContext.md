# Tech Context - ESP32 Theremin

## Technology Stack

### Microcontroller Platform
**ESP32 (Espressif Systems)**
- 32-bit dual-core Xtensa LX6 microprocessor
- Clock frequency: 160-240 MHz
- RAM: 520 KB SRAM
- Flash: 4 MB (typical on dev boards)
- Built-in WiFi/Bluetooth (not used in this project)
- 34x GPIO pins with multiple functions

**Why ESP32:**
- Sufficient processing power for real-time audio synthesis
- Multiple PWM channels (16 independent)
- Hardware I2C support
- Well-supported by Arduino framework
- Affordable and readily available
- Easy USB programming interface

### Development Environment

**IDE: Visual Studio Code**
- Lightweight, cross-platform
- Excellent extension ecosystem
- Integrated terminal for serial monitoring

**Build System: PlatformIO**
- Modern embedded development platform
- Automatic dependency management
- Multiple framework support
- Built-in library manager
- Serial monitor integration
- Superior to Arduino IDE for professional development
- **Installation Path:** `/Users/fquagliati/.platformio/penv/bin/pio`

**Framework: Arduino**
- Familiar, well-documented API
- Extensive library ecosystem
- Simplifies ESP32 peripheral access
- Good balance of ease-of-use and control

**Simulation: Wokwi Simulator** ✅ IMPLEMENTED
- VSCode plugin with active license
- Virtual circuit testing before hardware assembly
- Interactive debugging
- Component library includes ESP32, potentiometers, buzzer
- **Strategy:** Use potentiometers to simulate VL53L0X distance sensors
- **Benefit:** Tests audio synthesis logic without sensor dependencies

### Key Libraries

**Core Sensor Libraries:**

**Adafruit_VL53L0X v1.2.0+**
- Purpose: VL53L0X Time-of-Flight sensor interface
- Features:
  - I2C communication abstraction
  - Continuous ranging mode
  - Address configuration support
  - Error handling
- Repository: https://github.com/adafruit/Adafruit_VL53L0X
- Installation: Auto-managed by PlatformIO

**Wire.h (Arduino Core)**
- Purpose: I2C communication
- Built into Arduino framework
- Functions used:
  - `Wire.begin(SDA, SCL)` - Initialize I2C bus
  - `Wire.setClock(frequency)` - Set bus speed
- Default I2C pins on ESP32: GPIO21 (SDA), GPIO22 (SCL)

---

**Display & Control Libraries:**

**Adafruit_SSD1306 v2.5.7+**
- Purpose: SSD1306 OLED display driver (128x64)
- Features:
  - I2C communication (address 0x3C)
  - Text rendering with multiple fonts
  - Graphics primitives (lines, circles, etc.)
  - Double buffering for flicker-free updates
- Repository: https://github.com/adafruit/Adafruit_SSD1306
- Installation: Auto-managed by PlatformIO

**Adafruit_GFX v1.11.9+**
- Purpose: Graphics library (required by SSD1306)
- Features:
  - Text rendering engine
  - TomThumb font support (3x5px - very compact)
  - Drawing primitives
- Repository: https://github.com/adafruit/Adafruit-GFX-Library

**Adafruit_MCP23017 v2.3.2+**
- Purpose: MCP23017 I2C GPIO expander driver
- Features:
  - 16 GPIO pins via I2C (address 0x20)
  - Interrupt support
  - Input pull-up configuration
  - Used for physical switches (15 pins)
- Repository: https://github.com/adafruit/Adafruit_MCP23017
- Installation: Auto-managed by PlatformIO

---

**Network & Web Libraries:**

**ESP32Async/ESPAsyncWebServer v3.8.1+**
- Purpose: Asynchronous web server for ESP32
- Features:
  - Non-blocking HTTP server
  - WebSocket support
  - Static file serving (LittleFS/SPIFFS)
  - Multiple simultaneous connections
  - AsyncTCP included transitively
- Repository: https://github.com/ESP32Async/ESPAsyncWebServer
- Installation: Auto-managed by PlatformIO
- Note: Most actively maintained fork

**tzapu/WiFiManager v2.0.17+**
- Purpose: WiFi configuration with captive portal
- Features:
  - Auto-connect to saved WiFi
  - Captive portal for initial setup
  - AP+STA mode support
  - Credential persistence
  - Configurable timeout
- Repository: https://github.com/tzapu/WiFiManager
- Installation: Auto-managed by PlatformIO

**ArduinoJson v7.2.1+**
- Purpose: JSON serialization/deserialization
- Features:
  - Efficient memory usage
  - Static and dynamic documents
  - WebSocket message protocol
  - Configuration storage
- Repository: https://github.com/bblanchon/ArduinoJson
- Installation: Auto-managed by PlatformIO

**ElegantOTA v3.1.0+**
- Purpose: Over-the-air firmware updates
- Features:
  - Web-based update interface
  - AsyncWebServer support
  - HTTP Basic Authentication
  - Progress indication
- Repository: https://github.com/ayushsharma82/ElegantOTA
- Installation: Auto-managed by PlatformIO
- Flag: `-DELEGANTOTA_USE_ASYNC_WEBSERVER=1`

---

**ESP32 Arduino Core Libraries:**
- `ledcSetup()` - Configure PWM channel
- `ledcAttachPin()` - Attach PWM to GPIO
- `ledcWrite()` - Set PWM duty cycle
- `ledcWriteTone()` - Generate tone at frequency
- `i2s_*` functions - I2S DAC interface (PCM5102)
- `MDNS.begin()` - mDNS service registration

### Hardware Components

**Primary Components:**

1. **ESP32 Development Board**
   - Any variant (DevKit v1, WROOM-32, etc.)
   - Requirements: USB interface, exposed GPIO pins
   - Power: 5V via USB or 3.7V LiPo
   - Current draw: ~150mA typical (with all peripherals)

2. **VL53L0X Time-of-Flight Sensors (x2)**
   - Technology: Laser-based ToF ranging
   - Communication: I2C (400 kHz capable)
   - Default address: 0x29 (both sensors identical)
   - Range: 30-1000mm (optimal 50-500mm)
   - Accuracy: ±3% typical
   - Update rate: <30ms per reading (20ms with high-speed mode)
   - Power: 3.3V, ~20mA each
   - Breakout modules include:
     - 3.3V voltage regulator
     - I2C pull-up resistors (typically 10kΩ)
     - XSHUT pin for address configuration

3. **PCM5102 I2S DAC Module** ✅ **IMPLEMENTED**
   - Type: External I2S stereo DAC
   - Resolution: 16-bit (vs 8-bit built-in)
   - Sample rate: Up to 384 kHz (using 22050 Hz)
   - THD+N: < -93 dB (professional grade)
   - Dynamic range: 112 dB
   - Power: 3.3V, ~10mA
   - Interface: I2S (3 wires: BCK, WS, DATA)
   - Connections:
     - BCK → GPIO26 (Bit Clock)
     - WS → GPIO27 (Word Select / LRCK)
     - DATA → GPIO25 (Data Output)
     - VCC → 3.3V, GND → GND

4. **MCP23017 I2C GPIO Expander** ✅ **IMPLEMENTED**
   - Purpose: 16 additional GPIO pins for physical controls
   - Communication: I2C (address 0x20)
   - Features: Interrupt support, pull-up configuration
   - Power: 3.3V, ~1mA
   - Usage: 15 pins for oscillator switches (waveform + octave)
   - Connections: SDA/SCL shared with sensors

5. **SSD1306 OLED Display (128x64)** ✅ **IMPLEMENTED**
   - Technology: Monochrome OLED
   - Communication: I2C (address 0x3C)
   - Power: 3.3V, ~20mA
   - Features: Page-based display with navigation
   - Usage: Status pages, performance monitoring, tuner
   - Connections: SDA/SCL shared with sensors

**Supporting Components:**
- Breadboard (400+ tie points)
- Jumper wires (male-to-male)
- 100-220Ω resistor (buzzer protection)
- Optional: 2x LED + resistors for status indication

### Circuit Design

**Complete Wiring:**
```
ESP32 Pin      →  Component
─────────────────────────────
3.3V           →  VL53L0X #1 VCC
3.3V           →  VL53L0X #2 VCC
GND            →  VL53L0X #1 GND
GND            →  VL53L0X #2 GND
GPIO21 (SDA)   →  VL53L0X #1 SDA + VL53L0X #2 SDA (shared)
GPIO22 (SCL)   →  VL53L0X #1 SCL + VL53L0X #2 SCL (shared)
GPIO16         →  VL53L0X #1 XSHUT
GPIO17         →  VL53L0X #2 XSHUT
GPIO25         →  [100Ω] → Buzzer (+)
GND            →  Buzzer (-)
```

**Notes:**
- Keep I2C wires short (<20cm) for signal integrity
- Total power consumption: ~60-80mA (well within USB limits)
- No external pull-ups needed (VL53L0X breakouts include them)
- ESP32 GPIO outputs 3.3V (compatible with all components)

## Technical Constraints

### Hardware Limitations

**ESP32:**
- GPIO current limit: 12mA per pin, 40mA max total
- ADC not needed for this project
- DAC available (GPIO25/GPIO26) but not used in basic version
- WiFi/Bluetooth not used (reduces complexity and power)

**VL53L0X Sensors:**
- Maximum range: 1000mm (beyond this, readings unreliable)
- Minimum range: ~30mm (closer readings may be noisy)
- Ambient light affects accuracy at long distances
- Black or glossy surfaces may reduce accuracy
- Field of view: ~25° (requires clear line of sight)

**Passive Buzzer:**
- Audio quality: Low (square wave only)
- Volume: Fixed by buzzer design (duty cycle has limited effect)
- Frequency response: Not flat across range
- No polyphony (single tone at a time)

### Software Limitations

**I2C Bus:**
- Standard speed: 100 kHz (reliable)
- Fast mode: 400 kHz (may require shorter wires)
- Address space: 7-bit (0x08-0x77)
- Shared bus: Both sensors contend for bandwidth

**PWM Audio:**
- Resolution: 8-bit (256 levels)
- Single waveform: Square wave
- No envelope control
- No filtering (aliasing possible at low frequencies)

**Timing:**
- Arduino loop() not real-time (no RTOS guarantees)
- Sensor read blocking: ~20-30ms per measurement
- delay() blocks entire processor
- Serial output adds latency

### Development Constraints

**PlatformIO Configuration:**
- Must specify correct ESP32 board variant
- Library versions must be compatible
- Serial baud rate: 115200 recommended
- Upload speed: 921600 typical (can reduce if upload fails)

**Wokwi Limitations:**
- May not have exact VL53L0X model
- Simulation timing not perfectly accurate
- Limited component library
- Cannot fully replicate I2C timing issues

## Development Setup

### PlatformIO Configuration - Dual Environment

**platformio.ini:**
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps =
    adafruit/Adafruit_VL53L0X@^1.2.0
monitor_speed = 115200
upload_speed = 921600

; New simulation-specific environment
[env:esp32dev-wokwi]
platform = espressif32
board = esp32dev
framework = arduino
build_flags =
    -DWOKWI_SIMULATION
monitor_speed = 115200
upload_speed = 921600
; Note: VL53L0X library omitted - not needed for simulation
```

**Key Features:**
- Two environments from single configuration
- `-DWOKWI_SIMULATION` flag enables conditional compilation
- Simulation build omits VL53L0X library (faster compile)
- Same board target for both modes

### Wokwi Configuration - Implemented

**wokwi.toml:**
```toml
[wokwi]
version = 1

# PlatformIO builds firmware to these locations
firmware = '.pio/build/esp32dev-wokwi/firmware.bin'
elf = '.pio/build/esp32dev-wokwi/firmware.elf'

# Optional: Enable RFC2217 serial port forwarding
rfc2217ServerPort = 4000
```

**diagram.json:**
```json
{
  "version": 1,
  "author": "ESP32 Theremin Project",
  "editor": "wokwi",
  "parts": [
    {
      "type": "wokwi-esp32-devkit-v1",
      "id": "esp32",
      "top": 0,
      "left": 0,
      "attrs": {}
    },
    {
      "type": "wokwi-potentiometer",
      "id": "pitch_pot",
      "top": -100,
      "left": -150,
      "attrs": {
        "label": "Pitch (Distance)"
      }
    },
    {
      "type": "wokwi-potentiometer",
      "id": "volume_pot",
      "top": -100,
      "left": 100,
      "attrs": {
        "label": "Volume (Distance)"
      }
    },
    {
      "type": "wokwi-buzzer",
      "id": "buzzer",
      "top": 200,
      "left": 0,
      "attrs": {
        "volume": "0.5"
      }
    },
    {
      "type": "wokwi-resistor",
      "id": "r1",
      "top": 150,
      "left": 50,
      "attrs": {
        "value": "220"
      }
    }
  ],
  "connections": [
    ["pitch_pot:SIG", "esp32:GPIO34", "green", ["h0"]],
    ["pitch_pot:GND", "esp32:GND.1", "black", ["h0"]],
    ["pitch_pot:VCC", "esp32:3V3", "red", ["h0"]],

    ["volume_pot:SIG", "esp32:GPIO35", "blue", ["h0"]],
    ["volume_pot:GND", "esp32:GND.2", "black", ["h0"]],
    ["volume_pot:VCC", "esp32:3V3", "red", ["h0"]],

    ["esp32:GPIO25", "r1:1", "purple", ["h0"]],
    ["r1:2", "buzzer:1", "purple", ["h0"]],
    ["buzzer:2", "esp32:GND.3", "black", ["h0"]]
  ]
}
```

**Simulation Strategy:**
- Potentiometers replace VL53L0X sensors
- ADC input (0-4095) mapped to distance (mm)
- Identical audio synthesis logic to hardware
- GPIO34/35 for ADC (ADC1 channels, compatible with WiFi)
- GPIO25 for PWM buzzer output

### Serial Communication

**Baud Rate:** 115200
**Usage:**
- Initialization status messages
- Sensor reading debug output
- Error reporting
- Performance monitoring

**Debug Output Format (recommended):**
```
[INIT] Sensors initialized
[PITCH] Distance: 250mm, Freq: 850Hz
[VOLUME] Distance: 150mm, Duty: 128
[ERROR] Sensor timeout
```

## Tool Usage Patterns

### Development Workflow - Dual Mode

1. **Write Code in VSCode**
   - Edit src/main.cpp (single file for both modes)
   - PlatformIO IntelliSense for autocomplete
   - Use `#ifdef WOKWI_SIMULATION` for mode-specific code

2. **Build for Simulation**
   - Command: `/Users/fquagliati/.platformio/penv/bin/pio run -e esp32dev-wokwi`
   - Builds with `-DWOKWI_SIMULATION` flag
   - Output: `.pio/build/esp32dev-wokwi/firmware.bin`

3. **Build for Hardware**
   - Command: `/Users/fquagliati/.platformio/penv/bin/pio run -e esp32dev`
   - Builds without simulation flag
   - Output: `.pio/build/esp32dev/firmware.bin`

4. **Start Wokwi Simulation**
   - Press F1 (or Cmd+Shift+P)
   - Type "Wokwi: Start Simulator"
   - Interact with potentiometers to control pitch/volume
   - Monitor serial output

5. **Upload to Real Hardware** (when ready)
   - Command: `/Users/fquagliati/.platformio/penv/bin/pio run -e esp32dev -t upload`
   - Auto-detects COM port
   - No code changes needed

6. **Monitor Serial Output**
   - Command: `/Users/fquagliati/.platformio/penv/bin/pio device monitor`
   - Baud rate: 115200
   - Shows distance → frequency/volume mapping

### Common Commands

```bash
# Build simulation firmware
/Users/fquagliati/.platformio/penv/bin/pio run -e esp32dev-wokwi

# Build hardware firmware
/Users/fquagliati/.platformio/penv/bin/pio run -e esp32dev

# Upload to hardware
/Users/fquagliati/.platformio/penv/bin/pio run -e esp32dev -t upload

# Clean build artifacts
/Users/fquagliati/.platformio/penv/bin/pio run -t clean

# Monitor serial output
/Users/fquagliati/.platformio/penv/bin/pio device monitor
```

## Dependencies

### Required Software - ESP32

**Core Development:**
- Visual Studio Code (latest)
- PlatformIO IDE extension
- USB-to-Serial drivers (CP210x or CH340 depending on ESP32 board)

**Required Libraries:**
- Adafruit_VL53L0X >= 1.2.0
- Adafruit_SSD1306 >= 2.5.7
- Adafruit_GFX >= 1.11.9
- Adafruit_MCP23017 >= 2.3.2
- ESPAsyncWebServer >= 3.8.1
- WiFiManager >= 2.0.17
- ArduinoJson >= 7.2.1
- ElegantOTA >= 3.1.0
- Wire.h (included in Arduino core)

**Optional Tools:**
- Wokwi VSCode extension + active license
- Serial terminal (Arduino Serial Monitor, PuTTY, screen)
- Logic analyzer (for I2C debugging)

---

### Required Software - Web UI Frontend ✅ **IMPLEMENTED**

**Runtime:**
- Node.js >= 18.0.0 (LTS recommended)
- npm >= 9.0.0 (comes with Node.js)

**Build Tools:**
- Vite ^6.0.3 (Fast build system with hot-reload)
- PostCSS + Autoprefixer (CSS processing)

**Frontend Framework:**
- Preact ^10.26.2 (Lightweight React alternative, 3KB)
- Preact Hooks (State management)

**Styling:**
- Tailwind CSS ^3.4.17 (Utility-first CSS framework)
- Custom design system (defined in tailwind.config.js)

**Development Tools:**
- ESLint (Code linting, if configured)
- Hot Module Replacement (HMR via Vite)

**Frontend Dependencies:**
```json
{
  "dependencies": {
    "preact": "^10.26.2"
  },
  "devDependencies": {
    "@preact/preset-vite": "^2.9.3",
    "autoprefixer": "^10.4.20",
    "postcss": "^8.4.49",
    "tailwindcss": "^3.4.17",
    "vite": "^6.0.3"
  }
}
```

**Build Output:**
- Production bundle: ~30KB (uncompressed), ~10KB (gzipped)
- Output directory: `web_ui_src/dist/`
- Served by ESP32 via LittleFS filesystem

**Development Workflow:**
1. `cd web_ui_src/`
2. `npm install` - Install dependencies
3. `npm run dev` - Start dev server with hot-reload (port 5173)
4. `npm run build` - Build production bundle
5. Upload `dist/` to ESP32 via PlatformIO filesystem upload

**Environment Variables:**
- `.env.development` - Dev mode config (ESP32 IP override)
- `.env.development.example` - Template for local setup

## Upgrade Paths & Status

### Audio Quality Enhancement ✅ **COMPLETE**
**Previous:** PWM + passive buzzer
**Current:** PCM5102 I2S DAC (16-bit, professional grade)
- ✅ External I2S DAC (PCM5102)
- ✅ Sine wave generation via lookup table (1024 samples)
- ✅ Multiple waveforms (sine, square, triangle, sawtooth)
- ✅ 3 oscillators with independent control
- ✅ Audio effects chain (Delay, Chorus, Reverb)
- ✅ True volume control per oscillator
- **Result:** 256x resolution improvement, THD+N < -93 dB

### Processing Enhancement ✅ **COMPLETE**
**Previous:** Simple loop() with blocking reads
**Current:** FreeRTOS audio task (Core 1, high priority)
- ✅ Dedicated audio task on Core 1
- ✅ Non-blocking sensor reads
- ✅ Real-time audio deadline tracking (11ms buffer)
- ✅ Performance monitoring (CPU < 15% with all effects)
- **Result:** 85% CPU headroom, stable operation

### Display & Controls ✅ **COMPLETE**
**Previous:** None
**Current:** Full UI system
- ✅ SSD1306 OLED display (128x64, I2C)
- ✅ Page-based navigation system
- ✅ Notification overlays
- ✅ MCP23017 GPIO expander (15 physical switches)
- ✅ Serial command interface
- ✅ Web control interface (Preact + WebSocket)
- **Result:** Multi-modal control (physical + serial + web)

### Network Features ✅ **COMPLETE**
**Previous:** None
**Current:** Full web infrastructure
- ✅ WiFi with captive portal (WiFiManager)
- ✅ mDNS (theremin.local)
- ✅ WebSocket server (10 Hz updates)
- ✅ Modern web UI (Preact + Tailwind)
- ✅ OTA firmware updates (ElegantOTA)
- ✅ Multi-client support
- **Result:** Complete web control interface

---

### Future Enhancement Ideas

**Sensor Enhancement:**
- VL53L1X (4m range) or VL53L3X (5m range)
- Longer detection range
- Better ambient immunity
- Similar I2C interface
- **Priority:** Low (current sensors work well)

**Audio Enhancements:**
- ADSR envelope generator
- Additional waveforms (noise, custom)
- True stereo effects (currently mono duplicated)
- Higher sample rates (up to 384 kHz supported by PCM5102)
- **Priority:** Low (current audio quality excellent)

**UI Enhancements:**
- LED VU meters (analog or WS2812 addressable)
- Additional OLED pages (oscillators, sensors, effects)
- Web UI advanced features (presets, recording)
- Touch screen display
- **Priority:** Medium (nice-to-have improvements)

**Performance:**
- Higher sample rate (44.1 kHz or 48 kHz)
- More oscillators (4+)
- Additional effects (phaser, flanger, distortion)
- **Priority:** Low (current performance excellent)
