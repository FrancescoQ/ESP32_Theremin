#pragma once

#include <Arduino.h>
#include "audio/AudioEngine.h"
#include "system/DisplayManager.h"

/**
 * @brief TunerManager - Real-time frequency-to-note converter for theremin tuner
 *
 * Provides centralized tuner logic for both OLED display and Web UI:
 * - Converts frequency (Hz) to musical note with cents deviation
 * - Registers OLED display page
 * - Provides data for WebSocket broadcasting
 *
 * Architecture:
 *   AudioEngine → TunerManager → { DisplayManager, WebUIManager }
 *
 * Performance: <1% CPU overhead (simple logarithmic calculations)
 */
class TunerManager {
private:
    AudioEngine* audioEngine;
    DisplayManager* display;

    // Tuner state
    String currentNote;          // Musical note name (e.g., "C#4", "A3")
    String currentNoteName;      // Note name only (e.g., "C#", "A")
    int currentOctave;           // Octave number (e.g., 4, 3)
    float currentFrequency;      // Frequency in Hz
    int cents;                   // Deviation in cents (-50 to +50)
    bool inTune;                 // Within ±10 cents threshold

    // Update tracking
    unsigned long lastUpdate;
    static const int UPDATE_INTERVAL = 100;  // 100ms (10 Hz)

    // Display page callback
    void drawTunerPage(Adafruit_SSD1306& oled);

    // Frequency-to-note conversion
    void calculateTunerData(float frequency);

    // Note names lookup table
    static const char* NOTE_NAMES[12];

public:
    /**
     * @brief Construct TunerManager
     * @param engine Pointer to AudioEngine for frequency access
     */
    TunerManager(AudioEngine* engine);

    /**
     * @brief Set display and register tuner page
     * @param disp Pointer to DisplayManager
     */
    void setDisplay(DisplayManager* disp);

    /**
     * @brief Update tuner calculations (call in main loop)
     */
    void update();

    // Getters for WebUIManager
    /**
     * @brief Get current note with octave (e.g., "C#4")
     */
    const String& getCurrentNote() const { return currentNote; }

    /**
     * @brief Get note name only (e.g., "C#")
     */
    const String& getCurrentNoteName() const { return currentNoteName; }

    /**
     * @brief Get octave number
     */
    int getCurrentOctave() const { return currentOctave; }

    /**
     * @brief Get frequency in Hz
     */
    float getCurrentFrequency() const { return currentFrequency; }

    /**
     * @brief Get cents deviation from perfect pitch
     */
    int getCents() const { return cents; }

    /**
     * @brief Check if note is in tune (within ±10 cents)
     */
    bool isInTune() const { return inTune; }

    /**
     * @brief Check if tuner has valid data (frequency > 20 Hz)
     */
    bool hasValidData() const { return currentFrequency >= 20.0f; }
};
