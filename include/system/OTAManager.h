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

#ifdef ENABLE_NETWORK

  #include <WiFi.h>
  #include <ESPAsyncWebServer.h>
  #include <ElegantOTA.h>

class OTAManager {
 private:
  AsyncWebServer* server;  // Shared pointer (not owned)
  bool isInitialized;

 public:
  /**
   * Constructor
   * @param srv Pointer to shared AsyncWebServer instance (managed by NetworkManager)
   */
  OTAManager(AsyncWebServer* srv);

  /**
   * Initialize WiFi AP and ElegantOTA server
   * @param otaUser Username for OTA page authentication
   * @param otaPass Password for OTA page authentication
   * @return true if initialization successful, false otherwise
   */
  bool begin(const char* otaUser = "", const char* otaPass = "");

  /**
   * Handle incoming OTA requests (call in loop())
   * NOTE: With AsyncWebServer, this is no longer needed but kept for API compatibility.
   * AsyncWebServer handles requests automatically via callbacks.
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

#endif  // ENABLE_NETWORK
