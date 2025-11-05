/*
 * DisplayManager.cpp
 *
 * Implementation of page-based DisplayManager for SSD1306 OLED display.
 */

#include "system/DisplayManager.h"
#include "system/Debug.h"

const GFXfont* DisplayManager::SMALL_FONT = &TomThumb;

DisplayManager::DisplayManager()
    : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET),
      currentPageIndex(0),
      initialized(false) {
}

bool DisplayManager::begin() {
    DEBUG_PRINTLN("DisplayManager: Initializing SSD1306 display...");

    // Initialize display with I2C address from PinConfig
    if (!display.begin(SSD1306_SWITCHCAPVCC, PIN_DISPLAY_I2C_ADDR)) {
        DEBUG_PRINTLN("DisplayManager: ERROR - SSD1306 allocation failed!");
        DEBUG_PRINTF("DisplayManager: Check I2C address (trying 0x%02X)\n", PIN_DISPLAY_I2C_ADDR);
        initialized = false;
        return false;
    }

    DEBUG_PRINTLN("DisplayManager: Display initialized successfully");

    // Clear display
    display.clearDisplay();
    display.display();

    initialized = true;
    return true;
}

void DisplayManager::registerPage(String name, PageDrawCallback drawFunc, String title) {
    pages.emplace_back(name, drawFunc, title);
    DEBUG_PRINTF("DisplayManager: Registered page '%s' (total: %d)\n",
                 name.c_str(), pages.size());
}

void DisplayManager::registerOverlay(PageDrawCallback overlayFunc) {
    overlays.push_back(overlayFunc);
    DEBUG_PRINTF("DisplayManager: Registered overlay (total: %d)\n", overlays.size());
}

String DisplayManager::getCurrentPageName() const {
    if (pages.empty() || currentPageIndex >= pages.size()) {
        return "";
    }
    return pages[currentPageIndex].name;
}

void DisplayManager::nextPage() {
    if (pages.empty()) {
        return;
    }

    currentPageIndex = (currentPageIndex + 1) % pages.size();
    DEBUG_PRINTF("DisplayManager: Switched to page '%s' (%d/%d)\n",
                 getCurrentPageName().c_str(), currentPageIndex + 1, pages.size());
}

void DisplayManager::update() {
    if (!initialized || pages.empty()) {
        return;
    }

    // Clear display
    display.clearDisplay();

    // Auto-draw title if provided
    if (!pages[currentPageIndex].title.isEmpty()) {
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.print(pages[currentPageIndex].title);

        // Draw separator line below title
        display.drawLine(0, 9, SCREEN_WIDTH - 1, 9, SSD1306_WHITE);
    }

    // Draw current page content
    pages[currentPageIndex].drawFunction(display);

    // Draw all overlays on top
    for (auto& overlay : overlays) {
        overlay(display);
    }

    // Draw page indicator (top-right corner) if multiple pages exist
    if (pages.size() > 1) {
        char indicator[8];
        snprintf(indicator, sizeof(indicator), "%d/%d", currentPageIndex + 1, (int)pages.size());

        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);

        // Calculate position (right-aligned, 2px from edges)
        int16_t x1, y1;
        uint16_t w, h;
        display.getTextBounds(indicator, 0, 0, &x1, &y1, &w, &h);
        display.setCursor(SCREEN_WIDTH - w - 1, 1);
        display.setFont(DisplayManager::SMALL_FONT);
        display.print(indicator);
        display.setFont();  // Reset to default font
    }

    // Push to display
    display.display();
}
