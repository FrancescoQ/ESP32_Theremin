/*
 * OTAManager.cpp
 *
 * Implementation of OTA firmware update manager using WiFi Access Point mode.
 */

#include "system/OTAManager.h"

#ifdef ENABLE_NETWORK

  #include "system/Debug.h"

// Constructor
OTAManager::OTAManager(AsyncWebServer* srv) : server(srv), isInitialized(false) {}

// Initialize ElegantOTA (WiFi/AP is handled by NetworkManager)
bool OTAManager::begin(const char* otaUser, const char* otaPass) {
  DEBUG_PRINTLN("[OTA] Initializing OTA updates...");

  // Initialize ElegantOTA with optional authentication
  if (strlen(otaUser) > 0 && strlen(otaPass) > 0) {
    DEBUG_PRINT("OTA Authentication: Enabled (user: ");
    DEBUG_PRINT(otaUser);
    DEBUG_PRINTLN(")");
    ElegantOTA.begin(server, otaUser, otaPass);
  } else {
    DEBUG_PRINTLN("OTA Authentication: Disabled (open access)");
    ElegantOTA.begin(server);
  }

  // Set OTA callbacks (optional, for debugging)
  ElegantOTA.onStart([]() { DEBUG_PRINTLN("\n[OTA] Update started..."); });

  ElegantOTA.onProgress([](size_t current, size_t final) {
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 1000) {  // Print every second
      DEBUG_PRINT("[OTA] Progress: ");
      DEBUG_PRINT(String((current * 100) / final).c_str());
      DEBUG_PRINTLN("%");
      lastPrint = millis();
    }
  });

  ElegantOTA.onEnd([](bool success) {
    if (success) {
      DEBUG_PRINTLN("\n[OTA] Update successful! Rebooting...");
    } else {
      DEBUG_PRINTLN("\n[OTA] Update failed!");
    }
  });

  DEBUG_PRINTLN("[OTA] ✓ OTA updates enabled");
  DEBUG_PRINTLN("[OTA] Access OTA at /update route");

  isInitialized = true;
  return true;
}

// Handle OTA requests (call in loop())
// NOTE: With AsyncWebServer, this is no longer needed as the server handles
// requests automatically via callbacks. Kept for API compatibility.
void OTAManager::handle() {
  if (!isInitialized) {
    return;
  }

  // AsyncWebServer handles requests automatically - no manual handling needed
  // ElegantOTA.loop() is also handled automatically with async mode
}

// Check if OTA is running
bool OTAManager::isRunning() const {
  return isInitialized;
}

// Get AP IP address
IPAddress OTAManager::getIP() const {
  return WiFi.softAPIP();
}

#endif  // ENABLE_NETWORK
