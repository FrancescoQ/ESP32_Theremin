/*
 * SensorManager.cpp
 *
 * Implementation of sensor management for ESP32 Theremin.
 * Handles both simulation (potentiometers) and hardware (VL53L0X) modes.
 */

#include "SensorManager.h"
#include "Debug.h"

// Constructor
SensorManager::SensorManager()
    : smoothedPitchDistance(0.0f),
      smoothedVolumeDistance(0.0f),
      firstReading(true) {
  // Exponential smoothing values initialized to 0, will be set on first reading
}

// Initialize sensors
bool SensorManager::begin() {
  // Initialize I2C and VL53L0X sensors
  Wire.begin(PIN_SENSOR_I2C_SDA, PIN_SENSOR_I2C_SCL);
  DEBUG_PRINTLN("[SENSOR] I2C initialized");

  // Configure XSHUT pins
  pinMode(PIN_SENSOR_PITCH_XSHUT, OUTPUT);
  pinMode(PIN_SENSOR_VOLUME_XSHUT, OUTPUT);

  // Disable both sensors initially
  digitalWrite(PIN_SENSOR_PITCH_XSHUT, LOW);
  digitalWrite(PIN_SENSOR_VOLUME_XSHUT, LOW);
  delay(10);

  // Initialize pitch sensor at custom address to avoid conflict as
  // both sensors default to same I2C address.
  digitalWrite(PIN_SENSOR_PITCH_XSHUT, HIGH);
  delay(10);
  if (!pitchSensor.begin(I2C_ADDR_SENSOR_PITCH)) {
    DEBUG_PRINTLN("[SENSOR] ERROR: Pitch sensor failed to initialize!");
    return false;
  }
  DEBUG_PRINTLN("[SENSOR] Pitch sensor initialized at 0x30");

  // Initialize volume sensor at default address 0x29
  digitalWrite(PIN_SENSOR_VOLUME_XSHUT, HIGH);
  delay(10);
  if (!volumeSensor.begin(I2C_ADDR_SENSOR_VOLUME)) {
    DEBUG_PRINTLN("[SENSOR] ERROR: Volume sensor failed to initialize!");
    return false;
  }
  DEBUG_PRINTLN("[SENSOR] Volume sensor initialized at 0x29");

  return true;
}

// Get smoothed pitch distance
int SensorManager::getPitchDistance() {
  int rawDistance = readPitchRaw();
  return applyExponentialSmoothing(smoothedPitchDistance, rawDistance, firstReading);
}

// Get smoothed volume distance
int SensorManager::getVolumeDistance() {
  int rawDistance = readVolumeRaw();
  int result = applyExponentialSmoothing(smoothedVolumeDistance, rawDistance, firstReading);

  // After first reading of both sensors, clear the flag
  if (firstReading) {
    firstReading = false;
  }

  return result;
}

// Apply exponential weighted moving average (EWMA) smoothing
int SensorManager::applyExponentialSmoothing(float& smoothedValue, int newReading, bool isFirstReading) {
  if (isFirstReading) {
    // On first reading, initialize the smoothed value
    smoothedValue = (float)newReading;
  } else {
    // Apply EWMA formula: smoothed = alpha * new + (1 - alpha) * previous
    smoothedValue = (SMOOTHING_ALPHA * newReading) + ((1.0f - SMOOTHING_ALPHA) * smoothedValue);
  }

  return (int)smoothedValue;
}

int SensorManager::readPitchRaw() {
  pitchSensor.rangingTest(&pitchMeasure, false);
  // Return measured distance, or max distance if out of range
  return (pitchMeasure.RangeStatus != 4) ? pitchMeasure.RangeMilliMeter : PITCH_MAX_DIST;
}

int SensorManager::readVolumeRaw() {
  volumeSensor.rangingTest(&volumeMeasure, false);
  // Return measured distance, or min distance (silent) if out of range
  return (volumeMeasure.RangeStatus != 4) ? volumeMeasure.RangeMilliMeter : VOLUME_MIN_DIST;
}
