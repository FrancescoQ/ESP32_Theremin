/*
 * NetworkManager.h
 *
 * Central manager for all network-related functionality.
 * Encapsulates WiFi connection, OTA updates, and web server.
 *
 * Features:
 * - Auto-connect to saved WiFi (STA mode)
 * - Fallback to Access Point mode if connection fails
 * - Captive portal for WiFi configuration
 * - mDNS registration (theremin.local)
 * - OTA firmware updates
 * - Network status display page
 *
 * Usage:
 *   NetworkManager network(&display);
 *   network.begin("Theremin-Setup", "admin", "theremin");
 *
 *   In loop():
 *     network.update();
 */

#pragma once

#include <Arduino.h>

#ifdef ENABLE_NETWORK

  #include <WiFi.h>
  #include <WiFiManager.h>
  #include <ESPmDNS.h>
  #include <ESPAsyncWebServer.h>
  #include "system/OTAManager.h"
  #include "system/DisplayManager.h"
  #include "system/WebUIManager.h"

// Forward declaration
class Theremin;

class NetworkManager {
 private:
  // Core components
  WiFiManager wifiManager;
  AsyncWebServer server;
  OTAManager ota;
  WebUIManager* webUI;  // WebUIManager (created dynamically after Theremin is available)
  DisplayManager* display;
  Theremin* theremin;

  bool isInitialized;

  // Configuration
  String apName;
  String mdnsHostname;

  // Display page callback (member function)
  void renderNetworkPage(Adafruit_SSD1306& display);

  // Internal setup methods
  void setupWiFi(uint8_t connectTimeout, uint16_t portalTimeout, bool resetCredentials);
  void setupMDNS(const char* hostname);
  void setupOTA(const char* user, const char* pass);
  void setupStaticFiles();

 public:
  /**
     * Constructor
     * @param disp Pointer to DisplayManager for status page registration
     * @param thmn Pointer to Theremin instance for WebUI control (optional, can be set later)
     */
  NetworkManager(DisplayManager* disp, Theremin* thmn = nullptr);

  /**
     * Destructor - cleans up WebUIManager
     */
  ~NetworkManager();

  /**
     * Set Theremin instance (must be called before begin() if not provided in constructor)
     * @param thmn Pointer to Theremin instance
     */
  void setTheremin(Theremin* thmn);

  /**
     * Initialize all network services
     * @param apName Access Point name for config portal
     * @param otaUser OTA authentication username
     * @param otaPass OTA authentication password
     * @param connectTimeout Seconds to try connecting to saved WiFi (default: 15)
     * @param portalTimeout Seconds before portal times out (0 = never, default: 0)
     * @return true if initialization successful
     */
  bool begin(const char* apName = "Theremin-Setup", const char* otaUser = "admin",
             const char* otaPass = "theremin", uint8_t connectTimeout = 15,
             uint16_t portalTimeout = 0, bool resetCredentials = false);

  /**
     * Non-blocking update for network monitoring
     * Call in loop() for future reconnect logic, status updates, etc.
     */
  void update();

  /**
     * Get server instance for WebUI registration (Phase 3)
     * @return Pointer to AsyncWebServer instance
     */
  AsyncWebServer* getServer() {
    return &server;
  }

  /**
     * Check if connected to WiFi (STA mode)
     * @return true if connected to a WiFi network
     */
  bool isConnected() const;

  /**
     * Get current IP address
     * @return IP address (STA mode IP or AP IP)
     */
  IPAddress getIP() const;

  /**
     * Get current WiFi mode
     * @return "STA" if connected to WiFi, "AP" if in Access Point mode, "Off" if disabled
     */
  String getMode() const;

  /**
     * Get connected WiFi SSID (STA mode only)
     * @return SSID name or "N/A" if not connected
     */
  String getSSID() const;

  /**
     * Get WiFi signal strength (STA mode only)
     * @return RSSI in dBm, or 0 if not connected
     */
  int8_t getRSSI() const;

  /**
     * Check if NetworkManager is initialized and running
     * @return true if running
     */
  bool isRunning() const {
    return isInitialized;
  }
};

#endif  // ENABLE_NETWORK
