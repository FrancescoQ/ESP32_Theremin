/*
 * ESP32 Theremin Project
 *
 * A digital theremin using ESP32, dual VL53L0X Time-of-Flight sensors,
 * and PWM audio output to a passive piezoelectric buzzer.
 *
 * Hardware:
 * - ESP32 Dev Board
 * - 2x VL53L0X ToF Sensors (I2C)
 * - 1x Passive Piezoelectric Buzzer
 *
 * Connections:
 * - VL53L0X #1 (Pitch): SDA=GPIO21, SCL=GPIO22, XSHUT=GPIO16, Addr=0x30
 * - VL53L0X #2 (Volume): SDA=GPIO21, SCL=GPIO22, XSHUT=GPIO17, Addr=0x29
 * - Buzzer: GPIO25 with 100-220Ω resistor
 */

#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_VL53L0X.h"

// Pin Definitions
#define XSHUT_PIN_1 16      // XSHUT for pitch sensor (will be 0x30)
#define XSHUT_PIN_2 17      // XSHUT for volume sensor (will be 0x29)
#define BUZZER_PIN 25       // PWM output to buzzer
#define SDA_PIN 21          // I2C Data
#define SCL_PIN 22          // I2C Clock

// I2C Addresses
#define SENSOR_ADDR_1 0x30  // Pitch sensor (after reassignment)
#define SENSOR_ADDR_2 0x29  // Volume sensor (default address)

// Audio Configuration
#define PWM_CHANNEL 0       // PWM channel for buzzer
#define PWM_RESOLUTION 8    // 8-bit resolution (0-255)
#define PWM_FREQUENCY 2000  // Base PWM frequency (will be changed by ledcWriteTone)

// Sensor Objects
Adafruit_VL53L0X pitchSensor = Adafruit_VL53L0X();
Adafruit_VL53L0X volumeSensor = Adafruit_VL53L0X();

// Measurement Variables
VL53L0X_RangingMeasurementData_t pitchMeasure;
VL53L0X_RangingMeasurementData_t volumeMeasure;

void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);
  delay(100);
  Serial.println("\n\n=== ESP32 Theremin Initialization ===");

  // Initialize I2C
  Wire.begin(SDA_PIN, SCL_PIN);
  Serial.println("[INIT] I2C initialized");

  // Initialize XSHUT pins
  pinMode(XSHUT_PIN_1, OUTPUT);
  pinMode(XSHUT_PIN_2, OUTPUT);

  // Hold both sensors in shutdown
  digitalWrite(XSHUT_PIN_1, LOW);
  digitalWrite(XSHUT_PIN_2, LOW);
  delay(10);
  Serial.println("[INIT] Both sensors in shutdown");

  // Initialize Pitch Sensor (Sensor 1) with address 0x30
  digitalWrite(XSHUT_PIN_1, HIGH);
  delay(10);

  if (!pitchSensor.begin(SENSOR_ADDR_1)) {
    Serial.println("[ERROR] Failed to initialize pitch sensor!");
    while(1);
  }
  Serial.println("[INIT] Pitch sensor initialized at 0x30");

  // Initialize Volume Sensor (Sensor 2) at default address 0x29
  digitalWrite(XSHUT_PIN_2, HIGH);
  delay(10);

  if (!volumeSensor.begin(SENSOR_ADDR_2)) {
    Serial.println("[ERROR] Failed to initialize volume sensor!");
    while(1);
  }
  Serial.println("[INIT] Volume sensor initialized at 0x29");

  // Configure PWM for buzzer
  ledcSetup(PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(BUZZER_PIN, PWM_CHANNEL);
  Serial.println("[INIT] PWM configured for buzzer");

  Serial.println("=== Initialization Complete ===\n");
  Serial.println("Move your hands over the sensors to play!");
  Serial.println("Left sensor = Pitch, Right sensor = Volume\n");
}

void loop() {
  // Read pitch sensor
  pitchSensor.rangingTest(&pitchMeasure, false);

  // Read volume sensor
  volumeSensor.rangingTest(&volumeMeasure, false);

  // Debug output
  if (pitchMeasure.RangeStatus != 4) {  // 4 = out of range
    Serial.print("[PITCH] Distance: ");
    Serial.print(pitchMeasure.RangeMilliMeter);
    Serial.print("mm");
  } else {
    Serial.print("[PITCH] Out of range");
  }

  Serial.print("  |  ");

  if (volumeMeasure.RangeStatus != 4) {
    Serial.print("[VOLUME] Distance: ");
    Serial.print(volumeMeasure.RangeMilliMeter);
    Serial.println("mm");
  } else {
    Serial.println("[VOLUME] Out of range");
  }

  // TODO: Map sensor readings to frequency and volume
  // TODO: Generate audio output

  delay(100);  // Temporary delay for readable serial output
}
