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
#include "system/PinConfig.h"

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

  /**
   * Enable or disable pitch sensor
   * @param enabled True to enable, false to disable
   */
  void setPitchEnabled(bool enabled);

  /**
   * Enable or disable volume sensor
   * @param enabled True to enable, false to disable
   */
  void setVolumeEnabled(bool enabled);

  /**
   * Check if pitch sensor is enabled
   * @return True if enabled
   */
  bool isPitchEnabled() const { return pitchEnabled; }

  /**
   * Check if volume sensor is enabled
   * @return True if enabled
   */
  bool isVolumeEnabled() const { return volumeEnabled; }

  /**
   * Enable or disable volume smoothing
   * When disabled, provides instant/raw sensor response (useful for testing reverb trails)
   * @param enabled True to enable smoothing (default), false for instant response
   */
  void setVolumeSmoothingEnabled(bool enabled);

  /**
   * Check if volume smoothing is enabled
   * @return True if smoothing enabled
   */
  bool isVolumeSmoothingEnabled() const { return volumeSmoothingEnabled; }

  /**
   * Enable or disable pitch smoothing
   * When disabled, provides instant/raw sensor response
   * @param enabled True to enable smoothing (default), false for instant response
   */
  void setPitchSmoothingEnabled(bool enabled);

  /**
   * Check if pitch smoothing is enabled
   * @return True if smoothing enabled
   */
  bool isPitchSmoothingEnabled() const { return pitchSmoothingEnabled; }

  /**
   * Set pitch smoothing alpha value
   * @param alpha Smoothing factor (0.0 = very smooth/slow, 1.0 = no smoothing/instant)
   */
  void setPitchSmoothingAlpha(float alpha);

  /**
   * Set volume smoothing alpha value
   * @param alpha Smoothing factor (0.0 = very smooth/slow, 1.0 = no smoothing/instant)
   */
  void setVolumeSmoothingAlpha(float alpha);

  /**
   * Get current pitch smoothing alpha
   */
  float getPitchSmoothingAlpha() const { return pitchSmoothingAlpha; }

  /**
   * Get current volume smoothing alpha
   */
  float getVolumeSmoothingAlpha() const { return volumeSmoothingAlpha; }

  /**
   * Set pitch sensor distance range dynamically
   * @param minDist Minimum distance in mm
   * @param maxDist Maximum distance in mm
   */
  void setPitchRange(int minDist, int maxDist);

  /**
   * Get current pitch minimum distance
   * @return Minimum distance in mm
   */
  int getPitchMinDist() const { return pitchMinDist; }

  /**
   * Get current pitch maximum distance
   * @return Maximum distance in mm
   */
  int getPitchMaxDist() const { return pitchMaxDist; }

  // Default distance ranges
  static const int DEFAULT_PITCH_MIN_DIST = 50;
  static const int DEFAULT_PITCH_MAX_DIST = 400;
  static const int VOLUME_MIN_DIST = 50;
  static const int VOLUME_MAX_DIST = 400;

 private:
  // Exponential smoothing filter parameters
  // Alpha controls responsiveness: 0.0 = very smooth/slow, 1.0 = no smoothing/instant
  // Values 0.3-0.4 provide good balance between smoothness and responsiveness
  // Alpha value is "how much (percentage) of new value to mix to the existing smoothed value"
  static constexpr float DEFAULT_SMOOTHING_ALPHA = 0.35f;

  float smoothedPitchDistance;
  float smoothedVolumeDistance;
  bool firstReading;  // Track if this is the first reading (to initialize smoothed values)

  // Runtime-configurable smoothing alpha values
  float pitchSmoothingAlpha;   // Pitch sensor smoothing (0.0-1.0)
  float volumeSmoothingAlpha;  // Volume sensor smoothing (0.0-1.0)

  // Cached raw readings (updated by updateReadings())
  int cachedPitchRaw;
  int cachedVolumeRaw;

  // Dynamic pitch sensor range (can be changed at runtime)
  int pitchMinDist;
  int pitchMaxDist;

  // Sensor enable state
  bool pitchEnabled;   // Pitch sensor enable state
  bool volumeEnabled;  // Volume sensor enable state

  // Smoothing enable state
  bool pitchSmoothingEnabled;   // Pitch smoothing enable state
  bool volumeSmoothingEnabled;  // Volume smoothing enable state

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
   * @param alpha Smoothing factor (0.0-1.0)
   * @return Smoothed value
   */
  int applyExponentialSmoothing(float& smoothedValue, int newReading, bool isFirstReading, float alpha);

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
