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

void DisplayManager::registerPage(String name, PageDrawCallback drawFunc) {
    pages.emplace_back(name, drawFunc);
    DEBUG_PRINTF("DisplayManager: Registered page '%s' (total: %d)\n",
                 name.c_str(), pages.size());
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

    // Draw current page (for now, always page 0)
    pages[currentPageIndex].drawFunction(display);

    // Push to display
    display.display();
}
