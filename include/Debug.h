/*
 * Debug.h
 *
 * Centralized debug output control for ESP32 Theremin.
 * Set DEBUG_MODE to 0 for production builds - all debug code will be
 * completely removed by the compiler (zero overhead).
 */

#pragma once

// ============================================================================
// DEBUG CONFIGURATION
// ============================================================================

// Set to 1 for debug output, 0 for production (zero runtime overhead)
#ifndef DEBUG_MODE
  #define DEBUG_MODE 1
#endif

// ============================================================================
// DEBUG MACROS
// ============================================================================

#if DEBUG_MODE
  // Debug mode ENABLED: Macros expand to actual Serial calls
  #define DEBUG_INIT(baudrate) Serial.begin(baudrate)
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINTF(fmt, ...) Serial.printf(fmt, __VA_ARGS__)
  #define DEBUG_FLUSH() Serial.flush()
#else
  // Debug mode DISABLED: Macros expand to nothing (removed by compiler)
  #define DEBUG_INIT(baudrate) ((void)0)
  #define DEBUG_PRINT(x) ((void)0)
  #define DEBUG_PRINTLN(x) ((void)0)
  #define DEBUG_PRINTF(fmt, ...) ((void)0)
  #define DEBUG_FLUSH() ((void)0)
#endif

/*
 * Usage:
 *
 * #include "Debug.h"
 *
 * void setup() {
 *     DEBUG_INIT(9600);              // Replaces Serial.begin(9600)
 *     DEBUG_PRINTLN("Starting...");  // Replaces Serial.println("Starting...")
 * }
 *
 * void loop() {
 *     DEBUG_PRINT("Value: ");        // Replaces Serial.print("Value: ")
 *     DEBUG_PRINTLN(42);             // Replaces Serial.println(42)
 * }
 *
 * When DEBUG_MODE = 0, all DEBUG_* calls are completely removed during
 * compilation, resulting in zero code size and zero runtime overhead.
 */
