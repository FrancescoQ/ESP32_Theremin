/*
 * ESP32 Theremin - Clean Architecture
 *
 * Single file supporting both Wokwi simulation and hardware via conditional compilation.
 * Uses WOKWI_SIMULATION macro to switch between potentiometers (simulation) and VL53L0X sensors (hardware).
 *
 * Build Commands:
 * - Simulation: pio run -e esp32dev-wokwi
 * - Hardware:   pio run -e esp32dev
 */

#include <Arduino.h>

// ============================================================================
// CONDITIONAL INCLUDES & DEFINITIONS
// ============================================================================
#ifdef WOKWI_SIMULATION
  // Simulation mode: potentiometers via ADC
  #define PITCH_INPUT_PIN 34    // ADC1_CH6
  #define VOLUME_INPUT_PIN 35   // ADC1_CH7

  // Helper to convert ADC to distance
  int adcToDistance(int adc, int minDist, int maxDist) {
    return map(adc, 0, 4095, minDist, maxDist);
  }
#else
  // Hardware mode: VL53L0X sensors via I2C
  #include <Wire.h>
  #include "Adafruit_VL53L0X.h"

  #define SDA_PIN 21
  #define SCL_PIN 22
  #define XSHUT_PIN_1 16
  #define XSHUT_PIN_2 17
  #define SENSOR_ADDR_1 0x30
  #define SENSOR_ADDR_2 0x29

  Adafruit_VL53L0X pitchSensor = Adafruit_VL53L0X();
  Adafruit_VL53L0X volumeSensor = Adafruit_VL53L0X();
  VL53L0X_RangingMeasurementData_t pitchMeasure;
  VL53L0X_RangingMeasurementData_t volumeMeasure;
#endif

// ============================================================================
// SHARED CONFIGURATION (same for both modes)
// ============================================================================
#define BUZZER_PIN 25
#define PWM_CHANNEL 0
#define PWM_RESOLUTION 8
#define PWM_FREQUENCY 2000

#define PITCH_MIN_DIST 50
#define PITCH_MAX_DIST 400
#define VOLUME_MIN_DIST 50
#define VOLUME_MAX_DIST 300

#define MIN_FREQUENCY 100
#define MAX_FREQUENCY 2000
#define MIN_DUTY_CYCLE 0
#define MAX_DUTY_CYCLE 128

#define SAMPLES 5
int pitchReadings[SAMPLES];
int volumeReadings[SAMPLES];
int pitchIndex = 0;
int volumeIndex = 0;

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================
#ifdef WOKWI_SIMULATION
  void simulationSetup();
  int simulationReadPitch();
  int simulationReadVolume();
#else
  void hardwareSetup();
  int hardwareReadPitch();
  int hardwareReadVolume();
#endif

void sharedSetup();
void processAndPlayAudio(int pitchDistance, int volumeDistance);
int smoothReading(int readings[], int &index, int newReading);

// ============================================================================
// SETUP (Clean & Simple)
// ============================================================================
void setup() {
  Serial.begin(115200);
  delay(100);

  #ifdef WOKWI_SIMULATION
    Serial.println("\n=== ESP32 Theremin [SIMULATION] ===");
    simulationSetup();
  #else
    Serial.println("\n=== ESP32 Theremin [HARDWARE] ===");
    hardwareSetup();
  #endif

  sharedSetup();
  Serial.println("=== Ready! ===\n");
}

// ============================================================================
// MAIN LOOP (Clean & Simple)
// ============================================================================
void loop() {
  int pitchDistance, volumeDistance;

  #ifdef WOKWI_SIMULATION
    pitchDistance = simulationReadPitch();
    volumeDistance = simulationReadVolume();
  #else
    pitchDistance = hardwareReadPitch();
    volumeDistance = hardwareReadVolume();
  #endif

  processAndPlayAudio(pitchDistance, volumeDistance);
  delay(20);
}

// ============================================================================
// SIMULATION-SPECIFIC FUNCTIONS
// ============================================================================
#ifdef WOKWI_SIMULATION

void simulationSetup() {
  pinMode(PITCH_INPUT_PIN, INPUT);
  pinMode(VOLUME_INPUT_PIN, INPUT);
  Serial.println("[INIT] Analog inputs configured (GPIO34, GPIO35)");
}

int simulationReadPitch() {
  int adc = analogRead(PITCH_INPUT_PIN);
  return adcToDistance(adc, PITCH_MIN_DIST, PITCH_MAX_DIST);
}

int simulationReadVolume() {
  int adc = analogRead(VOLUME_INPUT_PIN);
  return adcToDistance(adc, VOLUME_MIN_DIST, VOLUME_MAX_DIST);
}

#endif

// ============================================================================
// HARDWARE-SPECIFIC FUNCTIONS
// ============================================================================
#ifndef WOKWI_SIMULATION

void hardwareSetup() {
  Wire.begin(SDA_PIN, SCL_PIN);
  Serial.println("[INIT] I2C initialized");

  pinMode(XSHUT_PIN_1, OUTPUT);
  pinMode(XSHUT_PIN_2, OUTPUT);
  digitalWrite(XSHUT_PIN_1, LOW);
  digitalWrite(XSHUT_PIN_2, LOW);
  delay(10);

  // Initialize pitch sensor at 0x30
  digitalWrite(XSHUT_PIN_1, HIGH);
  delay(10);
  if (!pitchSensor.begin(SENSOR_ADDR_1)) {
    Serial.println("[ERROR] Pitch sensor failed!");
    while(1);
  }
  Serial.println("[INIT] Pitch sensor initialized at 0x30");

  // Initialize volume sensor at 0x29
  digitalWrite(XSHUT_PIN_2, HIGH);
  delay(10);
  if (!volumeSensor.begin(SENSOR_ADDR_2)) {
    Serial.println("[ERROR] Volume sensor failed!");
    while(1);
  }
  Serial.println("[INIT] Volume sensor initialized at 0x29");
}

int hardwareReadPitch() {
  pitchSensor.rangingTest(&pitchMeasure, false);
  return (pitchMeasure.RangeStatus != 4) ? pitchMeasure.RangeMilliMeter : PITCH_MAX_DIST;
}

int hardwareReadVolume() {
  volumeSensor.rangingTest(&volumeMeasure, false);
  return (volumeMeasure.RangeStatus != 4) ? volumeMeasure.RangeMilliMeter : VOLUME_MAX_DIST;
}

#endif

// ============================================================================
// SHARED FUNCTIONS (used by both modes)
// ============================================================================

void sharedSetup() {
  ledcSetup(PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcAttachPin(BUZZER_PIN, PWM_CHANNEL);
  Serial.println("[INIT] PWM configured");

  for (int i = 0; i < SAMPLES; i++) {
    pitchReadings[i] = 0;
    volumeReadings[i] = 0;
  }
  Serial.println("[INIT] Smoothing filter initialized");
}

int smoothReading(int readings[], int &index, int newReading) {
  readings[index] = newReading;
  index = (index + 1) % SAMPLES;

  long sum = 0;
  for (int i = 0; i < SAMPLES; i++) {
    sum += readings[i];
  }
  return sum / SAMPLES;
}

void processAndPlayAudio(int pitchDistance, int volumeDistance) {
  // Apply smoothing
  int pitchSmooth = smoothReading(pitchReadings, pitchIndex, pitchDistance);
  int volumeSmooth = smoothReading(volumeReadings, volumeIndex, volumeDistance);

  // Map to audio parameters (inverse: closer = higher/louder)
  int frequency = map(pitchSmooth, PITCH_MIN_DIST, PITCH_MAX_DIST,
                      MAX_FREQUENCY, MIN_FREQUENCY);
  int dutyCycle = map(volumeSmooth, VOLUME_MIN_DIST, VOLUME_MAX_DIST,
                      MAX_DUTY_CYCLE, MIN_DUTY_CYCLE);

  // Constrain to valid ranges
  frequency = constrain(frequency, MIN_FREQUENCY, MAX_FREQUENCY);
  dutyCycle = constrain(dutyCycle, MIN_DUTY_CYCLE, MAX_DUTY_CYCLE);

  // Generate audio
  if (dutyCycle > 5) {
    ledcWriteTone(PWM_CHANNEL, frequency);
    ledcWrite(PWM_CHANNEL, dutyCycle);
  } else {
    ledcWriteTone(PWM_CHANNEL, 0);
  }

  // Debug output (throttled)
  static int loopCount = 0;
  if (loopCount++ % 10 == 0) {
    Serial.print("[PITCH] ");
    Serial.print(pitchSmooth);
    Serial.print("mm → ");
    Serial.print(frequency);
    Serial.print("Hz  |  [VOLUME] ");
    Serial.print(volumeSmooth);
    Serial.print("mm → ");
    Serial.print(dutyCycle);
    Serial.println();
  }
}
