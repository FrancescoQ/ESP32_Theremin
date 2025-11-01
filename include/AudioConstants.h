/*
 * AudioConstants.h
 *
 * System-wide audio constants for ESP32 Theremin.
 * Centralizes sample format and sample rate definitions.
 *
 * IMPORTANT: When upgrading to 24-bit DAC:
 * - Change SAMPLE_BIT_DEPTH to 24
 * - Change sample types from int16_t to int32_t
 * - Update SAMPLE_MIN/MAX/RANGE accordingly
 * - Recompile - all components will adapt automatically
 */

#pragma once
#include <Arduino.h>

namespace Audio {
    // ============================================
    // SAMPLE FORMAT (16-bit signed)
    // ============================================
    constexpr uint8_t SAMPLE_BIT_DEPTH = 16;
    constexpr int16_t SAMPLE_MIN = -32768;
    constexpr int16_t SAMPLE_MAX = 32767;
    constexpr uint32_t SAMPLE_RANGE = 65535;

    // Future 24-bit upgrade (commented out):
    // constexpr uint8_t SAMPLE_BIT_DEPTH = 24;
    // constexpr int32_t SAMPLE_MIN = -8388608;
    // constexpr int32_t SAMPLE_MAX = 8388607;
    // constexpr uint32_t SAMPLE_RANGE = 16777215;

    // ============================================
    // SYSTEM SAMPLE RATE
    // ============================================
    constexpr uint32_t SAMPLE_RATE = 22050;  // Hz
}
