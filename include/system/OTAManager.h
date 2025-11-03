/*
 * OTAManager.h
 *
 * Manages Over-The-Air (OTA) firmware updates via WiFi Access Point mode.
 * Encapsulates WebServer and ElegantOTA functionality in a clean interface.
 *
 * Features:
 * - Creates WiFi Access Point (no router needed)
 * - HTTP Basic Authentication for security
 * - Fixed IP address: 192.168.4.1
 * - Web interface at: http://192.168.4.1/update
 *
 * Usage:
 *   OTAManager ota("Theremin-OTA", "apPassword");
 *   ota.begin("admin", "otaPassword");
 *
 *   In loop():
 *     ota.handle();
 */

#pragma once

#include <Arduino.h>

#ifdef ENABLE_OTA

  #include <WiFi.h>
  #include <WebServer.h>
  #include <ElegantOTA.h>

class OTAManager {
 private:
  WebServer server;
  String apSSID;
  String apPassword;
  int enablePin;
  bool isInitialized;

 public:

  enum OTAForceState {
    AUTO = 0,
    ALWAYS_ENABLE = 1,
    ALWAYS_DISABLE = 2
  };

  /**
   * Constructor
   * @param ssid Access Point name (visible WiFi network name)
   * @param apPass Access Point password (leave empty for open network, min 8 chars if used)
   * @param buttonPin GPIO pin for enable button (-1 = always enable, >=0 = check button on boot)
   */
  OTAManager(const char* ssid, const char* apPass = "", int buttonPin = -1);

  /**
   * Initialize WiFi AP and ElegantOTA server
   * @param otaUser Username for OTA page authentication
   * @param otaPass Password for OTA page authentication
   * @return true if initialization successful, false otherwise
   */
  bool begin(const char* otaUser = "", const char* otaPass = "", OTAForceState forceState = OTAForceState::AUTO);

  /**
   * Handle incoming OTA requests (call in loop())
   * Non-blocking, processes HTTP requests and OTA updates
   */
  void handle();

  /**
   * Check if OTA manager is initialized and running
   * @return true if running, false otherwise
   */
  bool isRunning() const;

  /**
   * Get the Access Point IP address
   * @return IP address as IPAddress object
   */
  IPAddress getIP() const;
};

#endif  // ENABLE_OTA
