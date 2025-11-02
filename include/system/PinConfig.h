/*
 * PinConfig.h
 *
 * Central pin configuration for ESP32 Theremin.
 * All GPIO and I2C address assignments defined here.
 *
 * Usage: Include this header in any file that needs pin definitions.
 *        #include "system/PinConfig.h"
 */

#pragma once

//=============================================================================
// SENSOR PINS - VL53L0X Time-of-Flight Sensors
//=============================================================================
#define PIN_SENSOR_I2C_SDA        21  // I2C data line (shared bus)
#define PIN_SENSOR_I2C_SCL        22  // I2C clock line (shared bus)
#define PIN_SENSOR_PITCH_XSHUT    16  // Pitch sensor shutdown control
#define PIN_SENSOR_VOLUME_XSHUT   19  // Volume sensor shutdown control

// I2C Addresses (set via XSHUT initialization)
#define I2C_ADDR_SENSOR_PITCH     0x30  // Pitch sensor (reassigned)
#define I2C_ADDR_SENSOR_VOLUME    0x29  // Volume sensor (default)

//=============================================================================
// AUDIO PINS - I2S DAC Output
//=============================================================================
#define PIN_AUDIO_OUTPUT          25  // DAC1 output via I2S (built-in DAC mode)

// Future v2.0 pins (uncomment when implementing Phase 2)
// #define PIN_AUDIO_AMP_ENABLE      26  // PAM8403 amplifier enable/disable
// #define PIN_AUDIO_LINE_OUT        25  // 6.35mm jack (same as DAC output)

//=============================================================================
// OTA PINS - Over-The-Air Update Control
//=============================================================================
#define PIN_OTA_ENABLE            -1  // -1 = always enable OTA
                                      // Set to GPIO pin number (e.g., 2, 4, 15)
                                      // to require button press during boot

//=============================================================================
// DISPLAY PINS - Phase 2 (v2.0 Feature)
//=============================================================================
// #define PIN_DISPLAY_I2C_ADDR      0x3C  // SSD1306 OLED (shares I2C bus)
// Note: Display uses same SDA/SCL as sensors (GPIO 21/22)

//=============================================================================
// OSCILLATOR CONTROL PINS - Phase 3 (v2.0 Feature)
//=============================================================================
// MCP23017 I2C GPIO Expander
#define PIN_SWITCH_EXPANDER_ADDR  0x20  // MCP23017 I2C address

// Waveform Rotary Switches (4-position, 2 GPIO each = 6 total)
// Uses MCP23017 pins 0-5:
#define PIN_OSC1_WAVE_A 6
#define PIN_OSC1_WAVE_B 5
#define PIN_OSC1_WAVE_C 14

#define PIN_OSC2_WAVE_A 4
#define PIN_OSC2_WAVE_B 11
#define PIN_OSC2_WAVE_C 3

#define PIN_OSC3_WAVE_A 1
#define PIN_OSC3_WAVE_B 9
#define PIN_OS3_WAVE_C 0

// Octave Toggle Switches (3-position, 2 GPIO each = 6 total)
// Uses MCP23017 pins 6-11:
#define PIN_OSC1_OCT_UP 15
#define PIN_OSC1_OCT_DOWN 7

#define PIN_OSC2_OCT_UP 13
#define PIN_OSC2_OCT_DOWN 12

#define PIN_OSC3_OCT_UP 2
#define PIN_OSC3_OCT_DOWN 10

//=============================================================================
// EFFECTS CONTROL PINS - Phase 4 (v2.0 Feature)
//=============================================================================
// Uses MCP23017 pins 12-14:
// #define PIN_EFFECT_DELAY_EN       12  // Delay effect enable switch
// #define PIN_EFFECT_CHORUS_EN      13  // Chorus effect enable switch
// #define PIN_EFFECT_REVERB_EN      14  // Reverb effect enable switch (optional)

//=============================================================================
// LED METER PINS - Phase 4 (v2.0 Feature)
//=============================================================================
// #define PIN_LED_PITCH             4   // WS2812B data line for pitch meter
// #define PIN_LED_VOLUME            5   // WS2812B data line for volume meter
// #define LED_COUNT_PITCH           8   // Number of LEDs in pitch strip
// #define LED_COUNT_VOLUME          8   // Number of LEDs in volume strip

//=============================================================================
// PIN USAGE SUMMARY
//=============================================================================
/*
 * Current GPIO Allocation (Phase 1):
 * ----------------------------------
 *   16  - Sensor XSHUT (pitch)
 *   19  - Sensor XSHUT (volume)
 *   21  - I2C SDA (shared bus)
 *   22  - I2C SCL (shared bus)
 *   25  - Audio DAC output (I2S built-in DAC mode)
 *
 * Future GPIO Allocation (Phase 2+):
 * -----------------------------------
 *   4   - LED pitch meter (WS2812B)
 *   5   - LED volume meter (WS2812B)
 *   26  - Amplifier enable/disable
 *   TBD - OTA enable button (when chosen)
 *
 * I2C Device Addresses:
 * ---------------------
 *   0x29 - VL53L0X sensor (volume)
 *   0x30 - VL53L0X sensor (pitch)
 *   0x20 - MCP23017 GPIO expander (future)
 *   0x3C - SSD1306 OLED display (future)
 *
 * ESP32 Pin Notes:
 * ----------------
 *   AVOID: GPIO 0, 2, 12, 15 (boot strapping pins - can cause boot issues)
 *   INPUT-ONLY: GPIO 34-39 (no internal pullup, ADC only)
 *   SAFE: GPIO 4, 5, 13, 14, 16-27, 32, 33 (general purpose)
 *   RESERVED: GPIO 1, 3 (UART TX/RX for Serial)
 *   RESERVED: GPIO 6-11 (SPI flash - DO NOT USE)
 *
 * MCP23017 GPIO Expander Allocation (Phase 3+):
 * ----------------------------------------------
 *   See definitions above for waveforms and octave switch, only 1 free pin left
 *   at the moment.
 */

//=============================================================================
// HARDWARE REVISION TRACKING
//=============================================================================
#define HARDWARE_VERSION_MAJOR    1
#define HARDWARE_VERSION_MINOR    0

// Hardware change log:
// v1.0 (October 2025) - Initial Phase 1 implementation
//   - 2x VL53L0X sensors on I2C
//   - I2S DAC audio output on GPIO 25 (built-in DAC mode)
//   - OTA with optional button activation

// Planned v2.0 (Future)
//   - DAC audio output with PAM8403 amplifier
//   - SSD1306 OLED display on I2C
//   - MCP23017 GPIO expander for control switches
//   - WS2812B LED strips for visual meters
//   - 3x oscillators with waveform/octave controls
//   - Effects chain (Delay, Chorus, optional Reverb)
