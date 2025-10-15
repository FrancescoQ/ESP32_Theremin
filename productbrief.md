# Product Brief - ESP32 Homemade Theremin

## 1. Project Overview

**Project Name:** ESP32 Theremin  
**Type:** DIY Electronic Musical Instrument  
**Goal:** Create a simplified theremin using ESP32 and proximity sensors to control frequency and volume of an audio signal.

## 2. Concept

A homemade theremin that leverages ESP32 capabilities to detect hand distance via sensors and translate it into musical parameters (pitch and volume), producing sound through a piezoelectric buzzer or speaker.

## 3. Functional Requirements

### Core Features
- **Frequency Control:** One sensor detects hand distance and modulates sound frequency (pitch)
- **Volume Control:** A second sensor detects distance and controls sound intensity
- **Audio Generation:** Output through piezo buzzer or small speaker
- **Real-time Response:** Minimal latency between movement and sound change

### Optional Features (Future)
- Selection of different waveforms (sine, square, sawtooth)
- Preset saving
- LED visualization of active range
- Manual sensor calibration
- Audio output to external amplifier

## 4. Technical Specifications

### Main Hardware
- **Microcontroller:** ESP32 (any Dev Board variant)
- **Proximity Sensors:** 2x VL53L0X Time-of-Flight laser sensors
- **Audio Output:** Passive piezoelectric buzzer (alternative: DAC + mini speaker)
- **Power Supply:** USB (5V) or Li-Po battery 3.7V

### Sensor Choice

**Selected Sensor: VL53L0X (Time-of-Flight Laser)**

**Characteristics:**
- Technology: Laser ToF (Time of Flight)
- Range: 30-1000mm (optimal 50-500mm for theremin)
- Interface: I2C
- Precision: ±3% (excellent for musical application)
- Speed: Very fast continuous readings (<30ms)
- Dimensions: Very compact (breakout module ~15x10mm)

**Advantages for the Project:**
- No mutual interference between the two sensors
- Very stable and precise readings
- Perfect range for musical gestures
- Two sensors on the same I2C bus (configurable addresses)
- Well-supported Arduino libraries (Adafruit VL53L0X)

**I2C Notes:**
- Both VL53L0X sensors have the same default I2C address (0x29)
- Solution: use XSHUT pins to initialize sensors sequentially and assign different addresses
- Alternative: use an I2C multiplexer (TCA9548A) if expansion is desired in the future

### Circuit and Connections

**Complete Schematic:**
```
ESP32                VL53L0X #1 (Pitch)    VL53L0X #2 (Volume)
-----                -------------------    --------------------
3.3V  ─────────────── VCC                   VCC
GND   ─────────────── GND                   GND
GPIO21 (SDA) ───┬──── SDA                   SDA
GPIO22 (SCL) ───┤──── SCL                   SCL
                │
GPIO16 ──────────┼──── XSHUT
GPIO17 ──────────┴─────────────────────── XSHUT

GPIO25 (PWM) ───[100Ω]─── Piezo Buzzer (+)
GND ───────────────────── Piezo Buzzer (-)
```

**Circuit Components:**

*Essential:*
- 1x ESP32 Dev Board
- 2x VL53L0X breakout modules (with integrated regulator and pull-ups)
- 1x **PASSIVE** piezoelectric buzzer (not active!)
- 1x Breadboard 400+ points
- Male-to-male jumper wires

*Optional but recommended:*
- 1x 100-220Ω Resistor (buzzer protection)
- 2x 4.7kΩ Resistor (additional I2C pull-ups, only if modules don't have them)
- 2x LED + 2x 220Ω Resistor (visual feedback)

**Circuit Notes:**
- VL53L0X breakout modules already include 3.3V regulator and I2C pull-ups (typically 10kΩ)
- Total power consumption: ~60mA, easily manageable by ESP32 via USB
- Keep I2C cables short (<20cm) for stability
- Passive buzzer can be identified by visible components inside (oscillator)

**ESP32 GPIO Pins Used:**
- GPIO21: SDA (I2C Data)
- GPIO22: SCL (I2C Clock)
- GPIO16: XSHUT sensor #1
- GPIO17: XSHUT sensor #2
- GPIO25: PWM for buzzer (can be changed to any PWM pin)

### Output Audio

**Option A - Passive Piezo Buzzer:**
- Pro: Very simple, economical
- Con: Limited audio quality, fixed volume

**Option B - ESP32 Internal DAC + Speaker:**
- Pro: Better audio quality, real volume control
- Con: Requires amplification circuit, more complex

## 5. Estimated Operating Ranges

- **Frequency sensor distance:** 5-40 cm (useful musical range)
- **Frequency range:** 100-2000 Hz (approximately 4 octaves)
- **Volume sensor distance:** 5-30 cm
- **Volume range:** 0-100% (PWM duty cycle)

## 6. Software Architecture (High-Level)

### Development Environment
- **IDE:** Visual Studio Code
- **Framework:** Arduino Framework
- **Build System:** PlatformIO
- **Simulation:** Wokwi Simulator (VSCode plugin with active license)
- **Required Libraries:**
  - `Adafruit_VL53L0X` (ToF sensor management)
  - `Wire.h` (I2C communication, included in Arduino)

### PlatformIO Configuration
```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps = 
    adafruit/Adafruit VL53L0X@^1.2.0
monitor_speed = 115200
```

### Wokwi Simulation
Wokwi allows testing code and circuit virtually before assembling real hardware:
- Complete ESP32 simulation with sensors and components
- Interactive code debugging
- Circuit testing without risk of damaging components
- Rapid prototyping of functionalities

**Note:** Verify VL53L0X availability in Wokwi. If not available, use HC-SR04 ultrasonic sensors to test logic, then migrate to VL53L0X on real hardware.

### Main Program Flow

```
Setup:
├─ I2C Initialization (Wire.begin)
├─ Sensor #1 Initialization via XSHUT
│  ├─ XSHUT1 HIGH, XSHUT2 LOW
│  ├─ Configure sensor #1
│  └─ Change address to 0x30
├─ Sensor #2 Initialization via XSHUT
│  ├─ XSHUT2 HIGH
│  ├─ Configure sensor #2
│  └─ Keep address at 0x29
└─ PWM Setup for buzzer (ledcSetup)

Main Loop:
├─ Read Sensor #1 @ 0x30 (Pitch)
├─ Map distance → frequency (e.g., 100-2000 Hz)
├─ Read Sensor #2 @ 0x29 (Volume)
├─ Map distance → PWM duty cycle (0-255)
├─ Tone generation: ledcWriteTone(frequency)
├─ Volume control: ledcWrite(duty_cycle)
└─ Optional: value smoothing filter
```

## 7. Suggested Development Phases

### Phase 0 - Wokwi Simulation (Optional but Recommended)
- Create Wokwi project with ESP32
- Test basic logic with available sensors in Wokwi
- Validate distance → frequency/volume mapping
- Initial debugging without physical hardware
- **Note:** If VL53L0X not available in Wokwi, use HC-SR04 for logic

### Phase 1 - Environment Setup and Basic Testing
- Setup PlatformIO with ESP32 and Arduino framework
- Test single VL53L0X sensor on real hardware
- Test simple PWM tone generation on buzzer
- Verify I2C functionality

### Phase 2 - Dual Sensor Control
- Implement sequential XSHUT initialization
- Configure multiple I2C addresses (0x29 and 0x30)
- Simultaneous reading of both sensors
- Debug I2C communication

### Phase 3 - Mapping and Audio
- Map sensor #1 distance → frequency (pitch)
- Map sensor #2 distance → volume (duty cycle)
- Real-time tone control integration
- Calibrate operating ranges

### Phase 4 - Refinement
- Reading smoothing (moving average or low-pass filter)
- Fine-tune response curves
- Handle edge cases (out of range)
- Latency optimization

### Phase 5 - Enhancement (Optional)
- Visual LED feedback
- Preset saving in EEPROM/SPIFFS
- Waveform selection (if switching to DAC)
- Dynamic calibration via serial
- Case and physical ergonomics

## 8. Estimated Materials

### Base Components
- 1x ESP32 Dev Board (~€5-8)
- 2x VL53L0X ToF sensor breakout modules (~€3-6 each)
- 1x **Passive** piezo buzzer (~€1)
- 1x Breadboard 400+ points (~€3)
- Male-to-male jumper wires (~€2)
- 1x 100-220Ω Resistor for buzzer (~€0.10)
- **Estimated Total:** €17-30

**Optional Components:**
- 2x I2C pull-up resistors 4.7kΩ (only if not integrated in VL53L0X modules)
- 2x LED + 2x 220Ω Resistors (sensor status indicators)
- 100µF Capacitor (stabilization, rarely necessary)

### For Advanced Version
- 8Ω Mini speaker
- Amplifier (PAM8403 or similar)
- Potentiometer for calibration
- 3D printed or handmade case

## 9. Risks and Challenges

- **I2C Addressing:** Both VL53L0X have the same default address, sequential initialization with XSHUT pins required
- **Reading Jitter:** Possible need for software filters to stabilize (moving average or low-pass filter)
- **Limited Buzzer Range:** Audio quality not comparable to real theremin
- **Latency:** Must be minimized for fluid musical experience
- **I2C Consumption:** Watch bus speed for fast simultaneous readings

## 10. Success Criteria

The project is considered successful if:
- ✓ Both sensors correctly detect distance
- ✓ Pitch changes continuously and predictably
- ✓ Volume is independently controllable
- ✓ Latency is imperceptible (<50ms)
- ✓ System is stable and reproducible

## 11. Next Steps

1. **Development Environment Setup**
   - Install VSCode + PlatformIO extension
   - Verify Wokwi plugin and active license
   - Create new ESP32 project with Arduino framework
   - Configure platformio.ini with necessary libraries

2. **Virtual Prototyping (Wokwi)**
   - Create diagram.json and wokwi.toml for simulation
   - Test basic logic with simulated sensors
   - Validate mapping and virtual audio control
   - Initial debugging without physical components

3. **Order/Acquire Physical Components**
   - 1x ESP32 Dev Board
   - 2x VL53L0X breakout modules
   - 1x Passive piezo buzzer
   - Breadboard and jumper wires
   - 100-220Ω Resistor

4. **Real Component Testing**
   - Upload blink test to ESP32
   - Test single VL53L0X with Adafruit examples
   - Test PWM buzzer with simple tones
   - Verify I2C bus

5. **Physical Prototype Implementation**
   - Migrate code from Wokwi to hardware
   - Dual sensor initialization with XSHUT
   - Real-time reading + audio integration
   - Iteration and calibration

6. **Refinement and Case**
   - Smoothing and optimizations
   - Build physical support for sensors
   - Musical testing and fine-tuning

---

**Note:** This is an experimental educational project. A professional-quality theremin would require LC oscillator circuits and capacitive antennas, but this digital version is perfect for learning and having fun with ESP32, I2C, and audio synthesis!