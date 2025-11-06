/*
 * Theremin.cpp
 *
 * Implementation of the main Theremin coordinator class.
 * Maps sensor input to audio output.
 */

#include "system/Theremin.h"
#include "system/Debug.h"

// Constructor
Theremin::Theremin(PerformanceMonitor* perfMon, DisplayManager* displayMgr)
    : sensors(), audio(perfMon), serialControls(this), gpioControls(this, displayMgr), display(displayMgr), debugEnabled(false) {
  // Register splash page if display is available
  if (display) {
    display->registerPage("Splash", [this](Adafruit_SSD1306& oled) {
      this->drawSplashPage(oled);
    });
  }
}

// Initialize theremin
bool Theremin::begin() {
  DEBUG_PRINTLN("\n=== ESP32 Theremin Initializing ===");

  // Initialize sensors
  if (!sensors.begin()) {
    DEBUG_PRINTLN("[ERROR] Sensor initialization failed!");
    return false;
  }

  // Initialize audio
  audio.begin();

  // Initialize serial controls
  serialControls.begin();

  // Initialize GPIO controls
  if (gpioControls.begin()) {
    DEBUG_PRINTLN("[INIT] Physical GPIO controls enabled");
  } else {
    DEBUG_PRINTLN("[INIT] Physical GPIO controls unavailable - serial only");
  }

  DEBUG_PRINTLN("=== Initialization Complete ===\n");
  return true;
}

// Helper function: floating-point map for smoother frequency transitions
// Eliminates quantization artifacts from integer map() function
float Theremin::mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// Main update loop
void Theremin::update() {
  // Handle control inputs (serial commands and GPIO switches)
  serialControls.update();
  gpioControls.update();

  // Update sensor readings (reads both sensors once and caches results)
  // Always read hardware, even if sensors are disabled
  sensors.updateReadings();

  // Only apply pitch sensor if enabled
  if (sensors.isPitchEnabled()) {
    int pitchDistance = sensors.getPitchDistance();

    // Map pitch to frequency using floating-point math for smooth transitions
    // Use dynamic ranges from audio engine and sensor manager
    float frequencyFloat = mapFloat((float)pitchDistance,
                                     (float)sensors.getPitchMinDist(),
                                     (float)sensors.getPitchMaxDist(),
                                     (float)audio.getMaxFrequency(),
                                     (float)audio.getMinFrequency());

    // Constrain and convert to integer
    int frequency = constrain((int)frequencyFloat, audio.getMinFrequency(), audio.getMaxFrequency());

    // Update audio engine frequency
    audio.setFrequency(frequency);
  }

  // Only apply volume sensor if enabled
  if (sensors.isVolumeEnabled()) {
    int volumeDistance = sensors.getVolumeDistance();

    // Map volume using integer math (volume doesn't need sub-Hz precision)
    // Traditional theremin behavior: hand NEAR volume antenna = QUIET, hand FAR = LOUD
    int amplitude =
        map(volumeDistance, SensorManager::VOLUME_MIN_DIST, SensorManager::VOLUME_MAX_DIST,
            MIN_AMPLITUDE_PERCENT,     // Min amplitude (closest) - near sensor = quiet
            MAX_AMPLITUDE_PERCENT);    // Max amplitude (farthest) - far from sensor = loud
    amplitude = constrain(amplitude, MIN_AMPLITUDE_PERCENT, MAX_AMPLITUDE_PERCENT);

    // Update audio engine amplitude
    audio.setAmplitude(amplitude);
  }

  // This now do nothing, FreeRTOS task handles audio updates
  audio.update();

  // Debug output (throttled to every 10th loop)
  if (debugEnabled) {
    int pitchDistance = sensors.getPitchDistance();
    int volumeDistance = sensors.getVolumeDistance();
    // Note: frequency and amplitude shown in debug may not match current audio
    // if sensors are disabled, but shows sensor readings for diagnostics
    float frequencyFloat = mapFloat((float)pitchDistance,
                                     (float)sensors.getPitchMinDist(),
                                     (float)sensors.getPitchMaxDist(),
                                     (float)audio.getMaxFrequency(),
                                     (float)audio.getMinFrequency());
    int frequency = constrain((int)frequencyFloat, audio.getMinFrequency(), audio.getMaxFrequency());
    int amplitude = map(volumeDistance, SensorManager::VOLUME_MIN_DIST, SensorManager::VOLUME_MAX_DIST,
                        MIN_AMPLITUDE_PERCENT, MAX_AMPLITUDE_PERCENT);
    amplitude = constrain(amplitude, MIN_AMPLITUDE_PERCENT, MAX_AMPLITUDE_PERCENT);
    printDebugInfo(pitchDistance, volumeDistance, frequency, amplitude);
  }
}

// Enable/disable debug output
void Theremin::setDebugMode(bool enabled) {
  debugEnabled = enabled;
}

// Print debug information (throttled to avoid flooding serial monitor)
// Uses static local counter to print only every 10th loop iteration (~5 times/second at 50Hz)
void Theremin::printDebugInfo(int pitchDist, int volumeDist, int freq, int amplitude) {
  static unsigned int loopCounter = 0;  // Static local - only increments when debug is enabled

  if (loopCounter % DEBUG_THROTTLE_FACTOR == 0) {  // Print only when counter is divisible by throttle factor
    DEBUG_PRINT("[PITCH] ");
    DEBUG_PRINT(pitchDist);
    DEBUG_PRINT("mm → ");
    DEBUG_PRINT(freq);
    DEBUG_PRINT("Hz  |  [VOLUME] ");
    DEBUG_PRINT(volumeDist);
    DEBUG_PRINT("mm → ");
    DEBUG_PRINT(amplitude);
    DEBUG_PRINTLN("%");
  }

  // For future me: if you're worried about overflow, this will wrap around
  // safely, and in any case at 50Hz it should take something like ~994 days
  // to overflow... that's a long song...
  loopCounter++;
}

// Set pitch smoothing preset (coordinates both sensor and audio levels)
void Theremin::setPitchSmoothingPreset(SmoothingPreset preset) {
  DEBUG_PRINT("[THEREMIN] Setting pitch smoothing preset: ");
  DEBUG_PRINTLN((int)preset);

  switch (preset) {
    case SMOOTH_NONE:
      // Sensor: disabled, Audio: instant
      sensors.setPitchSmoothingEnabled(false);
      audio.setPitchSmoothingFactor(1.0f);  // Instant
      DEBUG_PRINTLN("[THEREMIN] Pitch smoothing: NONE (raw response)");
      break;

    case SMOOTH_NORMAL:
      // Sensor: α=0.35, Audio: factor=0.80 (default balanced)
      sensors.setPitchSmoothingEnabled(true);
      sensors.setPitchSmoothingAlpha(0.35f);
      audio.setPitchSmoothingFactor(0.80f);
      DEBUG_PRINTLN("[THEREMIN] Pitch smoothing: NORMAL (balanced)");
      break;

    case SMOOTH_EXTRA:
      // Sensor: α=0.20 (more smooth), Audio: factor=0.50 (more smooth)
      sensors.setPitchSmoothingEnabled(true);
      sensors.setPitchSmoothingAlpha(0.20f);
      audio.setPitchSmoothingFactor(0.50f);
      DEBUG_PRINTLN("[THEREMIN] Pitch smoothing: EXTRA (maximum smooth)");
      break;
  }
}

// Set volume smoothing preset (coordinates both sensor and audio levels)
void Theremin::setVolumeSmoothingPreset(SmoothingPreset preset) {
  DEBUG_PRINT("[THEREMIN] Setting volume smoothing preset: ");
  DEBUG_PRINTLN((int)preset);

  switch (preset) {
    case SMOOTH_NONE:
      // Sensor: disabled, Audio: instant
      sensors.setVolumeSmoothingEnabled(false);
      audio.setVolumeSmoothingFactor(1.0f);  // Instant
      DEBUG_PRINTLN("[THEREMIN] Volume smoothing: NONE (raw response)");
      break;

    case SMOOTH_NORMAL:
      // Sensor: α=0.35, Audio: factor=0.80 (default balanced)
      sensors.setVolumeSmoothingEnabled(true);
      sensors.setVolumeSmoothingAlpha(0.35f);
      audio.setVolumeSmoothingFactor(0.80f);
      DEBUG_PRINTLN("[THEREMIN] Volume smoothing: NORMAL (balanced)");
      break;

    case SMOOTH_EXTRA:
      // Sensor: α=0.20 (more smooth), Audio: factor=0.50 (more smooth)
      sensors.setVolumeSmoothingEnabled(true);
      sensors.setVolumeSmoothingAlpha(0.20f);
      audio.setVolumeSmoothingFactor(0.50f);
      DEBUG_PRINTLN("[THEREMIN] Volume smoothing: EXTRA (maximum smooth)");
      break;
  }
}

// Helper function to format build timestamp from compiler macros
static const char* formatBuildTimestamp() {
  static char timestamp[20];  // "YYYYMMDD.HHMMSS\0"
  static bool formatted = false;

  if (!formatted) {
    // Parse __DATE__ (e.g., "Nov  5 2025")
    const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    char month_str[4];
    int day, year;
    sscanf(__DATE__, "%s %d %d", month_str, &day, &year);

    int month = 1;
    for (int i = 0; i < 12; i++) {
      if (strcmp(month_str, months[i]) == 0) {
        month = i + 1;
        break;
      }
    }

    // Parse __TIME__ (e.g., "14:15:23")
    int hour, minute, second;
    sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second);

    // Format as YYYYMMDD.HHMMSS
    snprintf(timestamp, sizeof(timestamp), "%04d%02d%02d.%02d%02d%02d",
             year, month, day, hour, minute, second);

    formatted = true;
  }

  return timestamp;
}

// Set frequency range preset (coordinates both audio frequency range and sensor range)
void Theremin::setFrequencyRangePreset(FrequencyRangePreset preset) {
  DEBUG_PRINT("[THEREMIN] Setting frequency range preset: ");
  DEBUG_PRINTLN((int)preset);

  switch (preset) {
    case RANGE_NARROW:
      // 1 octave: A4-A5 (440-880 Hz)
      // Tight sensor range for intimate playing
      audio.setFrequencyRange(440, 880);
      sensors.setPitchRange(50, 300);  // 250mm range
      DEBUG_PRINTLN("[THEREMIN] Range: NARROW (1 octave, 250mm)");
      break;

    case RANGE_NORMAL:
      // 2 octaves: A3-A5 (220-880 Hz) - DEFAULT
      // Standard sensor range
      audio.setFrequencyRange(220, 880);
      sensors.setPitchRange(50, 400);  // 350mm range
      DEBUG_PRINTLN("[THEREMIN] Range: NORMAL (2 octaves, 350mm)");
      break;

    case RANGE_WIDE:
      // 3 octaves: A2-A5 (110-880 Hz)
      // Extended sensor range for precision
      audio.setFrequencyRange(110, 880);
      sensors.setPitchRange(50, 500);  // 450mm range
      DEBUG_PRINTLN("[THEREMIN] Range: WIDE (3 octaves, 450mm)");
      break;
  }
}

// Draw splash page for display
void Theremin::drawSplashPage(Adafruit_SSD1306& oled) {
  // Use default font for splash (larger, more readable)
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);

  // Calculate centered position for "TheremAIn 0.1"
  const char* text = "TheremAIn 0.1";
  int16_t x1, y1;
  uint16_t w, h;
  oled.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

  int x = (DisplayManager::SCREEN_WIDTH - w) / 2;
  int y = (DisplayManager::SCREEN_HEIGHT - h) / 2;

  oled.setCursor(x, y);
  oled.print(text);

  // Add build timestamp at bottom right in small font
  oled.setFont(DisplayManager::SMALL_FONT);
  const char* buildTime = formatBuildTimestamp();
  oled.getTextBounds(buildTime, 0, 0, &x1, &y1, &w, &h);
  oled.setCursor(DisplayManager::SCREEN_WIDTH - w - 2, DisplayManager::SCREEN_HEIGHT - 5);
  oled.print(buildTime);
  oled.setFont();  // Reset to default font
}
