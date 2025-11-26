#include "system/TunerManager.h"
#include "system/Debug.h"
#include <cmath>

// Note names lookup table
const char* TunerManager::NOTE_NAMES[12] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

TunerManager::TunerManager(AudioEngine* engine)
    : audioEngine(engine),
      display(nullptr),
      currentNote("---"),
      currentNoteName("---"),
      currentOctave(0),
      currentFrequency(0.0f),
      cents(0),
      inTune(false),
      lastUpdate(0) {
    DEBUG_PRINTLN("[Tuner] TunerManager initialized");
}

void TunerManager::setDisplay(DisplayManager* disp) {
    display = disp;

    if (display) {
        // Register tuner page with DisplayManager
        // Pattern: registerPage(name, callback, title, weight)
        display->registerPage("Tuner", [this](Adafruit_SSD1306& oled) {
            this->drawTunerPage(oled);
        }, "TUNER", 50);

        DEBUG_PRINTLN("[Tuner] Display page registered");
    }
}

void TunerManager::update() {
    unsigned long now = millis();

    if (now - lastUpdate >= UPDATE_INTERVAL) {
        lastUpdate = now;

        // Get current frequency from audio engine
        float frequency = static_cast<float>(audioEngine->getFrequency());

        // Calculate tuner data
        calculateTunerData(frequency);
    }
}

void TunerManager::calculateTunerData(float frequency) {
    // Handle invalid/low frequencies
    if (frequency < 20.0f) {
        currentNote = "---";
        currentNoteName = "---";
        currentOctave = 0;
        currentFrequency = 0.0f;
        cents = 0;
        inTune = false;
        return;
    }

    currentFrequency = frequency;

    // A4 = 440 Hz = MIDI note 69
    const float A4 = 440.0f;
    const int A4_MIDI = 69;

    // Calculate MIDI note number using logarithmic formula
    // halfSteps = 12 * log2(f / 440)
    float halfSteps = 12.0f * log2f(frequency / A4);
    int midiNote = round(A4_MIDI + halfSteps);

    // Calculate cents deviation (how far off from perfect pitch)
    // 100 cents = 1 semitone
    float exactNote = A4_MIDI + halfSteps;
    cents = round((exactNote - midiNote) * 100);

    // Constrain cents to ±50 range (beyond that, it's closer to another note)
    if (cents < -50) cents = -50;
    if (cents > 50) cents = 50;

    // Determine if in tune (within ±10 cents threshold)
    inTune = (abs(cents) <= 10);

    // Extract note name and octave
    int noteIndex = midiNote % 12;
    currentNoteName = NOTE_NAMES[noteIndex];
    currentOctave = (midiNote / 12) - 1;

    // Build full note string (e.g., "C#4")
    currentNote = currentNoteName + String(currentOctave);
}

void TunerManager::drawTunerPage(Adafruit_SSD1306& oled) {
    // Title and separator are auto-drawn by DisplayManager
    // Cursor already positioned at CONTENT_START_Y by DisplayManager
    oled.setFont();
    oled.setTextSize(1);
    oled.setTextColor(SSD1306_WHITE);

    if (!hasValidData()) {
        // No signal or frequency too low
        oled.println("No signal");
        oled.println();
        oled.println("Play to see");
        oled.println("note info");
        return;
    }

    // Display note name (large) with directional indicators
    // Use multiple setCursor calls to place indicators at fixed positions
    // and center the note name dynamically based on its width
    oled.setTextSize(2);

    int16_t y = oled.getCursorY();

    // Calculate centered position for note name
    // Each character is 12 pixels wide at textSize(2)
    int16_t noteWidth = currentNoteName.length() * 12;
    int16_t noteX = (128 - noteWidth) / 2;  // Center on 128-pixel screen

    // Draw left indicator at fixed position if needed
    if (inTune || cents < 0) {
        oled.setCursor(35, y);  // Fixed X position for <
        oled.print("<");
    }

    // Draw the centered note name
    oled.setCursor(noteX, y);
    oled.print(currentNoteName);

    // Draw right indicator at fixed position if needed
    if (inTune || cents > 0) {
        oled.setCursor(80, y);  // Fixed X position for >
        oled.print(">");
    }

    // Move to next line (size 2 text is 16 pixels tall)
    oled.setCursor(0, y);
    oled.println();

    // Display frequency
    oled.setTextSize(1);
    oled.println();
    oled.print(currentFrequency, 1);  // 1 decimal place
    oled.print(" Hz / ");
    if (cents > 0) {
      oled.print("+");
    };
    oled.println(cents);
}
