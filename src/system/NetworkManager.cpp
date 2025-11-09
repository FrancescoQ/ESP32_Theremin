/*
 * NetworkManager.cpp
 *
 * Implementation of NetworkManager class.
 * Manages WiFi connection, OTA updates, and web server.
 */

#include "system/NetworkManager.h"

#ifdef ENABLE_NETWORK

  #include "system/Debug.h"
  #include "system/Theremin.h"
  #include <LittleFS.h>

// Constructor
NetworkManager::NetworkManager(DisplayManager* disp, Theremin* thmn)
    : server(80),
      ota(&server),  // OTAManager only needs server reference (WiFi handled by WiFiManager)
      webUI(nullptr),  // Created later after Theremin is available
      display(disp),
      theremin(thmn),
      isInitialized(false),
      apName("Theremin-Setup"),
      mdnsHostname("theremin") {
  // Register network status display page using lambda (consistent with Theremin pattern)
  if (display) {
    display->registerPage(
        "Network", [this](Adafruit_SSD1306& oled) { this->renderNetworkPage(oled); }, "Network",
        90);
  }
}

// Destructor
NetworkManager::~NetworkManager() {
  if (webUI) {
    delete webUI;
    webUI = nullptr;
  }
}

// Set Theremin instance
void NetworkManager::setTheremin(Theremin* thmn) {
  theremin = thmn;
}

// Initialize all network services
bool NetworkManager::begin(const char* apName, const char* otaUser, const char* otaPass,
                           uint8_t connectTimeout, uint16_t portalTimeout, bool resetCredentials) {
  DEBUG_PRINTLN("\n[Network] Initializing NetworkManager...");

  // Store configuration
  this->apName = apName;

  // Setup WiFi connection
  setupWiFi(connectTimeout, portalTimeout, resetCredentials);

  // Setup mDNS if connected to WiFi
  if (WiFi.isConnected()) {
    setupMDNS(mdnsHostname.c_str());
  }

  // Setup OTA
  setupOTA(otaUser, otaPass);

  // Initialize WebUI if Theremin is available
  if (theremin) {
    DEBUG_PRINTLN("[Network] Initializing WebUI...");
    webUI = new WebUIManager(&server, theremin);
    webUI->begin();
  } else {
    DEBUG_PRINTLN("[Network] WARNING: Theremin not set, WebUI disabled");
    DEBUG_PRINTLN("[Network] Call setTheremin() before begin() to enable WebUI");
  }

  // Setup static file serving
  setupStaticFiles();

  // Start the web server
  server.begin();
  DEBUG_PRINTLN("[Network] AsyncWebServer started on port 80");

  isInitialized = true;
  DEBUG_PRINTLN("[Network] NetworkManager initialized successfully\n");

  return true;
}

// Setup WiFi connection with WiFiManager
void NetworkManager::setupWiFi(uint8_t connectTimeout, uint16_t portalTimeout, bool resetCredentials) {
  DEBUG_PRINTLN("[WiFi] Configuring WiFiManager...");

  // Reset saved WiFi credentials if requested
  if (resetCredentials) {
    DEBUG_PRINTLN("[WiFi] Resetting saved WiFi credentials...");
    wifiManager.resetSettings();
    DEBUG_PRINTLN("[WiFi] Credentials cleared - will start in AP mode");
  }

  // Configure timeouts
  wifiManager.setConnectTimeout(connectTimeout);
  wifiManager.setConfigPortalTimeout(portalTimeout);

  // Configure AP callback for status updates
  wifiManager.setAPCallback([](WiFiManager* myWiFiManager) {
    DEBUG_PRINTLN("[WiFi] Entered config portal mode");
    DEBUG_PRINT("[WiFi] AP Name: ");
    DEBUG_PRINTLN(myWiFiManager->getConfigPortalSSID());
    DEBUG_PRINT("[WiFi] AP IP: ");
    DEBUG_PRINTLN(WiFi.softAPIP());
    DEBUG_PRINTLN("[WiFi] Connect to this AP to configure WiFi");
  });

  // Attempt to auto-connect to saved WiFi or start config portal
  DEBUG_PRINT("[WiFi] Attempting to connect");
  if (connectTimeout > 0) {
    DEBUG_PRINT(" (timeout: ");
    DEBUG_PRINT(connectTimeout);
    DEBUG_PRINT("s)");
  }
  DEBUG_PRINTLN("...");

  if (!wifiManager.autoConnect(apName.c_str())) {
    DEBUG_PRINTLN("[WiFi] Failed to connect to WiFi");
    // With portalTimeout=0, this should not happen
    // But if it does, we continue (AP mode is running)
  }

  // Print connection status
  if (WiFi.isConnected()) {
    DEBUG_PRINTLN("[WiFi] Connected to WiFi (STA mode)");
    DEBUG_PRINT("[WiFi] SSID: ");
    DEBUG_PRINTLN(WiFi.SSID());
    DEBUG_PRINT("[WiFi] IP: ");
    DEBUG_PRINTLN(WiFi.localIP());
    DEBUG_PRINT("[WiFi] Signal: ");
    DEBUG_PRINT(WiFi.RSSI());
    DEBUG_PRINTLN(" dBm");
  }
  else {
    DEBUG_PRINTLN("[WiFi] Running in Access Point mode");
    DEBUG_PRINT("[WiFi] AP Name: ");
    DEBUG_PRINTLN(apName);
    DEBUG_PRINT("[WiFi] AP IP: ");
    DEBUG_PRINTLN(WiFi.softAPIP());
    DEBUG_PRINTLN("[WiFi] Connect to this AP to access the web interface");
  }
}

// Setup mDNS service
void NetworkManager::setupMDNS(const char* hostname) {
  DEBUG_PRINT("[mDNS] Registering hostname: ");
  DEBUG_PRINT(hostname);
  DEBUG_PRINTLN(".local");

  if (MDNS.begin(hostname)) {
    MDNS.addService("http", "tcp", 80);
    DEBUG_PRINT("[mDNS] Accessible at http://");
    DEBUG_PRINT(hostname);
    DEBUG_PRINTLN(".local");
  }
  else {
    DEBUG_PRINTLN("[mDNS] Failed to start mDNS service");
  }
}

// Setup OTA updates
void NetworkManager::setupOTA(const char* user, const char* pass) {
  // OTAManager registers ElegantOTA on the shared AsyncWebServer
  // WiFi/AP connection is already handled by WiFiManager
  if (ota.begin(user, pass)) {
    DEBUG_PRINTLN("[OTA] OTA registered successfully");
  } else {
    DEBUG_PRINTLN("[OTA] Failed to register OTA");
  }
}

// Non-blocking update
void NetworkManager::update() {
  if (!isInitialized) {
    return;
  }

  // Update WebUI (periodic state broadcasts)
  if (webUI) {
    webUI->update();
  }

  // Future: Add reconnect logic, WiFi monitoring, etc.
}

// Setup static file serving from LittleFS
// use
//   pio run -t uploadfs
// to upload the data folder
void NetworkManager::setupStaticFiles() {
  DEBUG_PRINTLN("[Network] Setting up static file serving...");

  // Initialize LittleFS
  if (!LittleFS.begin(false)) {  // false = don't format on fail
    DEBUG_PRINTLN("[Network] WARNING: LittleFS mount failed");
    DEBUG_PRINTLN("[Network] Run 'pio run -t uploadfs' to upload filesystem");

    // Serve a simple fallback page
    this->server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
      request->send(200, "text/html",
                    "<html><body><h1>Theremin WebUI</h1>"
                    "<p>Filesystem not available. Upload with: pio run -t uploadfs</p>"
                    "<p>WebSocket endpoint: ws://" +
                        request->host() + "/ws</p></body></html>");
    });
    return;
  }

  DEBUG_PRINTLN("[Network] LittleFS mounted successfully");

  // Serve all static files from root directory
  // Automatically handles MIME types, subdirectories, and caching
  this->server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

  DEBUG_PRINTLN("[Network] Static file serving enabled");
}

// Check if connected to WiFi
bool NetworkManager::isConnected() const {
  return WiFi.isConnected();
}

// Get current IP address
IPAddress NetworkManager::getIP() const {
  if (WiFi.isConnected()) {
    return WiFi.localIP();
  }
  else {
    return WiFi.softAPIP();
  }
}

// Get current WiFi mode
String NetworkManager::getMode() const {
  if (!isInitialized) {
    return "Off";
  }

  if (WiFi.isConnected()) {
    return "STA";
  }
  else if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
    return "AP";
  }
  else {
    return "Off";
  }
}

// Get connected WiFi SSID
String NetworkManager::getSSID() const {
  if (WiFi.isConnected()) {
    return WiFi.SSID();
  }
  return "N/A";
}

// Get WiFi signal strength
int8_t NetworkManager::getRSSI() const {
  if (WiFi.isConnected()) {
    return WiFi.RSSI();
  }
  return 0;
}

// Render network status display page
void NetworkManager::renderNetworkPage(Adafruit_SSD1306& display) {
  // Mode and connection status
  String mode = getMode();

  display.setCursor(0, DisplayManager::CONTENT_START_Y);
  display.print("Mode: ");
  display.println(mode);

  if (mode == "STA") {
    // Connected to WiFi - show network details
    display.print("SSID: ");
    String ssid = getSSID();
    if (ssid.length() > 14) {
      ssid = ssid.substring(0, 14);  // Truncate if too long
    }
    display.println(ssid);

    display.print("IP: ");
    display.println(getIP().toString());

    display.print("Signal: ");
    display.print(getRSSI());
    display.println(" dBm");

    display.println("Access:");
    display.print("  ");
    display.print(mdnsHostname);
    display.println(".local");
  }
  else if (mode == "AP") {
    // Access Point mode - show AP details
    display.print("AP: ");
    String ap = apName;
    if (ap.length() > 16) {
      ap = ap.substring(0, 16);  // Truncate if too long
    }
    display.println(ap);

    display.print("IP: ");
    display.println(getIP().toString());

    display.println("");
    display.println("Connect to AP");
    display.println("to configure");
  }
  else {
    // Network disabled
    display.println("");
    display.println("Network");
    display.println("disabled");
  }
}

#endif  // ENABLE_NETWORK
