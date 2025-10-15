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

**Framework: Arduino**
- Familiar, well-documented API
- Extensive library ecosystem
- Simplifies ESP32 peripheral access
- Good balance of ease-of-use and control

**Simulation: Wokwi Simulator**
- VSCode plugin with active license required
- Virtual circuit testing before hardware assembly
- Interactive debugging
- Component library includes ESP32
- Limitations: May not have VL53L0X (use HC-SR04 for logic testing)

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

### Initial PlatformIO Configuration

**platformio.ini:**
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps =
    adafruit/Adafruit VL53L0X@^1.2.0
monitor_speed = 115200
upload_speed = 921600
```

**Alternative boards:**
- `esp32doit-devkit-v1`
- `esp32-wroom-32`
- `lolin32`
(Choose based on actual hardware)

### Wokwi Configuration

**diagram.json (example):**
```json
{
  "version": 1,
  "author": "Theremin Project",
  "editor": "wokwi",
  "parts": [
    { "type": "wokwi-esp32-devkit-v1", "id": "esp32", "top": 0, "left": 0 },
    { "type": "wokwi-hc-sr04", "id": "sensor1", "top": 100, "left": -100 },
    { "type": "wokwi-hc-sr04", "id": "sensor2", "top": 100, "left": 100 },
    { "type": "wokwi-buzzer", "id": "bz1", "top": 200, "left": 0 }
  ],
  "connections": [
    // ... connections defined here
  ]
}
```

**Note:** If VL53L0X not available, use HC-SR04 for simulation and migrate code to VL53L0X on real hardware.

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

### Development Workflow

1. **Write Code in VSCode**
   - Edit src/main.cpp
   - PlatformIO IntelliSense for autocomplete

2. **Build Project**
   - Command: `pio run`
   - Or: Click PlatformIO build button
   - Checks compilation errors

3. **Upload to ESP32**
   - Command: `pio run --target upload`
   - Or: Click PlatformIO upload button
   - Auto-detects COM port

4. **Monitor Serial Output**
   - Command: `pio device monitor`
   - Or: Click PlatformIO monitor button
   - Exit: Ctrl+C

5. **Wokwi Simulation**
   - Open diagram.json
   - Press F1 → "Wokwi: Start Simulation"
   - Interact with virtual components

### Common Commands

```bash
# Initialize new PlatformIO project
pio project init --board esp32dev

# Install library
pio lib install "Adafruit VL53L0X"

# Build and upload
pio run -t upload

# Clean build
pio run -t clean

# Update platforms and libraries
pio update
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
