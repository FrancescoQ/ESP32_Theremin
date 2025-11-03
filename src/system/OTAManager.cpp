/*
 * OTAManager.cpp
 *
 * Implementation of OTA firmware update manager using WiFi Access Point mode.
 */

#include "system/OTAManager.h"

#ifdef ENABLE_OTA

#include "system/Debug.h"

// Constructor
OTAManager::OTAManager(const char* ssid, const char* apPass, int buttonPin)
    : server(80), apSSID(ssid), apPassword(apPass), enablePin(buttonPin),
      isInitialized(false) {}

// Initialize WiFi AP and ElegantOTA
bool OTAManager::begin(const char* otaUser, const char* otaPass, OTAForceState forceState) {

  // Immediately return if OTA is forced as disabled.
  if (forceState == OTAForceState::ALWAYS_DISABLE) {
    DEBUG_PRINTLN("[OTA] FORCE DISABLED.");
    isInitialized = false;
    return false;
  }

  // Check enable pin if configured and if we don't want to always force enable.
  if (forceState == OTAForceState::AUTO && enablePin >= 0) {
    DEBUG_PRINTLN("[OTA] ENABLED BY AUTO CHECK.");
    pinMode(enablePin, INPUT_PULLUP);
    if (digitalRead(enablePin) != LOW) {
      // Button not pressed during boot
      DEBUG_PRINTLN("[OTA] Enable button not pressed, OTA disabled");
      isInitialized = false;
      return false;
    }
    DEBUG_PRINTLN("[OTA] Enable button detected, starting OTA...");
  }

  if (forceState == OTAForceState::ALWAYS_ENABLE) {
    DEBUG_PRINTLN("[OTA] FORCE ENABLE.");
  }

  DEBUG_PRINTLN("\n=== OTA Manager Initialization ===");

  // Configure and start Access Point
  DEBUG_PRINT("Creating WiFi Access Point: ");
  DEBUG_PRINTLN(apSSID.c_str());

  bool apStarted = false;
  if (apPassword.length() > 0) {
    // Secured AP (password must be at least 8 characters)
    if (apPassword.length() < 8) {
      DEBUG_PRINTLN("[ERROR] AP password must be at least 8 characters!");
      return false;
    }
    apStarted = WiFi.softAP(apSSID.c_str(), apPassword.c_str());
  } else {
    // Open AP (no password)
    DEBUG_PRINTLN("[WARNING] Creating OPEN WiFi network (no password)");
    apStarted = WiFi.softAP(apSSID.c_str());
  }

  if (!apStarted) {
    DEBUG_PRINTLN("[ERROR] Failed to start Access Point!");
    return false;
  }

  // Wait for AP to be ready
  delay(100);

  // Get and display IP address
  IPAddress IP = WiFi.softAPIP();
  DEBUG_PRINT("Access Point IP: ");
  DEBUG_PRINTLN(IP.toString().c_str());

  // Initialize ElegantOTA with optional authentication
  if (strlen(otaUser) > 0 && strlen(otaPass) > 0) {
    DEBUG_PRINT("OTA Authentication: Enabled (user: ");
    DEBUG_PRINT(otaUser);
    DEBUG_PRINTLN(")");
    ElegantOTA.begin(&server, otaUser, otaPass);
  } else {
    DEBUG_PRINTLN("OTA Authentication: Disabled (open access)");
    ElegantOTA.begin(&server);
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

  // Start web server
  server.begin();
  DEBUG_PRINTLN("Web server started on port 80");
  DEBUG_PRINTLN("\nOTA Update Interface:");
  DEBUG_PRINT("  URL: http://");
  DEBUG_PRINT(IP.toString().c_str());
  DEBUG_PRINTLN("/update");
  DEBUG_PRINTLN("\n=== OTA Manager Ready ===\n");

  isInitialized = true;
  return true;
}

// Handle OTA requests (call in loop())
void OTAManager::handle() {
  if (!isInitialized) {
    return;
  }

  server.handleClient();
  ElegantOTA.loop();
}

// Check if OTA is running
bool OTAManager::isRunning() const {
  return isInitialized;
}

// Get AP IP address
IPAddress OTAManager::getIP() const {
  return WiFi.softAPIP();
}

#endif  // ENABLE_OTA
