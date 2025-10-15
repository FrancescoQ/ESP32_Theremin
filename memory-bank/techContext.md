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

**ESP32 Arduino Core Libraries**
- `ledcSetup()` - Configure PWM channel
- `ledcAttachPin()` - Attach PWM to GPIO
- `ledcWrite()` - Set PWM duty cycle
- `ledcWriteTone()` - Generate tone at frequency

### Hardware Components

**Primary Components:**

1. **ESP32 Development Board**
   - Any variant (DevKit v1, WROOM-32, etc.)
   - Requirements: USB interface, exposed GPIO pins
   - Power: 5V via USB or 3.7V LiPo
   - Current draw: ~80mA typical + sensors

2. **VL53L0X Time-of-Flight Sensors (x2)**
   - Technology: Laser-based ToF ranging
   - Communication: I2C (400 kHz capable)
   - Default address: 0x29 (both sensors identical)
   - Range: 30-1000mm (optimal 50-500mm)
   - Accuracy: ±3% typical
   - Update rate: <30ms per reading
   - Power: 3.3V, ~20mA each
   - Breakout modules include:
     - 3.3V voltage regulator
     - I2C pull-up resistors (typically 10kΩ)
     - XSHUT pin for address configuration

3. **Passive Piezoelectric Buzzer**
   - Type: PASSIVE (requires external oscillation)
   - Frequency range: 100-10,000 Hz typical
   - Drive: PWM square wave
   - Current: ~10-30mA
   - Polarity: Usually marked (+/-)
   - Protection: 100-220Ω series resistor recommended

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

### Required Software
- Visual Studio Code (latest)
- PlatformIO IDE extension
- USB-to-Serial drivers (CP210x or CH340 depending on ESP32 board)

### Required Libraries
- Adafruit_VL53L0X >= 1.2.0
- Adafruit_BusIO (auto-installed as dependency)
- Wire.h (included in Arduino core)

### Optional Tools
- Wokwi VSCode extension + active license
- Serial terminal (Arduino Serial Monitor, PuTTY, screen)
- Logic analyzer (for I2C debugging)

## Upgrade Paths

### Audio Quality Enhancement
**Current:** PWM + passive buzzer
**Upgrade:** ESP32 DAC + amplifier + speaker
- Use GPIO25 (DAC1) or GPIO26 (DAC2)
- Add PAM8403 amplifier module
- Implement sine wave generation via lookup table
- True volume control possible

### Sensor Enhancement
**Current:** VL53L0X (1m range)
**Upgrade:** VL53L1X (4m range) or VL53L3X (5m range)
- Longer detection range
- Better ambient immunity
- Similar I2C interface

### Processing Enhancement
**Current:** Simple loop() with blocking reads
**Upgrade:** FreeRTOS tasks
- Separate sensor reading task
- Audio generation task
- Non-blocking operation
- Better timing control
