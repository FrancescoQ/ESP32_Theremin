/*
 * SensorManager.cpp
 *
 * Implementation of sensor management for ESP32 Theremin.
 * Handles both simulation (potentiometers) and hardware (VL53L0X) modes.
 */

#include "SensorManager.h"
#include "Debug.h"

// Constructor
SensorManager::SensorManager() : pitchIndex(0), volumeIndex(0) {
  // Initialize smoothing arrays to zero
  for (int i = 0; i < SAMPLES; i++) {
    pitchReadings[i] = 0;
    volumeReadings[i] = 0;
  }
}

// Initialize sensors
bool SensorManager::begin() {
#ifdef WOKWI_SIMULATION
  // Simulation mode: configure ADC pins (from PinConfig.h)
  pinMode(PIN_SENSOR_PITCH_ADC, INPUT);
  pinMode(PIN_SENSOR_VOLUME_ADC, INPUT);
  DEBUG_PRINTLN("[SENSOR] Analog inputs configured (GPIO34, GPIO35)");
  return true;

#else
  // Hardware mode: initialize I2C and VL53L0X sensors
  Wire.begin(PIN_SENSOR_I2C_SDA, PIN_SENSOR_I2C_SCL);
  DEBUG_PRINTLN("[SENSOR] I2C initialized");

  // Configure XSHUT pins
  pinMode(PIN_SENSOR_PITCH_XSHUT, OUTPUT);
  pinMode(PIN_SENSOR_VOLUME_XSHUT, OUTPUT);

  // Disable both sensors initially
  digitalWrite(PIN_SENSOR_PITCH_XSHUT, LOW);
  digitalWrite(PIN_SENSOR_VOLUME_XSHUT, LOW);
  delay(10);

  // Initialize pitch sensor at custom address 0x30
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
#endif
}

// Get smoothed pitch distance
int SensorManager::getPitchDistance() {
  int rawDistance = readPitchRaw();
  return smoothReading(pitchReadings, pitchIndex, rawDistance);
}

// Get smoothed volume distance
int SensorManager::getVolumeDistance() {
  int rawDistance = readVolumeRaw();
  return smoothReading(volumeReadings, volumeIndex, rawDistance);
}

// Apply moving average smoothing
int SensorManager::smoothReading(int readings[], int& index, int newReading) {
  readings[index] = newReading;
  index = (index + 1) % SAMPLES;

  long sum = 0;
  for (int i = 0; i < SAMPLES; i++) {
    sum += readings[i];
  }
  return sum / SAMPLES;
}

#ifdef WOKWI_SIMULATION
// ============================================================================
// SIMULATION MODE IMPLEMENTATIONS
// ============================================================================

int SensorManager::readPitchRaw() {
  int adc = analogRead(PIN_SENSOR_PITCH_ADC);
  return adcToDistance(adc, PITCH_MIN_DIST, PITCH_MAX_DIST);
}

int SensorManager::readVolumeRaw() {
  int adc = analogRead(PIN_SENSOR_VOLUME_ADC);
  return adcToDistance(adc, VOLUME_MIN_DIST, VOLUME_MAX_DIST);
}

int SensorManager::adcToDistance(int adc, int minDist, int maxDist) {
  return map(adc, 0, 4095, minDist, maxDist);
}

#else
// ============================================================================
// HARDWARE MODE IMPLEMENTATIONS
// ============================================================================

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

#endif
