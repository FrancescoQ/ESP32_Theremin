/*
 * SensorManager.h
 *
 * Manages distance sensor input for the ESP32 Theremin.
 * Supports both hardware (VL53L0X I2C sensors) and simulation (potentiometers via ADC).
 * Provides smoothed distance readings for pitch and volume control.
 */

#pragma once
#include <Arduino.h>

// Conditional includes based on build mode
#ifdef WOKWI_SIMULATION
  // Simulation mode: no hardware sensor libraries needed
#else
  // Hardware mode: VL53L0X Time-of-Flight sensors
  #include <Wire.h>
  #include "Adafruit_VL53L0X.h"
#endif

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
     * Apply moving average smoothing to sensor readings
     * @param readings Array of recent readings
     * @param index Current position in circular buffer
     * @param newReading New raw reading to add
     * @return Smoothed value
     */
    int smoothReading(int readings[], int &index, int newReading);

#ifdef WOKWI_SIMULATION
    // Simulation mode: potentiometer pins
    static const int PITCH_INPUT_PIN = 34;    // ADC1_CH6
    static const int VOLUME_INPUT_PIN = 35;   // ADC1_CH7

    /**
     * Read raw pitch distance from ADC (simulation mode)
     */
    int readPitchRaw();

    /**
     * Read raw volume distance from ADC (simulation mode)
     */
    int readVolumeRaw();

    /**
     * Convert ADC value (0-4095) to distance in mm
     */
    int adcToDistance(int adc, int minDist, int maxDist);

#else
    // Hardware mode: VL53L0X sensors
    static const int SDA_PIN = 21;
    static const int SCL_PIN = 22;
    static const int XSHUT_PIN_1 = 16;
    static const int XSHUT_PIN_2 = 17;
    static const uint8_t SENSOR_ADDR_1 = 0x30;  // Pitch sensor
    static const uint8_t SENSOR_ADDR_2 = 0x29;  // Volume sensor

    Adafruit_VL53L0X pitchSensor;
    Adafruit_VL53L0X volumeSensor;
    VL53L0X_RangingMeasurementData_t pitchMeasure;
    VL53L0X_RangingMeasurementData_t volumeMeasure;

    /**
     * Read raw pitch distance from VL53L0X sensor (hardware mode)
     */
    int readPitchRaw();

    /**
     * Read raw volume distance from VL53L0X sensor (hardware mode)
     */
    int readVolumeRaw();
#endif
};
