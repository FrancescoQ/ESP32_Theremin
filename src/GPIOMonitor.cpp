/*
 * GPIOMonitor.cpp
 *
 * Implementation of GPIO monitoring utility for MCP23017
 */

#include "GPIOMonitor.h"

GPIOMonitor::GPIOMonitor(uint8_t i2cAddress)
    : i2cAddress(i2cAddress), previousState(0) {
}

bool GPIOMonitor::begin() {
  Serial.println("[GPIO] Initializing MCP23017 monitor...");

  // Initialize MCP23017
  if (!mcp.begin_I2C(i2cAddress)) {
    Serial.print("[GPIO] ERROR: MCP23017 not found at address 0x");
    Serial.println(i2cAddress, HEX);
    return false;
  }

  Serial.print("[GPIO] MCP23017 found at address 0x");
  Serial.println(i2cAddress, HEX);

  // Configure all 16 pins as inputs with pullups
  for (uint8_t pin = 0; pin < 16; pin++) {
    mcp.pinMode(pin, INPUT_PULLUP);
  }

  // Read initial state
  previousState = readAllPins();

  Serial.println("[GPIO] All pins configured as INPUT_PULLUP");
  Serial.println("[GPIO] Monitor ready - wiggle controls to see changes!");
  Serial.println("");

  return true;
}

void GPIOMonitor::update() {
  // Read current state of all pins
  uint16_t currentState = readAllPins();

  // Check for changes
  if (currentState != previousState) {
    // Find which pins changed
    uint16_t changedPins = currentState ^ previousState;

    for (uint8_t pin = 0; pin < 16; pin++) {
      if (changedPins & (1 << pin)) {
        // This pin changed
        bool oldValue = (previousState >> pin) & 1;
        bool newValue = (currentState >> pin) & 1;

        Serial.print("[GPIO] Pin ");
        Serial.print(pin);
        Serial.print(" (");
        Serial.print(getPinName(pin));
        Serial.print("): ");
        Serial.print(oldValue ? "HIGH" : "LOW");
        Serial.print(" -> ");
        Serial.println(newValue ? "HIGH" : "LOW");
      }
    }

    previousState = currentState;
  }
}

void GPIOMonitor::printCurrentState() {
  uint16_t state = readAllPins();

  Serial.println("[GPIO] Current pin states:");
  Serial.println("  Port A (pins 0-7):");
  for (uint8_t pin = 0; pin < 8; pin++) {
    bool value = (state >> pin) & 1;
    Serial.print("    ");
    Serial.print(getPinName(pin));
    Serial.print(": ");
    Serial.println(value ? "HIGH" : "LOW");
  }

  Serial.println("  Port B (pins 8-15):");
  for (uint8_t pin = 8; pin < 16; pin++) {
    bool value = (state >> pin) & 1;
    Serial.print("    ");
    Serial.print(getPinName(pin));
    Serial.print(": ");
    Serial.println(value ? "HIGH" : "LOW");
  }
  Serial.println("");
}

uint16_t GPIOMonitor::readAllPins() {
  // Read both ports (A and B) as a single 16-bit value
  uint16_t portA = mcp.readGPIOAB() & 0xFF;        // Lower 8 bits (GPA0-7)
  uint16_t portB = (mcp.readGPIOAB() >> 8) & 0xFF; // Upper 8 bits (GPB0-7)
  return portA | (portB << 8);
}

const char* GPIOMonitor::getPinName(uint8_t pinNum) {
  static char name[8];

  if (pinNum < 8) {
    // Port A
    snprintf(name, sizeof(name), "GPA%d", pinNum);
  } else {
    // Port B
    snprintf(name, sizeof(name), "GPB%d", pinNum - 8);
  }

  return name;
}
