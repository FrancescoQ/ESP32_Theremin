/*
 * DisplayManager.h
 *
 * Page-based display manager for SSD1306 OLED display.
 * Components register pages using callbacks, DisplayManager handles rendering.
 */

#pragma once

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <vector>
#include "system/DisplayPage.h"
#include "system/PinConfig.h"

// Fonts
#include <Fonts/TomThumb.h>  // 3x5 px - Small but readable

class DisplayManager {
public:
    DisplayManager();

    // Display dimensions and constants
    static constexpr int SCREEN_WIDTH = 128;
    static constexpr int SCREEN_HEIGHT = 64;
    static constexpr int CHAR_HEIGHT = 8;
    static constexpr int CHAR_WIDTH = 5;
    static constexpr int LINE_HEIGHT = 10;
    static constexpr int CONTENT_START_Y = 14;  // Y position where page content starts (below title separator)

    // Default font
    static const GFXfont* SMALL_FONT;

    /**
     * Initialize the display
     * @return true if successful, false on error
     */
    bool begin();

    /**
     * Register a new page with name and drawing callback
     * @param name Page name (for debugging/navigation)
     * @param drawFunc Callback function that draws page content
     * @param title Optional title to auto-draw at top (empty = no title)
     * @param weight Sort order (lower values appear first, alphabetical for ties, default = 0)
     */
    void registerPage(String name, PageDrawCallback drawFunc, String title = "", int weight = 0);

    /**
     * Register a global overlay (appears on all pages)
     * @param overlayFunc Callback function that draws overlay content
     */
    void registerOverlay(PageDrawCallback overlayFunc);

    /**
     * Update display - renders current page
     * Call this in main loop
     */
    void update();

    /**
     * Navigate to next page (wraps around to first page)
     */
    void nextPage();

    /**
     * Navigate to previous page (wraps around to last page)
     */
    void previousPage();

    /**
     * Get current page index
     * @return Index of currently displayed page
     */
    uint8_t getCurrentPageIndex() const { return currentPageIndex; }

    /**
     * Get total number of registered pages
     * @return Page count
     */
    uint8_t getPageCount() const { return pages.size(); }

    /**
     * Get current page name
     * @return Name of current page, or empty string if no pages
     */
    String getCurrentPageName() const;

    /**
     * Check if display is initialized
     * @return true if ready, false otherwise
     */
    bool isInitialized() const { return initialized; }

    /**
     * Direct access to OLED display (for advanced use)
     * @return Reference to underlying Adafruit_SSD1306 object
     */
    Adafruit_SSD1306& getDisplay() { return display; }

private:
    Adafruit_SSD1306 display;
    std::vector<DisplayPage> pages;
    std::vector<PageDrawCallback> overlays;
    uint8_t currentPageIndex;
    bool initialized;

    static constexpr int OLED_RESET = -1;  // Reset pin not used
};
