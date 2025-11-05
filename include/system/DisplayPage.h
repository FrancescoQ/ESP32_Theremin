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
    PageDrawCallback drawFunction;

    DisplayPage(String n, PageDrawCallback func)
        : name(n), drawFunction(func) {}
};
