/*
 * DisplayPage.h
 *
 * Helper structures for Display page registration system.
 * Defines callback types and page data structures.
 */

#pragma once

#include <functional>
#include <Adafruit_SSD1306.h>
#include <WString.h>

// Callback type for page drawing functions
// Takes reference to OLED display and draws content
using PageDrawCallback = std::function<void(Adafruit_SSD1306&)>;

/**
 * Represents a single display page with name and drawing function
 */
struct DisplayPage {
    String name;
    String title;  // Optional title - if provided, auto-drawn by DisplayManager
    PageDrawCallback drawFunction;
    int weight;    // Sort order (lower = earlier, alphabetical for ties)

    DisplayPage(String n, PageDrawCallback func, String t = "", int w = 0)
        : name(n), title(t), drawFunction(func), weight(w) {}
};
