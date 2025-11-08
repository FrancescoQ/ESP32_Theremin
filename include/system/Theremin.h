/*
 * Theremin.h
 *
 * Main coordinator class for the ESP32 Theremin.
 * Manages sensors and audio engine, mapping sensor input to sound output.
 */

#pragma once
#include "controls/SensorManager.h"
#include "audio/AudioEngine.h"
#include "controls/SerialControls.h"
#include "controls/GPIOControls.h"
#include "system/DisplayManager.h"
#include "system/NotificationManager.h"

// Forward declaration
class PerformanceMonitor;

class Theremin {
 public:
  /**
   * Smoothing preset levels for unified control
   */
  enum SmoothingPreset {
    SMOOTH_NONE = 0,     // Raw/instant response (testing)
    SMOOTH_NORMAL = 1,   // Balanced (default)
    SMOOTH_EXTRA = 2     // Maximum smoothness
  };

  /**
   * Frequency range presets for different playing styles
   */
  enum FrequencyRangePreset {
    RANGE_NARROW = 0,   // 1 octave  - tight playing area
    RANGE_NORMAL = 1,   // 2 octaves - balanced (default)
    RANGE_WIDE = 2      // 3 octaves - extended range
  };

  /**
   * Constructor
   * @param perfMon Pointer to PerformanceMonitor instance (optional)
   */
  Theremin(PerformanceMonitor* perfMon = nullptr, DisplayManager* displayMgr = nullptr);

  /**
   * Destructor - cleans up notification manager
   *
   * NOTE: Like AudioEngine, this destructor is never called in practice
   * (Theremin lives until ESP32 loses power), but included as C++ best
   * practice for proper RAII pattern and resource cleanup documentation.
   */
  ~Theremin();

  /**
   * Initialize theremin (sensors and audio)
   * Must be called in setup()
   * @return true if successful, false if initialization fails
   */
  bool begin();

  /**
   * Main update loop - read sensors and generate audio
   * Call this from loop()
   */
  void update();

  /**
   * Enable or disable debug output
   */
  void setDebugMode(bool enabled);

  /**
   * Get pointer to AudioEngine instance (for direct control access)
   * @return Pointer to AudioEngine
   */
  AudioEngine* getAudioEngine() { return &audio; }

  /**
   * Get pointer to SensorManager instance (for control access)
   * @return Pointer to SensorManager
   */
  SensorManager* getSensorManager() { return &sensors; }

  /**
   * Get pointer to NotificationManager instance (for control access)
   * @return Pointer to NotificationManager, or nullptr if display not available
   */
  NotificationManager* getNotificationManager() { return notifications; }

  /**
   * Set pitch smoothing preset (coordinates both sensor and audio levels)
   * @param preset Smoothing preset level
   */
  void setPitchSmoothingPreset(SmoothingPreset preset);

  /**
   * Set volume smoothing preset (coordinates both sensor and audio levels)
   * @param preset Smoothing preset level
   */
  void setVolumeSmoothingPreset(SmoothingPreset preset);

  /**
   * Set frequency range preset (coordinates both audio frequency range and sensor range)
   * @param preset Frequency range preset level
   */
  void setFrequencyRangePreset(FrequencyRangePreset preset);

 private:
  SensorManager sensors;
  AudioEngine audio;
  SerialControls serialControls;
  GPIOControls gpioControls;
  DisplayManager* display;
  NotificationManager* notifications;
  bool debugEnabled;

  /**
   * Draw splash page for display
   */
  void drawSplashPage(Adafruit_SSD1306& oled);

  /**
   * Draw effects page showing current effect settings
   */
  void drawEffectsPage(Adafruit_SSD1306& oled);

  /**
   * Draw oscillators page showing oscillator configurations
   */
  void drawOscillatorsPage(Adafruit_SSD1306& oled);

  /**
   * Draw audio range page showing frequency and distance ranges
   */
  void drawAudioRangePage(Adafruit_SSD1306& oled);

  // Amplitude range constants (internal use only)
  static const int MIN_AMPLITUDE_PERCENT = 0;
  static const int MAX_AMPLITUDE_PERCENT = 100;

  // Debug output throttling (internal use only)
  static const int DEBUG_THROTTLE_FACTOR = 10;  // Print every Nth loop iteration

  /**
   * Map sensor distances to audio parameters
   * Closer distance = higher frequency and louder volume
   */
  void mapSensorsToAudio();

  /**
   * Floating-point map function for smoother frequency transitions
   * Eliminates quantization artifacts from integer map() function
   */
  float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);

  /**
   * Print debug information to serial
   */
  void printDebugInfo(int pitchDist, int volumeDist, int freq, int amplitude);
};
