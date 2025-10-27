/*
 * ESP32 Theremin - Main Entry Point
 *
 * Hardware: ESP32 + 2x VL53L0X sensors + DAC audio output
 */

/*
 * I2S test with volume sensor
 * Fixed 440Hz pitch, volume controlled by sensor
 */

#include <Arduino.h>
#include <driver/i2s.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>
#include "Oscillator.h"
#include "PinConfig.h"
#include "Debug.h"

// SAMPLE_RATE is defined in Oscillator.h
const int BUFFER_SIZE = 512;
uint16_t audioBuffer[BUFFER_SIZE];

// XSHUT pins for sensor address assignment
const int PITCH_XSHUT_PIN = 16;
const int VOLUME_XSHUT_PIN = 17;

// Sensors
Adafruit_VL53L0X pitchSensor = Adafruit_VL53L0X();
Adafruit_VL53L0X volumeSensor = Adafruit_VL53L0X();

// Oscillator (with wavetable lookup - FAST!)
Oscillator oscillator;
int currentAmplitude = 100;  // 0-100%

void setup() {
  Serial.begin(115200);
  delay(1000);
  DEBUG_PRINTLN("\n=== ESP32 Theremin with I2S Audio ===");
  DEBUG_PRINTLN("Pitch + Volume sensors, 220-880 Hz range");

  // Initialize I2C
  Wire.begin();
  DEBUG_PRINTLN("I2C initialized");

  // Initialize XSHUT pins
  pinMode(PITCH_XSHUT_PIN, OUTPUT);
  pinMode(VOLUME_XSHUT_PIN, OUTPUT);

  // Reset both sensors (active LOW)
  digitalWrite(PITCH_XSHUT_PIN, LOW);
  digitalWrite(VOLUME_XSHUT_PIN, LOW);
  delay(10);

  // Enable pitch sensor and set to 0x30
  digitalWrite(PITCH_XSHUT_PIN, HIGH);
  delay(10);
  if (!pitchSensor.begin(I2C_ADDR_SENSOR_PITCH)) {
    DEBUG_PRINTLN("ERROR: Pitch sensor init failed!");
    while(1) delay(1000);
  }
  DEBUG_PRINTLN("Pitch sensor initialized at 0x30");

  // Enable volume sensor at default 0x29
  digitalWrite(VOLUME_XSHUT_PIN, HIGH);
  delay(10);
  if (!volumeSensor.begin(I2C_ADDR_SENSOR_VOLUME)) {
    DEBUG_PRINTLN("ERROR: Volume sensor init failed!");
    while(1) delay(1000);
  }
  DEBUG_PRINTLN("Volume sensor initialized at 0x29");

  // Configure I2S in built-in DAC mode
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
    .communication_format = I2S_COMM_FORMAT_STAND_MSB,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 2,
    .dma_buf_len = BUFFER_SIZE,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  // Install I2S driver
  esp_err_t result = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  if (result != ESP_OK) {
    DEBUG_PRINT("I2S install failed: ");
    DEBUG_PRINTLN(result);
    while(1) delay(1000);
  }

  // Enable built-in DAC
  i2s_set_dac_mode(I2S_DAC_CHANNEL_RIGHT_EN);

  DEBUG_PRINTLN("I2S initialized");
}

void loop() {
  static unsigned long lastSensorRead = 0;
  static unsigned long lastDebug = 0;
  static unsigned long lastLoop = micros();

  // Stagger sensor reads to reduce I2C overhead per loop
  // Alternate between pitch and volume every 10ms
  // Each sensor updates every 20ms (2.5× faster than before!)
  static bool readPitch = true;

  if (millis() - lastSensorRead > 10) {
    if (readPitch) {
      // Read pitch sensor
      VL53L0X_RangingMeasurementData_t pitchMeasure;
      pitchSensor.rangingTest(&pitchMeasure, false);

      if (pitchMeasure.RangeStatus != 4) {  // 4 = out of range
        int pitchDistance = pitchMeasure.RangeMilliMeter;
        // Map distance: NEAR (50mm) = HIGH (880Hz), FAR (400mm) = LOW (220Hz)
        float frequency = map(constrain(pitchDistance, 50, 400), 50, 400, 880, 220);
        // Update oscillator frequency (wavetable lookup, not sin()!)
        oscillator.setFrequency(frequency);
      }
    } else {
      // Read volume sensor
      VL53L0X_RangingMeasurementData_t volumeMeasure;
      volumeSensor.rangingTest(&volumeMeasure, false);

      if (volumeMeasure.RangeStatus != 4) {  // 4 = out of range
        int volumeDistance = volumeMeasure.RangeMilliMeter;
        // Map distance 50-400mm to amplitude 0-100%
        currentAmplitude = map(constrain(volumeDistance, 50, 400), 50, 400, 0, 100);
      }
    }

    readPitch = !readPitch;  // Alternate for next time
    lastSensorRead = millis();
  }

  // Pre-fill and send 4 buffers per loop to prevent gaps
  // 4 buffers × 512 samples = 2048 samples = 92.9ms audio
  // More resilient to sensor I2C delays
  for (int bufferNum = 0; bufferNum < 4; bufferNum++) {
    // Fill buffer with wavetable samples (FAST - just array lookup!)
    for (int i = 0; i < BUFFER_SIZE; i++) {
      // Get next sample from wavetable oscillator (-128 to 127)
      int8_t sample = oscillator.getNextSample();

      // Apply amplitude (0-100%)
      int16_t scaledValue = (int16_t)(sample * currentAmplitude / 100.0f);
      uint8_t dacValue = 128 + scaledValue;

      audioBuffer[i] = (uint16_t)(dacValue << 8);
    }

    // Send to I2S (DMA will output continuously)
    size_t bytes_written;
    i2s_write(I2S_NUM_0, audioBuffer, BUFFER_SIZE * sizeof(uint16_t), &bytes_written, portMAX_DELAY);
  }

  // Debug output every 2 seconds (AFTER buffer filling to avoid audio interruption)
  if (millis() - lastDebug > 2000) {
    // RAM stats
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t totalHeap = ESP.getHeapSize();
    uint32_t usedHeap = totalHeap - freeHeap;
    float ramPercent = (usedHeap * 100.0f) / totalHeap;

    // Loop timing
    unsigned long loopTime = micros() - lastLoop;

    DEBUG_PRINT("Freq: ");
    DEBUG_PRINT((int)oscillator.getFrequency());
    DEBUG_PRINT("Hz | Amp: ");
    DEBUG_PRINT(currentAmplitude);
    DEBUG_PRINT("% | RAM: ");
    DEBUG_PRINT(usedHeap / 1024);
    DEBUG_PRINT("KB/");
    DEBUG_PRINT(totalHeap / 1024);
    DEBUG_PRINT("KB (");
    DEBUG_PRINT(ramPercent, 1);
    DEBUG_PRINT("%) | Loop: ");
    DEBUG_PRINT(loopTime / 1000.0f, 2);
    DEBUG_PRINTLN("ms");

    lastDebug = millis();
  }

  lastLoop = micros();
}
