# Display Integration Plan - ESP32 Theremin

## Implementation Status

**Status:** ✅ **CORE SYSTEM COMPLETE** (November 5, 2025)

### What Got Built
- ✅ Page registration system with callback-based rendering
- ✅ Automatic title rendering (optional per page)
- ✅ Page navigation with button integration
- ✅ Page indicator (e.g., "1/2", "2/2")
- ✅ Real-time updates (performance page live at ~200 FPS)
- ✅ Build timestamp display (YYYYMMDD.HHMMSS format)
- ✅ Two functional pages: Splash + Performance

### Implementation Differences from Plan
- **Simplified approach:** No overlay system yet (deferred for future)
- **Title abstraction:** Added automatic title+separator rendering (not in original plan)
- **Page order fix:** Registration order mattered - fixed with setDisplay() pattern
- **Button state machine:** Short press detection required logic fix
- **Build timestamp:** Static function in Theremin.cpp (not separate BuildInfo.h)

### Current Pages
1. **Splash Page** (1/2): "TheremAIn 0.1" centered + build timestamp bottom-right
2. **Performance Page** (2/2): Title "SYSTEM" + Status/Audio/RAM metrics (live updates)

### Files Created
- `include/system/DisplayPage.h` - Page structure with optional title
- `include/system/DisplayManager.h/cpp` - Page manager with auto-title rendering
- Build timestamp formatter in `src/system/Theremin.cpp`

### Future Enhancements (Deferred)
- Overlay system for global indicators
- Additional pages (oscillators, sensors, effects)
- Text alignment helper methods
- Advanced page navigation features

---

## Overview
This document outlines the implementation plan for a flexible display system featuring:
- **Page Registration System**: Hook-based architecture where components register their info pages ✅ **IMPLEMENTED**
- **Overlay System**: Global indicators that appear on top of any page ⏳ **DEFERRED**
- **Text Alignment Helpers**: Simplified methods for common text positioning ⏳ **DEFERRED**

## Architecture

### Core Components

```
Display Manager
├── Pages (vector of registered pages)
│   ├── Main Page (from Theremin)
│   ├── Oscillator Page (from Theremin)
│   ├── OSC1 Detail (from Oscillator 1)
│   ├── OSC2 Detail (from Oscillator 2)
│   └── OSC3 Detail (from Oscillator 3)
└── Overlays (vector of global indicators)
    ├── Button Indicator (from GPIOManager)
    ├── Error Indicator (from ErrorManager)
    └── Notification (from NotificationManager)
```

### Data Flow

```
loop()
  ↓
Update Logic (GPIO, Theremin, etc.)
  ↓ (state changes)
Display::update()
  ↓
1. clearDisplay()
2. Draw current page callback
3. Draw all overlay callbacks (if active)
4. display()
```

## Implementation Plan

### Phase 1: Core Display Class

#### 1.1 Define Page Callback Type
```cpp
// DisplayPage.h
#pragma once

#include <functional>
#include <Adafruit_SSD1306.h>

// Callback type for page drawing functions
using PageDrawCallback = std::function<void(Adafruit_SSD1306&)>;

struct DisplayPage {
    String name;
    PageDrawCallback drawFunction;

    DisplayPage(String n, PageDrawCallback func)
        : name(n), drawFunction(func) {}
};
```

#### 1.2 Display Manager Core
```cpp
// Display.h
#pragma once

#include <Adafruit_SSD1306.h>
#include <vector>
#include "DisplayPage.h"

class Display {
private:
    Adafruit_SSD1306 oled;
    std::vector<DisplayPage> pages;
    std::vector<PageDrawCallback> overlays;
    uint8_t currentPageIndex;
    unsigned long lastPageChange;

public:
    Display(uint8_t width, uint8_t height, TwoWire* wire, int8_t reset = -1);

    bool begin(uint8_t i2cAddress = 0x3C);

    // Page registration
    void registerPage(String name, PageDrawCallback drawFunc);

    // Overlay registration
    void registerOverlay(PageDrawCallback overlayFunc);

    // Navigation
    void nextPage();
    void previousPage();
    void showPage(uint8_t index);

    // Main update (call in loop)
    void update();

    // Getters
    uint8_t getCurrentPageIndex() const { return currentPageIndex; }
    uint8_t getPageCount() const { return pages.size(); }
    String getCurrentPageName() const;

    // Direct OLED access for special cases
    Adafruit_SSD1306& getOLED() { return oled; }
```

#### 1.3 Key Methods Implementation
```cpp
// Display.cpp

void Display::registerPage(String name, PageDrawCallback drawFunc) {
    pages.emplace_back(name, drawFunc);
    Serial.printf("Display: Registered page '%s' (total: %d)\n",
                  name.c_str(), pages.size());
}

void Display::registerOverlay(PageDrawCallback overlayFunc) {
    overlays.push_back(overlayFunc);
    Serial.printf("Display: Registered overlay (total: %d)\n", overlays.size());
}

void Display::nextPage() {
    if (pages.empty()) return;
    currentPageIndex = (currentPageIndex + 1) % pages.size();
    lastPageChange = millis();
}

void Display::update() {
    if (pages.empty()) return;

    oled.clearDisplay();

    // 1. Draw current page
    pages[currentPageIndex].drawFunction(oled);

    // 2. Draw all overlays on top
    for (auto& overlay : overlays) {
        overlay(oled);
    }

    oled.display();
}
```

### Phase 2: Text Alignment Helpers (Optional)

Add these methods to Display class for common text operations:

```cpp
// Simple helpers for 90% of use cases
void Display::showTextCentered(const char* text, int y, int size) {
    int16_t x1, y1;
    uint16_t width, height;
    oled.setTextSize(size);
    oled.getTextBounds(text, 0, 0, &x1, &y1, &width, &height);

    oled.setTextColor(SSD1306_WHITE);
    oled.setCursor(64 - width/2, y);  // Center at x=64
    oled.print(text);
}

void Display::showTextLeft(const char* text, int y, int size) {
    oled.setTextSize(size);
    oled.setTextColor(SSD1306_WHITE);
    oled.setCursor(0, y);
    oled.print(text);
}

void Display::showTextRight(const char* text, int y, int size) {
    int16_t x1, y1;
    uint16_t width, height;
    oled.setTextSize(size);
    oled.getTextBounds(text, 0, 0, &x1, &y1, &width, &height);

    oled.setTextColor(SSD1306_WHITE);
    oled.setCursor(128 - width, y);
    oled.print(text);
}

// Advanced: custom anchor point
enum TextAlign { LEFT, CENTER, RIGHT };

void Display::showText(const char* text, int anchorX, int y, int size, TextAlign align) {
    int16_t x1, y1;
    uint16_t width, height;
    oled.setTextSize(size);
    oled.getTextBounds(text, 0, 0, &x1, &y1, &width, &height);

    int finalX;
    switch(align) {
        case CENTER: finalX = anchorX - width/2; break;
        case RIGHT:  finalX = anchorX - width; break;
        case LEFT:
        default:     finalX = anchorX; break;
    }

    oled.setTextColor(SSD1306_WHITE);
    oled.setCursor(finalX, y);
    oled.print(text);
}
```

### Phase 3: Component Integration

#### 3.1 Theremin Pages Registration

```cpp
// Theremin.h
class Theremin {
private:
    Display* display;

    // Page drawing methods
    void drawMainPage(Adafruit_SSD1306& oled) {
        oled.setTextSize(1);
        oled.setTextColor(SSD1306_WHITE);
        oled.setCursor(0, 0);
        oled.printf("THEREMIN v%s\n", VERSION);
        oled.printf("Freq: %d Hz\n", currentFrequency);
        oled.printf("Vol:  %d%%\n", currentVolume);
        oled.printf("Osc:  %d/%d ON\n", activeOscCount, totalOscCount);
        oled.printf("CPU:  %.1f%%\n", cpuUsage);
    }

    void drawOscillatorsPage(Adafruit_SSD1306& oled) {
        oled.setTextSize(1);
        oled.setTextColor(SSD1306_WHITE);
        oled.setCursor(0, 0);
        oled.println("OSCILLATORS");

        for (int i = 0; i < oscillators.size(); i++) {
            char status = oscillators[i]->isActive() ? '*' : ' ';
            oled.printf("[%c] OSC%d: %s\n",
                       status, i+1, oscillators[i]->getWaveformName());
        }
    }

public:
    Theremin(Display* disp) : display(disp) {
        // Register pages using lambdas that capture 'this'
        display->registerPage("Main", [this](Adafruit_SSD1306& oled) {
            this->drawMainPage(oled);
        });

        display->registerPage("Oscillators", [this](Adafruit_SSD1306& oled) {
            this->drawOscillatorsPage(oled);
        });
    }
};
```

#### 3.2 Individual Oscillator Pages

```cpp
// Oscillator.h
class Oscillator {
private:
    Display* display;
    int oscNumber;

    void drawDetailPage(Adafruit_SSD1306& oled) {
        oled.setTextSize(1);
        oled.setTextColor(SSD1306_WHITE);
        oled.setCursor(0, 0);

        oled.printf("OSCILLATOR %d\n\n", oscNumber);
        oled.printf("Waveform: %s\n", getWaveformName());
        oled.printf("Frequency: %d Hz\n", frequency);
        oled.printf("Amplitude: %d\n", amplitude);
        oled.printf("Active: %s\n", isActive() ? "YES" : "NO");
        oled.printf("CPU: %.2f%%\n", cpuUsage);
    }

public:
    Oscillator(int num, Display* disp) : oscNumber(num), display(disp) {
        if (display) {
            String pageName = "OSC" + String(num);
            display->registerPage(pageName, [this](Adafruit_SSD1306& oled) {
                this->drawDetailPage(oled);
            });
        }
    }
};
```

#### 3.3 GPIO Manager Overlay

```cpp
// GPIOManager.h
class GPIOManager {
private:
    Display* display;
    bool buttonLongPressed;

    void drawButtonIndicator(Adafruit_SSD1306& oled) {
        if (buttonLongPressed) {
            // Draw indicator circle in top-right corner
            oled.fillCircle(120, 5, 3, SSD1306_WHITE);
        }
        // When false, draws nothing = "removed"
    }

public:
    GPIOManager(Display* disp) : display(disp), buttonLongPressed(false) {
        // Register overlay once in constructor
        display->registerOverlay([this](Adafruit_SSD1306& oled) {
            this->drawButtonIndicator(oled);
        });
    }

    void update() {
        // Update button state
        bool pressed = digitalRead(BUTTON_PIN) == LOW;

        if (pressed && millis() - pressStart > 1000) {
            buttonLongPressed = true;
        } else {
            buttonLongPressed = false;
        }

        // Display will automatically call overlay when rendering
    }
};
```

### Phase 4: Main Integration

```cpp
// main.cpp
#include "Display.h"
#include "Theremin.h"
#include "Oscillator.h"
#include "GPIOManager.h"

Display display(128, 64, &Wire, -1);
Theremin theremin(&display);
Oscillator osc1(1, &display);
Oscillator osc2(2, &display);
Oscillator osc3(3, &display);
GPIOManager gpio(&display);

const int PAGE_BUTTON_PIN = 25;
bool lastButtonState = HIGH;

void setup() {
    Serial.begin(115200);
    Wire.begin();

    if (!display.begin(0x3C)) {
        Serial.println("Display init failed!");
        while(1);
    }

    pinMode(PAGE_BUTTON_PIN, INPUT_PULLUP);

    Serial.printf("Display initialized with %d pages\n", display.getPageCount());
}

void loop() {
    // 1. Update logic
    gpio.update();
    theremin.update();

    // 2. Handle page navigation button
    bool currentButtonState = digitalRead(PAGE_BUTTON_PIN);
    if (currentButtonState == LOW && lastButtonState == HIGH) {
        display.nextPage();
        Serial.printf("Switched to page: %s\n", display.getCurrentPageName().c_str());
        delay(50);  // Debounce
    }
    lastButtonState = currentButtonState;

    // 3. Update display (renders current page + overlays)
    display.update();

    delay(50);  // ~20 FPS
}
```

## Screen Layout Conventions

### Reserved Areas for Overlays

```
┌──────────────────────────┐
│ER     TITLE           BT │  Top 10px reserved for overlays
├──────────────────────────┤
│                          │
│                          │
│      PAGE CONTENT        │  Main content area (10-54px)
│                          │
│                          │
├──────────────────────────┤
│      NOTIFICATIONS       │  Bottom 10px for messages
└──────────────────────────┘

Legend:
ER = Error indicator (top-left, 0-10px)
BT = Button indicator (top-right, 118-128px)
```

### Recommended Overlay Positions

```cpp
// Top-left corner (errors, warnings)
oled.fillCircle(5, 5, 4, SSD1306_WHITE);

// Top-right corner (button feedback, status)
oled.fillCircle(120, 5, 3, SSD1306_WHITE);

// Bottom full-width (notifications, messages)
oled.setCursor(0, 56);
oled.print("Message here");
```

## Testing Strategy

### Phase 1: Test Display Core
1. Create minimal sketch with Display class
2. Register 2-3 dummy pages with simple text
3. Test page navigation
4. Verify memory usage

### Phase 2: Test One Component
1. Integrate Theremin class first
2. Register its pages
3. Verify rendering and navigation
4. Check for crashes/memory issues

### Phase 3: Add More Components
1. Add Oscillator pages one by one
2. Verify each registration
3. Test with all pages cycling

### Phase 4: Add Overlays
1. Add GPIO overlay
2. Test overlay appears on all pages
3. Add more overlays if needed
4. Verify no overlap/interference

### Testing in Wokwi
- Create "Display Playground" project
- Quick iteration on layouts
- Test animations and timing
- Verify before hardware upload

## Common Patterns

### Pattern 1: Label + Value Layout
```cpp
void drawInfoLine(const char* label, const char* value, int y) {
    oled.setCursor(0, y);
    oled.print(label);

    // Right-align value
    int16_t x1, y1;
    uint16_t w, h;
    oled.getTextBounds(value, 0, 0, &x1, &y1, &w, &h);
    oled.setCursor(128 - w, y);
    oled.print(value);
}

// Usage:
drawInfoLine("Freq:", "440 Hz", 10);
drawInfoLine("Vol:", "85%", 20);
```

### Pattern 2: Multi-Column Layout
```cpp
// Three equal columns
int col1 = 21;   // Center of first column (0-42)
int col2 = 64;   // Center of second column (43-85)
int col3 = 107;  // Center of third column (86-128)

// Center text in each column
showTextCenter("OSC1", col1, 0);
showTextCenter("OSC2", col2, 0);
showTextCenter("OSC3", col3, 0);
```

### Pattern 3: Progress/Level Bars
```cpp
void drawBar(const char* label, int value, int maxValue, int y) {
    // Label
    oled.setCursor(0, y);
    oled.print(label);

    // Bar
    int barX = 40;
    int barWidth = 80;
    int barHeight = 6;
    int fillWidth = (value * barWidth) / maxValue;

    oled.drawRect(barX, y, barWidth, barHeight, SSD1306_WHITE);
    oled.fillRect(barX, y, fillWidth, barHeight, SSD1306_WHITE);

    // Percentage
    oled.setCursor(barX + barWidth + 4, y);
    oled.printf("%d%%", (value * 100) / maxValue);
}
```

## Performance Considerations

### Memory Usage
- Each lambda callback: ~16 bytes
- Each DisplayPage: ~20 bytes + lambda
- Example: 5 pages + 3 overlays ≈ 150 bytes

### CPU Usage
- `clearDisplay()`: <1ms
- Draw operations: ~5-10ms total
- `display()` I2C transfer: ~12ms at 400kHz
- **Total per frame: ~15-20ms → 50+ FPS possible**

### Optimization Tips
1. Draw only what changes (if not using clearDisplay)
2. Use smaller fonts where possible
3. Limit overlays to essential indicators
4. Keep page callbacks simple and fast

## Future Enhancements

### Possible Additions
- **Page priorities**: Always show important pages first
- **Conditional pages**: Register/unregister based on state
- **Auto-rotation**: Cycle through pages automatically
- **Sticky pages**: Prevent auto-advance for critical info
- **Overlay Z-order**: Control overlay drawing order
- **Transition effects**: Fade/slide between pages
- **Page metadata**: Add icons, categories, shortcuts

### Advanced Features
```cpp
// Example: Conditional registration
void Theremin::updatePages() {
    if (effectsEnabled && !effectPageRegistered) {
        display->registerPage("Effects", [this](...) { drawEffectsPage(...); });
        effectPageRegistered = true;
    }
}

// Example: Page with refresh rate control
struct DisplayPage {
    String name;
    PageDrawCallback drawFunction;
    uint32_t minRefreshMs;  // Don't redraw faster than this
    unsigned long lastDraw;
};
```

## Summary

This architecture provides:
- ✅ **Decoupled components**: Each class manages its own pages
- ✅ **Scalability**: Add pages/overlays without modifying Display
- ✅ **Flexibility**: Lambda callbacks access private class members
- ✅ **Global indicators**: Overlays work across all pages
- ✅ **Clean API**: Simple registration and automatic rendering

### Key Concepts
1. **Pages** = Content that fills the screen (one visible at a time)
2. **Overlays** = Small indicators that appear on top of any page
3. **Lambda captures** = `[this]` allows callbacks to access class members
4. **State-based rendering** = Update state, then render based on state

### Implementation Order
1. Display core class with page registration ✓
2. Add text helper methods (optional) ✓
3. Integrate one component (Theremin) ✓
4. Add more component pages (Oscillators) ✓
5. Add overlay system ✓
6. Integrate GPIO overlay ✓
7. Test and refine ✓

---

**Status**: Ready for implementation
**Estimated Time**: 2-4 hours for full integration
**Dependencies**: Adafruit_GFX, Adafruit_SSD1306

Good luck! 🚀
