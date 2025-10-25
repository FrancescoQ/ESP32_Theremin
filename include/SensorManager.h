/*
 * SensorManager.h
 *
 * Manages distance sensor input for the ESP32 Theremin.
 * Uses VL53L0X Time-of-Flight sensors over I2C.
 * Provides smoothed distance readings for pitch and volume control.
 */

#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_VL53L0X.h"
#include "PinConfig.h"

class SensorManager {
 public:
  /**
   * Constructor
   */
  SensorManager();

  /**
   * Initialize sensors (must be called in setup())
   * Returns true if successful, false if sensor initialization fails
   */
  bool begin();

  /**
   * Read and return smoothed pitch distance in millimeters
   * Range: PITCH_MIN_DIST to PITCH_MAX_DIST (typically 50-400mm)
   */
  int getPitchDistance();

  /**
   * Read and return smoothed volume distance in millimeters
   * Range: VOLUME_MIN_DIST to VOLUME_MAX_DIST (typically 50-300mm)
   */
  int getVolumeDistance();

  // Distance ranges (same for both modes)
  static const int PITCH_MIN_DIST = 50;
  static const int PITCH_MAX_DIST = 400;
  static const int VOLUME_MIN_DIST = 50;
  static const int VOLUME_MAX_DIST = 300;

 private:
  // Smoothing filter parameters
  static const int SAMPLES = 5;
  int pitchReadings[SAMPLES];
  int volumeReadings[SAMPLES];
  int pitchIndex;
  int volumeIndex;

  /**
   * Apply moving average smoothing to sensor readings using a circular buffer.
   * Stores the new reading in the buffer, advances the buffer position (wrapping at SAMPLES),
   * then calculates and returns the average of all readings in the buffer.
   * This reduces sensor noise by averaging the most recent SAMPLES readings.
   *
   * @param readings Array of recent readings (circular buffer)
   * @param index Current position in circular buffer (passed by reference, auto-increments)
   * @param newReading New raw reading to add to the buffer
   * @return Smoothed value (average of last SAMPLES readings)
   */
  int smoothReading(int readings[], int& index, int newReading);

  // VL53L0X sensors (uses pins from PinConfig.h)
  // PIN_SENSOR_I2C_SDA, PIN_SENSOR_I2C_SCL
  // PIN_SENSOR_PITCH_XSHUT, PIN_SENSOR_VOLUME_XSHUT
  // I2C_ADDR_SENSOR_PITCH, I2C_ADDR_SENSOR_VOLUME

  Adafruit_VL53L0X pitchSensor;
  Adafruit_VL53L0X volumeSensor;
  VL53L0X_RangingMeasurementData_t pitchMeasure;
  VL53L0X_RangingMeasurementData_t volumeMeasure;

  /**
   * Read raw pitch distance from VL53L0X sensor
   */
  int readPitchRaw();

  /**
   * Read raw volume distance from VL53L0X sensor
   */
  int readVolumeRaw();
};
