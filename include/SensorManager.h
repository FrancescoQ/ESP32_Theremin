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
   * Update sensor readings - reads both sensors and caches results.
   * Call this once per loop iteration before getting individual distances.
   * This ensures sensors are only read once per update cycle.
   */
  void updateReadings();

  /**
   * Read and return smoothed pitch distance in millimeters
   * Range: PITCH_MIN_DIST to PITCH_MAX_DIST (typically 50-400mm)
   * NOTE: Call updateReadings() first to ensure fresh data
   */
  int getPitchDistance();

  /**
   * Read and return smoothed volume distance in millimeters
   * Range: VOLUME_MIN_DIST to VOLUME_MAX_DIST (typically 50-300mm)
   * NOTE: Call updateReadings() first to ensure fresh data
   */
  int getVolumeDistance();

  // Distance ranges (same for both modes)
  static const int PITCH_MIN_DIST = 50;
  static const int PITCH_MAX_DIST = 400;
  static const int VOLUME_MIN_DIST = 50;
  static const int VOLUME_MAX_DIST = 300;

 private:
  // Exponential smoothing filter parameters
  // Alpha controls responsiveness: 0.0 = very smooth/slow, 1.0 = no smoothing/instant
  // Values 0.3-0.4 provide good balance between smoothness and responsiveness
  static constexpr float SMOOTHING_ALPHA = 0.3f;

  float smoothedPitchDistance;
  float smoothedVolumeDistance;
  bool firstReading;  // Track if this is the first reading (to initialize smoothed values)

  // Cached raw readings (updated by updateReadings())
  int cachedPitchRaw;
  int cachedVolumeRaw;

  /**
   * Apply exponential weighted moving average (EWMA) smoothing to sensor readings.
   * Formula: smoothed = alpha * newReading + (1 - alpha) * previousSmoothed
   *
   * This provides better responsiveness than simple moving average while maintaining
   * smooth transitions. Lower alpha = smoother but slower response.
   *
   * @param smoothedValue Current smoothed value (passed by reference, updated in-place)
   * @param newReading New raw reading from sensor
   * @param isFirstReading If true, initialize smoothedValue to newReading
   * @return Smoothed value
   */
  int applyExponentialSmoothing(float& smoothedValue, int newReading, bool isFirstReading);

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
