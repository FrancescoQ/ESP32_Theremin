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
  // Simulation mode: configure ADC pins
  pinMode(PITCH_INPUT_PIN, INPUT);
  pinMode(VOLUME_INPUT_PIN, INPUT);
  DEBUG_PRINTLN("[SENSOR] Analog inputs configured (GPIO34, GPIO35)");
  return true;

#else
  // Hardware mode: initialize I2C and VL53L0X sensors
  Wire.begin(SDA_PIN, SCL_PIN);
  DEBUG_PRINTLN("[SENSOR] I2C initialized");

  // Configure XSHUT pins
  pinMode(XSHUT_PIN_1, OUTPUT);
  pinMode(XSHUT_PIN_2, OUTPUT);

  // Disable both sensors initially
  digitalWrite(XSHUT_PIN_1, LOW);
  digitalWrite(XSHUT_PIN_2, LOW);
  delay(10);

  // Initialize pitch sensor at custom address 0x30
  digitalWrite(XSHUT_PIN_1, HIGH);
  delay(10);
  if (!pitchSensor.begin(SENSOR_ADDR_1)) {
    DEBUG_PRINTLN("[SENSOR] ERROR: Pitch sensor failed to initialize!");
    return false;
  }
  DEBUG_PRINTLN("[SENSOR] Pitch sensor initialized at 0x30");

  // Initialize volume sensor at default address 0x29
  digitalWrite(XSHUT_PIN_2, HIGH);
  delay(10);
  if (!volumeSensor.begin(SENSOR_ADDR_2)) {
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
  int adc = analogRead(PITCH_INPUT_PIN);
  return adcToDistance(adc, PITCH_MIN_DIST, PITCH_MAX_DIST);
}

int SensorManager::readVolumeRaw() {
  int adc = analogRead(VOLUME_INPUT_PIN);
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
  // Return measured distance, or max distance if out of range
  return (volumeMeasure.RangeStatus != 4) ? volumeMeasure.RangeMilliMeter : VOLUME_MAX_DIST;
}

#endif
