/*
 * GPIOMonitor.h
 *
 * Utility class for monitoring MCP23017 GPIO pin changes.
 * Useful for debugging wiring when controls are connected
 * but pin assignments are unknown.
 *
 * Usage:
 * 1. Enable ENABLE_GPIO_MONITOR=1 in platformio.ini
 * 2. Upload and open Serial Monitor
 * 3. Wiggle each control one at a time
 * 4. Note which pin numbers change
 * 5. Document mappings in PinConfig.h
 * 6. Disable monitor when done (ENABLE_GPIO_MONITOR=0)
 */

#pragma once
#include <Arduino.h>
#include <Adafruit_MCP23X17.h>

class GPIOMonitor {
public:
  /**
   * Constructor
   * @param i2cAddress I2C address of MCP23017 (typically 0x20)
   */
  GPIOMonitor(uint8_t i2cAddress = 0x20);

  /**
   * Initialize MCP23017 and configure all pins as inputs
   * Call this after Wire.begin()
   * @return true if successful, false if MCP23017 not found
   */
  bool begin();

  /**
   * Poll all GPIO pins and print changes to Serial
   * Call this frequently from main loop (non-blocking)
   */
  void update();

  /**
   * Print current state of all 16 pins
   * Useful for initial debugging
   */
  void printCurrentState();

private:
  Adafruit_MCP23X17 mcp;
  uint8_t i2cAddress;
  uint16_t previousState;  // 16-bit state (GPA0-7 = bits 0-7, GPB0-7 = bits 8-15)

  /**
   * Read all 16 pins into a single 16-bit value
   * @return 16-bit state (bit 0 = GPA0, bit 15 = GPB7)
   */
  uint16_t readAllPins();

  /**
   * Get pin name string (e.g., "GPA0", "GPB7")
   * @param pinNum Pin number (0-15)
   * @return Pointer to static string
   */
  const char* getPinName(uint8_t pinNum);
};
