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
      mdnsInitialized(false),
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
                           uint8_t connectTimeout, uint16_t portalTimeout, bool resetCredentials,
                           bool forcePortal) {
  DEBUG_PRINTLN("\n[Network] Initializing NetworkManager...");

  // Store configuration
  this->apName = apName;

  // Setup WiFi connection
  setupWiFi(connectTimeout, portalTimeout, resetCredentials, forcePortal);

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
void NetworkManager::setupWiFi(uint8_t connectTimeout, uint16_t portalTimeout, bool resetCredentials,
                                bool forcePortal) {
  DEBUG_PRINTLN("[WiFi] Configuring WiFiManager...");

  // Reset saved WiFi credentials if requested
  if (resetCredentials) {
    DEBUG_PRINTLN("[WiFi] Resetting saved WiFi credentials...");
    wifiManager.resetSettings();
    DEBUG_PRINTLN("[WiFi] Credentials cleared - will start in AP mode");
  }

  // Configure AP callback for status updates
  wifiManager.setAPCallback([this](WiFiManager* myWiFiManager) {
    DEBUG_PRINTLN("[WiFi] Entered config portal mode");
    DEBUG_PRINT("[WiFi] AP Name: ");
    DEBUG_PRINTLN(myWiFiManager->getConfigPortalSSID());
    DEBUG_PRINT("[WiFi] AP IP: ");
    DEBUG_PRINTLN(WiFi.softAPIP());
    DEBUG_PRINTLN("[WiFi] Connect to this AP to configure WiFi");
  });

  if (forcePortal) {
    // FORCE PORTAL MODE: Button pressed during boot
    // Show captive portal for WiFi configuration with button exit option
    DEBUG_PRINTLN("[WiFi] Force portal mode - button pressed during boot");
    DEBUG_PRINTLN("[WiFi] Starting non-blocking captive portal...");

    // Update display to show WiFi config message (Option B)
    if (display) {
      Adafruit_SSD1306& oled = display->getDisplay();
      oled.clearDisplay();
      oled.setTextSize(1);
      oled.setTextColor(SSD1306_WHITE);
      oled.setCursor(0, 0);
      oled.println("WiFi Setup...");
      oled.println("");
      oled.println("Connect to:");
      oled.println("192.168.4.1");
      oled.println("");
      oled.println("Push btn to exit");
      oled.println("and start local AP");
      oled.println("for Web UI");
      oled.display();
    }

    // Configure non-blocking portal
    wifiManager.setConfigPortalBlocking(false);
    wifiManager.setConfigPortalTimeout(0);  // No automatic timeout
    wifiManager.setConnectTimeout(connectTimeout);

    // Start the config portal
    wifiManager.startConfigPortal(apName.c_str());

    // Monitor loop - check button and portal status
    unsigned long startTime = millis();
    unsigned long maxDuration = 300000;  // 5 minutes maximum
    bool portalActive = true;

    DEBUG_PRINTLN("[WiFi] Portal active - press button to exit");

    while (portalActive && (millis() - startTime < maxDuration)) {
      // Process WiFiManager events
      wifiManager.process();

      // Check if WiFi connected (user configured via portal)
      if (WiFi.isConnected()) {
        DEBUG_PRINTLN("[WiFi] WiFi configured via portal");
        portalActive = false;
        break;
      }

      // Check button state (via Theremin instance)
      if (theremin && theremin->getMCP().digitalRead(PIN_MULTI_BUTTON) == LOW) {
        DEBUG_PRINTLN("[WiFi] Button pressed - exiting portal");
        wifiManager.stopConfigPortal();
        portalActive = false;
        delay(500);  // Debounce
        break;
      }

      delay(100);  // Check every 100ms
    }

    // Check for timeout
    if (millis() - startTime >= maxDuration) {
      DEBUG_PRINTLN("[WiFi] Config portal timed out after 5 minutes");
      wifiManager.stopConfigPortal();
    }

    // Restore loading screen
    if (display) {
      display->showLoadingScreen();
    }

    // If not connected, start AP mode
    if (!WiFi.isConnected()) {
      DEBUG_PRINTLN("[WiFi] Starting AP mode without portal...");
      WiFi.mode(WIFI_AP);
      WiFi.softAP(apName.c_str());
    }
  } else {
    // NORMAL MODE: Non-blocking WiFi setup
    // Try to connect, fallback to AP mode without blocking portal
    DEBUG_PRINTLN("[WiFi] Normal mode - non-blocking WiFi setup");

    wifiManager.setConnectTimeout(connectTimeout);
    wifiManager.setConfigPortalTimeout(0);  // No portal timeout

    DEBUG_PRINT("[WiFi] Attempting to connect");
    if (connectTimeout > 0) {
      DEBUG_PRINT(" (timeout: ");
      DEBUG_PRINT(connectTimeout);
      DEBUG_PRINT("s)");
    }
    DEBUG_PRINTLN("...");

    if (!wifiManager.autoConnect(apName.c_str())) {
      DEBUG_PRINTLN("[WiFi] Failed to connect to WiFi");
      DEBUG_PRINTLN("[WiFi] Starting AP mode for web access...");

      // Start AP mode without captive portal
      WiFi.mode(WIFI_AP);
      WiFi.softAP(apName.c_str());
    }
  }

  // Print connection status
  if (WiFi.isConnected()) {
    // Disable WiFi sleep mode for reliable server operation
    // This prevents mDNS timeouts and WebSocket disconnections
    // Trade-off: ~30-50mA higher power consumption (acceptable for powered device)
    WiFi.setSleep(WIFI_PS_NONE);
    DEBUG_PRINTLN("[WiFi] WiFi sleep mode disabled for server operation");

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
    mdnsInitialized = true;
    DEBUG_PRINT("[mDNS] Accessible at http://");
    DEBUG_PRINT(hostname);
    DEBUG_PRINTLN(".local");
  }
  else {
    mdnsInitialized = false;
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

  // Note: ESP32's mDNS is handled automatically by FreeRTOS tasks
  // No manual update() call needed (unlike ESP8266)
  // The mdnsInitialized flag is still useful for tracking state

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
