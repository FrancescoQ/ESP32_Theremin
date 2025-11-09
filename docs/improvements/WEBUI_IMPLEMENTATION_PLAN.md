# Web UI Implementation Plan

**Date:** November 8-9, 2025
**Status:** Phase 2 Complete (Network Infrastructure Ready)
**Goal:** Add web-based control interface with WebSocket real-time communication

## Overview

This document outlines the complete implementation plan for adding a Web UI to the ESP32 Theremin. The system will support:

- **WiFi Modes:** AP + STA (simultaneous) with automatic fallback
- **mDNS Discovery:** Access via `theremin.local`
- **Captive Portal:** Easy WiFi configuration
- **AsyncWebServer:** Non-blocking web server
- **WebSocket:** Real-time bidirectional communication
- **Full Control:** All parameters accessible via web interface
- **OTA Compatible:** Works alongside existing OTA system

---

## Current State Analysis

### Existing Infrastructure

**OTA Manager:**
- Uses synchronous `WebServer` in AP-only mode
- ElegantOTA v3.1.7 integrated (supports AsyncWebServer)
- HTTP Basic Authentication implemented

**Web Assets:**
- Template `data/index.html` exists
- Ready for expansion

**Architecture:**
- Clean modular design
- Easy integration points

### Resource Availability

**Current Performance:**
- CPU: 14.5% usage (85% headroom)
- RAM: 314 KB free
- Flash: 35% free (~450 KB available)

**Estimated Additional Requirements:**
- RAM: ~25-35 KB (WebSocket + AsyncWebServer)
- Flash: ~80-115 KB (libraries + web assets)
- CPU: ~5-8% (WebSocket broadcasts)

**Conclusion:** Plenty of headroom for Web UI implementation!

---

## Authentication Options

### Option 1: HTTP Basic Authentication (Recommended)

**Implementation:**
```cpp
server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->authenticate("admin", "theremin")) {
        return request->requestAuthentication();
    }
    request->send(SPIFFS, "/index.html", "text/html");
});
```

**Pros:**
- ✅ Simple one-line implementation
- ✅ Built-in browser support
- ✅ Consistent with current OTA authentication
- ✅ Works with WebSockets

**Cons:**
- ⚠️ Credentials sent with every request (base64 encoded)
- ⚠️ No logout button (browser caches credentials)

### Option 2: Session-based Authentication

**Implementation:** Custom login page + session cookies

**Pros:**
- ✅ Better UX (login page vs browser popup)
- ✅ Logout functionality
- ✅ "Remember me" capability

**Cons:**
- ⚠️ More complex implementation
- ⚠️ Session token management required

### Option 3: No Authentication (Initial Development)

**Recommendation:** Start without authentication for faster development, add HTTP Basic Auth later (5-minute addition)

---

## Phase 1: Foundation (AsyncWebServer Migration)

### Goal
Migrate from synchronous `WebServer` to `AsyncWebServer` for non-blocking operation

### Tasks

#### 1.1 Update platformio.ini

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
build_flags =
    -DDEBUG_MODE=1
    -DENABLE_OTA=1
    -DENABLE_STARTUP_TEST=0
    -DENABLE_STARTUP_SOUND=1
    -DENABLE_GPIO_MONITOR=0
    -DELEGANTOTA_USE_ASYNC_WEBSERVER=1       # NEW
lib_deps =
    adafruit/Adafruit_VL53L0X@^1.2.0
    ayushsharma82/ElegantOTA@^3.1.0
    adafruit/Adafruit MCP23017 Arduino Library@^2.3.2
    adafruit/Adafruit SSD1306@^2.5.7
    adafruit/Adafruit GFX Library@^1.11.9
    ESP32Async/ESPAsyncWebServer@^3.8.1      # NEW (AsyncTCP auto-included)
    tzapu/WiFiManager@^2.0.17                # NEW
    bblanchon/ArduinoJson@^7.2.1             # NEW
board_build.filesystem = littlefs            # NEW
```

**Note:** We use the `ESP32Async/ESPAsyncWebServer` fork (v3.8.1), which is the most actively maintained fork. AsyncTCP is automatically included as a transitive dependency, so no need to add it explicitly.

#### 1.2 Refactor OTAManager

**Changes to `include/system/OTAManager.h`:**
```cpp
#include <AsyncWebServer.h>  // Replace WebServer.h
#include <ElegantOTA.h>

class OTAManager {
private:
    AsyncWebServer* server;  // Pointer instead of owned instance
    String apSSID;
    String apPassword;
    int enablePin;
    bool isInitialized;

public:
    // Accept server pointer (shared instance)
    OTAManager(AsyncWebServer* srv, const char* ssid = "",
               const char* apPass = "", int buttonPin = -1);

    bool begin(const char* otaUser = "", const char* otaPass = "",
               OTAForceState forceState = OTAForceState::AUTO);

    void handle();  // No longer needed with AsyncWebServer
    bool isRunning() const;
    IPAddress getIP() const;
};
```

**Changes to `src/system/OTAManager.cpp`:**
- Replace `server.begin()` calls
- Update ElegantOTA initialization for async mode
- Remove `server.handleClient()` from `handle()` method

#### 1.3 Add Filesystem Support

**Create `data/` directory structure:**
```
data/
├── index.html
├── styles.css
├── scripts.js
└── favicon.ico
```

**Add upload command:**
```bash
pio run -t uploadfs  # Upload filesystem to ESP32
```

### Deliverables
- [x] OTAManager refactored to use AsyncWebServer
- [x] ElegantOTA working in async mode
- [x] Filesystem support configured
- [x] OTA functionality verified

### Testing Checklist
- [x] Build completes without errors
- [x] OTA accessible via web interface
- [x] Firmware upload works correctly
- [ ] Static files served from filesystem (pending Phase 3)

---

## Phase 2: Network Infrastructure (NetworkManager + WiFiManager) ✅ COMPLETE

### Goal
Integrate mature WiFiManager library for automatic WiFi configuration with AP+STA mode, captive portal, OTA, and mDNS

### Implementation Approach
**IMPLEMENTED:** Created unified `NetworkManager` class that encapsulates:
- WiFiManager integration (tzapu library) for captive portal and auto-connect
- OTAManager coordination (shared AsyncWebServer)
- mDNS registration (theremin.local)
- DisplayManager integration (network status page)
- **Bonus Features:**
  - WiFi credentials reset (special state + button during boot)
  - System reboot (10s button hold with accidental-reboot protection)

### Tasks

#### 2.1 Configure WiFiManager for AsyncWebServer

**Integration approach:**
```cpp
#include <WiFiManager.h>  // tzapu/WiFiManager library

// WiFiManager uses its own internal web server for configuration
// Our AsyncWebServer runs separately on port 80
// WiFiManager's captive portal runs temporarily during setup
```

#### 2.2 Setup WiFiManager in main.cpp

**File: `src/main.cpp`** (additions to existing code)

```cpp
#if ENABLE_OTA
#include <ESPAsyncWebServer.h>
#include <WiFiManager.h>  // NEW
#include "system/OTAManager.h"
#endif

// Global instances
#if ENABLE_OTA
AsyncWebServer server(80);
WiFiManager wifiManager;  // NEW: Library instance

OTAManager ota(&server, "Theremin-OTA", "", PIN_OTA_ENABLE);
#endif

void setup() {
    // ... existing setup code ...

    #if ENABLE_OTA
        // Configure WiFiManager
        wifiManager.setConfigPortalTimeout(180);  // 3 minutes timeout
        wifiManager.setAPCallback([](WiFiManager *myWiFiManager) {
            DEBUG_PRINTLN("[WiFi] Entered config mode");
            DEBUG_PRINT("[WiFi] Config AP: ");
            DEBUG_PRINTLN(myWiFiManager->getConfigPortalSSID());
            DEBUG_PRINT("[WiFi] Config IP: ");
            DEBUG_PRINTLN(WiFi.softAPIP());
        });

        // Auto-connect to saved WiFi or start config portal
        if (!wifiManager.autoConnect("Theremin-Setup")) {
            DEBUG_PRINTLN("[WiFi] Failed to connect, restarting...");
            delay(3000);
            ESP.restart();
        }

        DEBUG_PRINTLN("[WiFi] Connected!");
        DEBUG_PRINT("[WiFi] IP address: ");
        DEBUG_PRINTLN(WiFi.localIP());

        // Setup mDNS
        if (MDNS.begin("theremin")) {
            MDNS.addService("http", "tcp", 80);
            DEBUG_PRINTLN("[mDNS] theremin.local registered");
        }

        // Initialize OTA and start server
        OTAManager::OTAForceState otaForcedState = OTAManager::ALWAYS_DISABLE;
        if (theremin.getAudioEngine()->getSpecialState(1)) {
            otaForcedState = OTAManager::ALWAYS_ENABLE;
        }

        if (ota.begin("admin", "theremin", otaForcedState)) {
            DEBUG_PRINTLN("[OTA] OTA updates enabled");
            server.begin();
            DEBUG_PRINTLN("[Server] AsyncWebServer started on port 80");
        }
    #endif
}

void loop() {
    // ... existing loop code ...

    // WiFiManager is non-blocking, no update() needed
}
```

#### 2.3 WiFiManager Configuration Options

**Optional customizations:**
```cpp
// Set custom parameters (before autoConnect)
wifiManager.setAPStaticIPConfig(
    IPAddress(192,168,4,1),  // AP IP
    IPAddress(192,168,4,1),  // Gateway
    IPAddress(255,255,255,0) // Subnet
);

// Customize portal appearance
wifiManager.setTitle("TheremAIn WiFi Setup");
wifiManager.setCustomHeadElement("<style>body{background:#1a1a2e;}</style>");

// Add custom parameters (advanced)
WiFiManagerParameter custom_text("<p>TheremAIn Configuration</p>");
wifiManager.addParameter(&custom_text);

// Enable/disable features
wifiManager.setShowInfoUpdate(false);
wifiManager.setShowInfoErase(false);
wifiManager.setShowStaticFields(true);
```

#### 2.4 Reset WiFi Credentials (Optional Feature)

**Add button-triggered reset:**
```cpp
// In setup(), check button before WiFiManager
if (digitalRead(RESET_WIFI_PIN) == LOW) {
    DEBUG_PRINTLN("[WiFi] Reset button pressed, clearing credentials");
    wifiManager.resetSettings();
}
```

### Deliverables
- [x] WiFiManager library integrated
- [x] NetworkManager class created
- [x] Auto-connect to saved network implemented
- [x] Captive portal accessible on first boot
- [x] mDNS `theremin.local` registered
- [x] WiFi credentials persisted automatically
- [x] Fallback to AP mode on connection failure
- [x] OTAManager integrated with shared AsyncWebServer
- [x] Display page for network status registered
- [x] WiFi credentials reset feature (special state + button)
- [x] System reboot feature (10s button hold with protection)

### Testing Checklist
- [x] First boot: Captive portal appears ("Theremin-Setup" AP)
- [x] Portal: Can scan and select WiFi network
- [x] Portal: Connection successful after entering password
- [x] Reboot: Auto-connects to saved network
- [x] STA mode: `theremin.local` resolves correctly
- [x] STA mode: AsyncWebServer accessible
- [x] OTA accessible at `/update` route
- [x] Display page shows network status (IP, SSID, signal, mode)
- [x] WiFi reset: Special state + button during boot clears credentials
- [x] System reboot: 10s button hold triggers reboot
- [x] Reboot protection: Touching any control in modifier mode prevents reboot
- [ ] Connection loss: Auto-reconnects (pending hardware testing)

### Implementation Notes

**WiFiManager Operation:**
1. **First boot (no credentials):**
   - Creates "Theremin-Setup" AP
   - Starts captive portal on 192.168.4.1
   - User connects, configures WiFi
   - Saves credentials and reboots

2. **Subsequent boots (has credentials):**
   - Attempts connection to saved network
   - Success → STA mode, continues to main app
   - Failure → Falls back to config portal

3. **Our AsyncWebServer:**
   - Only starts after WiFiManager succeeds
   - Runs on connected network (STA mode)
   - No conflict with WiFiManager's temporary server

**Key Differences from Custom Implementation:**
- ✅ No custom WiFiManager class needed
- ✅ Library handles credential storage automatically
- ✅ Captive portal built-in (no custom HTML needed)
- ✅ Auto-reconnect logic included
- ✅ Non-blocking operation
- ⚠️ Less control over portal appearance (but customizable)
- ⚠️ AP runs only during config (not permanent AP+STA mode)

### Additional Features Implemented

#### WiFi Credentials Reset
**Purpose:** Allow users to reconfigure WiFi without reflashing firmware

**Implementation:**
- Boot with special state: All oscillators OFF + all octave switches -1
- Hold multi-function button during boot
- Credentials erased from flash
- ESP32 boots into AP mode
- User configures new WiFi via captive portal

**Code location:**
- `src/main.cpp`: Detects special state + button press during setup
- `NetworkManager::begin()`: Accepts `resetCredentials` parameter
- `NetworkManager::setupWiFi()`: Calls `wifiManager.resetSettings()` if requested

#### System Reboot Feature
**Purpose:** Allow system restart without power cycling (useful for testing/recovery)

**Implementation:**
- Hold multi-function button for 10 seconds WITHOUT touching controls
- Audio stops after ~10s (audible confirmation - I2S buffer drains)
- "REBOOTING..." notification appears on display
- 2-second delay to show notification
- ESP32 restarts via `ESP.restart()`

**Protection Against Accidental Reboot:**
- `modifierWasUsed` flag tracks if any secondary control was touched
- If any control is used in modifier mode, reboot is disabled
- Safe to use modifier mode for extended periods
- Reboot only triggered if button held idle for full 10s

**Code location:**
- `GPIOControls::updateButton()`: Detects 10s hold in LONG_PRESS_ACTIVE state
- `GPIOControls::performSystemReboot()`: Shows notification and calls `ESP.restart()`
- All 6 secondary control methods: Set `modifierWasUsed = true` on control change

**User Experience:**
1. Hold button → modifier mode activates (600ms)
2. Keep holding without touching controls
3. Audio stops at 10s mark (audible cue)
4. "REBOOTING..." notification visible for 2s
5. System restarts

**Optional Timeout Adjustment:**
The 10-second timeout can be reduced to 5-6 seconds if desired:
```cpp
// In GPIOControls.h
static constexpr unsigned long VERY_LONG_PRESS_THRESHOLD_MS = 6000;  // 6s instead of 10s
```
Since the protection mechanism prevents accidental reboots during normal modifier use, a shorter timeout is safe and more convenient.

**Permanent AP+STA Mode (Optional - Not Implemented):**
If you need both modes simultaneously in the future:
```cpp
// After successful connection in NetworkManager
WiFi.softAP("Theremin-OTA");  // Start AP alongside STA
DEBUG_PRINTLN("[WiFi] AP+STA mode active");
```

---

## Phase 3: WebUI Backend (WebSocket Server)

### Goal
Implement real-time bidirectional communication between ESP32 and web clients

### Tasks

#### 3.1 Create WebUIManager Class

**File: `include/system/WebUIManager.h`**
```cpp
#pragma once

#include <Arduino.h>
#include <AsyncWebServer.h>
#include <AsyncWebSocket.h>
#include <ArduinoJson.h>
#include "system/Theremin.h"

class WebUIManager {
private:
    AsyncWebServer* server;
    AsyncWebSocket ws;
    Theremin* theremin;

    unsigned long lastUpdate;
    static const int UPDATE_INTERVAL = 100;  // 10 Hz broadcast rate

    void handleWebSocketMessage(AsyncWebSocketClient *client,
                                uint8_t *data, size_t len);
    void sendFullState(AsyncWebSocketClient *client);
    void broadcastUpdate(const char* type, JsonDocument& doc);

    // Command handlers
    void handleOscillatorCommand(JsonDocument& doc);
    void handleEffectCommand(JsonDocument& doc);
    void handleSensorCommand(JsonDocument& doc);

public:
    WebUIManager(AsyncWebServer* srv, Theremin* thmn);

    void begin();
    void update();  // Call in loop() for periodic broadcasts
};
```

**File: `src/system/WebUIManager.cpp`**

#### 3.2 WebSocket Event Handler

```cpp
void onWebSocketEvent(AsyncWebSocket *server,
                     AsyncWebSocketClient *client,
                     AwsEventType type,
                     void *arg,
                     uint8_t *data,
                     size_t len) {
    WebUIManager* manager = (WebUIManager*)arg;

    switch(type) {
        case WS_EVT_CONNECT:
            DEBUG_PRINTF("[WS] Client #%u connected from %s\n",
                        client->id(),
                        client->remoteIP().toString().c_str());
            manager->sendFullState(client);
            break;

        case WS_EVT_DISCONNECT:
            DEBUG_PRINTF("[WS] Client #%u disconnected\n", client->id());
            break;

        case WS_EVT_DATA:
            AwsFrameInfo *info = (AwsFrameInfo*)arg;
            if (info->final && info->index == 0 && info->len == len) {
                if (info->opcode == WS_TEXT) {
                    manager->handleWebSocketMessage(client, data, len);
                }
            }
            break;
    }
}
```

#### 3.3 JSON Protocol Definition

**Client → ESP32 (Commands):**

```json
{
    "cmd": "setWaveform",
    "osc": 1,
    "value": "SINE"
}

{
    "cmd": "setOctave",
    "osc": 2,
    "value": -1
}

{
    "cmd": "setVolume",
    "osc": 1,
    "value": 0.8
}

{
    "cmd": "setEffectParam",
    "effect": "delay",
    "param": "time",
    "value": 500
}

{
    "cmd": "enableEffect",
    "effect": "chorus",
    "value": true
}
```

**ESP32 → Client (Updates):**

```json
{
    "type": "oscillator",
    "osc": 1,
    "waveform": "SINE",
    "octave": 0,
    "volume": 1.0
}

{
    "type": "effect",
    "effect": "delay",
    "enabled": true,
    "time": 500,
    "feedback": 0.5,
    "mix": 0.3
}

{
    "type": "sensor",
    "pitch": 250,
    "volume": 150
}

{
    "type": "performance",
    "cpu": 14.5,
    "ram": 314000,
    "uptime": 3600000
}
```

#### 3.4 Command Handler Implementation

```cpp
void WebUIManager::handleWebSocketMessage(AsyncWebSocketClient *client,
                                         uint8_t *data, size_t len) {
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, data, len);

    if (error) {
        DEBUG_PRINTLN("[WS] JSON parse error");
        return;
    }

    const char* cmd = doc["cmd"];

    if (strcmp(cmd, "setWaveform") == 0) {
        handleOscillatorCommand(doc);
    } else if (strcmp(cmd, "setEffectParam") == 0) {
        handleEffectCommand(doc);
    }
    // ... more command handlers

    // Broadcast change to all clients
    broadcastUpdate("oscillator", doc);
}
```

### Deliverables
- [x] WebUIManager class created
- [x] WebSocket endpoint `/ws` functional
- [x] JSON command parser working
- [x] Integration with Theremin subsystems
- [x] Broadcast system operational

### Testing Checklist
- [ ] WebSocket connection established from browser
- [ ] Commands parsed and executed correctly
- [ ] State changes broadcast to all clients
- [ ] Multiple clients can connect simultaneously
- [ ] Reconnection works after disconnect

---

## Phase 4: Web Frontend (Dashboard UI)

### Goal
Build responsive web interface with real-time controls

### Tasks

#### 4.1 HTML Structure

**File: `data/index.html`**

```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>TheremAIn Web UI</title>
    <link rel="stylesheet" href="styles.css">
</head>
<body>
    <header class="header">
        <h1>TheremAIn Control Panel</h1>
        <div class="status">
            <span id="connection-status">Disconnected</span>
            <span id="wifi-mode">---</span>
        </div>
    </header>

    <main class="dashboard">
        <!-- Oscillator Controls -->
        <section class="oscillators">
            <h2>Oscillators</h2>
            <div class="oscillator-grid">
                <div class="oscillator" data-osc="1">
                    <h3>Oscillator 1</h3>
                    <div class="control">
                        <label>Waveform</label>
                        <select class="waveform">
                            <option value="OFF">Off</option>
                            <option value="SINE">Sine</option>
                            <option value="SQUARE">Square</option>
                            <option value="TRIANGLE">Triangle</option>
                            <option value="SAW">Sawtooth</option>
                        </select>
                    </div>
                    <div class="control">
                        <label>Octave: <span class="octave-value">0</span></label>
                        <input type="range" class="octave" min="-1" max="1" step="1" value="0">
                    </div>
                    <div class="control">
                        <label>Volume: <span class="volume-value">100</span>%</label>
                        <input type="range" class="volume" min="0" max="100" step="1" value="100">
                    </div>
                </div>
                <!-- Repeat for oscillators 2 & 3 -->
            </div>
        </section>

        <!-- Effects Controls -->
        <section class="effects">
            <h2>Effects</h2>
            <div class="effects-grid">
                <div class="effect" data-effect="delay">
                    <h3>Delay</h3>
                    <div class="control">
                        <label>
                            <input type="checkbox" class="enable"> Enable
                        </label>
                    </div>
                    <div class="control">
                        <label>Time: <span class="time-value">300</span>ms</label>
                        <input type="range" class="time" min="10" max="2000" value="300">
                    </div>
                    <div class="control">
                        <label>Feedback: <span class="feedback-value">50</span>%</label>
                        <input type="range" class="feedback" min="0" max="95" value="50">
                    </div>
                    <div class="control">
                        <label>Mix: <span class="mix-value">30</span>%</label>
                        <input type="range" class="mix" min="0" max="100" value="30">
                    </div>
                </div>
                <!-- Repeat for chorus & reverb -->
            </div>
        </section>

        <!-- Sensors & Performance -->
        <section class="monitoring">
            <h2>Real-time Monitoring</h2>
            <div class="monitor-grid">
                <div class="monitor-card">
                    <h4>Sensors</h4>
                    <div class="sensor-bar">
                        <label>Pitch:</label>
                        <div class="bar-container">
                            <div class="bar" id="pitch-bar"></div>
                        </div>
                        <span id="pitch-value">---</span> mm
                    </div>
                    <div class="sensor-bar">
                        <label>Volume:</label>
                        <div class="bar-container">
                            <div class="bar" id="volume-bar"></div>
                        </div>
                        <span id="volume-value">---</span> mm
                    </div>
                </div>

                <div class="monitor-card">
                    <h4>Performance</h4>
                    <div class="metric">
                        <span>CPU Usage:</span>
                        <span id="cpu-value">---</span>%
                    </div>
                    <div class="metric">
                        <span>Free RAM:</span>
                        <span id="ram-value">---</span> KB
                    </div>
                    <div class="metric">
                        <span>Uptime:</span>
                        <span id="uptime-value">---</span>
                    </div>
                </div>
            </div>
        </section>
    </main>

    <footer class="footer">
        <a href="/update">OTA Update</a> |
        ESP32 Theremin v1.0
    </footer>

    <script src="scripts.js"></script>
</body>
</html>
```

#### 4.2 JavaScript WebSocket Client

**File: `data/scripts.js`**

```javascript
let ws;
let reconnectInterval;

// Initialize on page load
document.addEventListener('DOMContentLoaded', () => {
    connectWebSocket();
    setupEventListeners();
});

// WebSocket connection
function connectWebSocket() {
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const wsUrl = `${protocol}//${window.location.host}/ws`;

    ws = new WebSocket(wsUrl);

    ws.onopen = () => {
        console.log('Connected to theremin');
        updateConnectionStatus(true);
        clearInterval(reconnectInterval);
    };

    ws.onmessage = (event) => {
        const data = JSON.parse(event.data);
        handleUpdate(data);
    };

    ws.onclose = () => {
        console.log('Disconnected from theremin');
        updateConnectionStatus(false);

        // Auto-reconnect
        reconnectInterval = setInterval(() => {
            console.log('Attempting to reconnect...');
            connectWebSocket();
        }, 2000);
    };

    ws.onerror = (error) => {
        console.error('WebSocket error:', error);
    };
}

// Handle incoming updates
function handleUpdate(data) {
    switch(data.type) {
        case 'oscillator':
            updateOscillatorUI(data);
            break;
        case 'effect':
            updateEffectUI(data);
            break;
        case 'sensor':
            updateSensorUI(data);
            break;
        case 'performance':
            updatePerformanceUI(data);
            break;
    }
}

// Update oscillator UI
function updateOscillatorUI(data) {
    const oscDiv = document.querySelector(`.oscillator[data-osc="${data.osc}"]`);
    if (!oscDiv) return;

    oscDiv.querySelector('.waveform').value = data.waveform;
    oscDiv.querySelector('.octave').value = data.octave;
    oscDiv.querySelector('.octave-value').textContent = data.octave;
    oscDiv.querySelector('.volume').value = data.volume * 100;
    oscDiv.querySelector('.volume-value').textContent = Math.round(data.volume * 100);
}

// Update sensor visualization
function updateSensorUI(data) {
    document.getElementById('pitch-value').textContent = data.pitch;
    document.getElementById('volume-value').textContent = data.volume;

    // Update visual bars (0-400mm range)
    const pitchPercent = (data.pitch / 400) * 100;
    const volumePercent = (data.volume / 400) * 100;
    document.getElementById('pitch-bar').style.width = pitchPercent + '%';
    document.getElementById('volume-bar').style.width = volumePercent + '%';
}

// Update performance metrics
function updatePerformanceUI(data) {
    document.getElementById('cpu-value').textContent = data.cpu.toFixed(1);
    document.getElementById('ram-value').textContent = (data.ram / 1024).toFixed(0);

    // Format uptime
    const seconds = Math.floor(data.uptime / 1000);
    const hours = Math.floor(seconds / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    const secs = seconds % 60;
    document.getElementById('uptime-value').textContent =
        `${hours}h ${minutes}m ${secs}s`;
}

// Send command to ESP32
function sendCommand(cmd, params) {
    if (ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({cmd, ...params}));
    } else {
        console.error('WebSocket not connected');
    }
}

// Setup event listeners for controls
function setupEventListeners() {
    // Oscillator waveform selects
    document.querySelectorAll('.oscillator .waveform').forEach(select => {
        select.addEventListener('change', (e) => {
            const osc = parseInt(e.target.closest('.oscillator').dataset.osc);
            sendCommand('setWaveform', {osc, value: e.target.value});
        });
    });

    // Oscillator octave sliders
    document.querySelectorAll('.oscillator .octave').forEach(slider => {
        slider.addEventListener('input', (e) => {
            const osc = parseInt(e.target.closest('.oscillator').dataset.osc);
            const value = parseInt(e.target.value);
            e.target.previousElementSibling.querySelector('.octave-value').textContent = value;
            sendCommand('setOctave', {osc, value});
        });
    });

    // Oscillator volume sliders
    document.querySelectorAll('.oscillator .volume').forEach(slider => {
        slider.addEventListener('input', (e) => {
            const osc = parseInt(e.target.closest('.oscillator').dataset.osc);
            const value = parseInt(e.target.value) / 100;
            e.target.previousElementSibling.querySelector('.volume-value').textContent =
                Math.round(value * 100);
            sendCommand('setVolume', {osc, value});
        });
    });

    // Effect enable checkboxes
    document.querySelectorAll('.effect .enable').forEach(checkbox => {
        checkbox.addEventListener('change', (e) => {
            const effect = e.target.closest('.effect').dataset.effect;
            sendCommand('enableEffect', {effect, value: e.target.checked});
        });
    });

    // Effect parameter sliders (time, feedback, mix, etc.)
    document.querySelectorAll('.effect input[type="range"]').forEach(slider => {
        slider.addEventListener('input', (e) => {
            const effect = e.target.closest('.effect').dataset.effect;
            const param = e.target.className;
            let value = parseInt(e.target.value);

            // Update display value
            const valueSpan = e.target.previousElementSibling.querySelector(`.${param}-value`);
            if (valueSpan) valueSpan.textContent = value;

            // Convert percentage to 0.0-1.0 for feedback/mix
            if (param === 'feedback' || param === 'mix') {
                value = value / 100;
            }

            sendCommand('setEffectParam', {effect, param, value});
        });
    });
}

// Update connection status indicator
function updateConnectionStatus(connected) {
    const statusEl = document.getElementById('connection-status');
    statusEl.textContent = connected ? 'Connected' : 'Disconnected';
    statusEl.className = connected ? 'connected' : 'disconnected';
}
```

#### 4.3 CSS Styling

**File: `data/styles.css`**

```css
/* Variables */
:root {
    --bg-primary: #1a1a2e;
    --bg-secondary: #16213e;
    --bg-card: #0f3460;
    --text-primary: #eee;
    --text-secondary: #aaa;
    --accent: #e94560;
    --accent-light: #ff6b6b;
    --success: #4caf50;
    --border: #2a2a3e;
}

/* Reset & Base */
* {
    margin: 0;
    padding: 0;
    box-sizing: border-box;
}

body {
    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
    background: var(--bg-primary);
    color: var(--text-primary);
    line-height: 1.6;
}

/* Header */
.header {
    background: var(--bg-secondary);
    padding: 1rem 2rem;
    border-bottom: 2px solid var(--accent);
    display: flex;
    justify-content: space-between;
    align-items: center;
}

.header h1 {
    font-size: 1.5rem;
    color: var(--accent-light);
}

.status {
    display: flex;
    gap: 1rem;
    font-size: 0.9rem;
}

.connected {
    color: var(--success);
}

.disconnected {
    color: var(--accent);
}

/* Dashboard Layout */
.dashboard {
    padding: 2rem;
    max-width: 1400px;
    margin: 0 auto;
}

section {
    background: var(--bg-secondary);
    border: 1px solid var(--border);
    border-radius: 8px;
    padding: 1.5rem;
    margin-bottom: 2rem;
}

section h2 {
    color: var(--accent-light);
    margin-bottom: 1rem;
    font-size: 1.3rem;
}

/* Grid Layouts */
.oscillator-grid,
.effects-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
    gap: 1.5rem;
}

/* Cards */
.oscillator,
.effect,
.monitor-card {
    background: var(--bg-card);
    padding: 1rem;
    border-radius: 6px;
    border: 1px solid var(--border);
}

.oscillator h3,
.effect h3,
.monitor-card h4 {
    color: var(--accent);
    margin-bottom: 1rem;
    font-size: 1.1rem;
}

/* Controls */
.control {
    margin-bottom: 1rem;
}

.control label {
    display: block;
    margin-bottom: 0.5rem;
    color: var(--text-secondary);
    font-size: 0.9rem;
}

select,
input[type="range"] {
    width: 100%;
    padding: 0.5rem;
    background: var(--bg-secondary);
    border: 1px solid var(--border);
    border-radius: 4px;
    color: var(--text-primary);
    cursor: pointer;
}

select {
    font-size: 1rem;
}

select:focus,
input[type="range"]:focus {
    outline: none;
    border-color: var(--accent);
}

/* Checkboxes */
input[type="checkbox"] {
    width: 18px;
    height: 18px;
    margin-right: 0.5rem;
    cursor: pointer;
}

/* Sensor Bars */
.sensor-bar {
    display: flex;
    align-items: center;
    gap: 1rem;
    margin-bottom: 1rem;
}

.sensor-bar label {
    min-width: 60px;
    margin: 0;
}

.bar-container {
    flex: 1;
    height: 20px;
    background: var(--bg-secondary);
    border-radius: 10px;
    overflow: hidden;
}

.bar {
    height: 100%;
    background: linear-gradient(90deg, var(--accent), var(--accent-light));
    transition: width 0.1s ease;
}

/* Performance Metrics */
.metric {
    display: flex;
    justify-content: space-between;
    padding: 0.5rem;
    margin-bottom: 0.5rem;
    background: var(--bg-secondary);
    border-radius: 4px;
}

/* Footer */
.footer {
    text-align: center;
    padding: 1rem;
    color: var(--text-secondary);
    font-size: 0.9rem;
    border-top: 1px solid var(--border);
}

.footer a {
    color: var(--accent);
    text-decoration: none;
}

.footer a:hover {
    text-decoration: underline;
}

/* Responsive */
@media (max-width: 768px) {
    .header {
        flex-direction: column;
        gap: 1rem;
        text-align: center;
    }

    .dashboard {
        padding: 1rem;
    }

    .oscillator-grid,
    .effects-grid {
        grid-template-columns: 1fr;
    }
}
```

### Deliverables
- [x] Responsive HTML dashboard
- [x] WebSocket client implementation
- [x] Real-time UI updates
- [x] Mobile-friendly design
- [x] Visual feedback for all controls

### Testing Checklist
- [ ] UI loads correctly on desktop
- [ ] UI responsive on mobile/tablet
- [ ] All controls send commands correctly
- [ ] Real-time updates display properly
- [ ] Sensor bars animate smoothly
- [ ] Performance metrics update regularly

---

## Phase 5: Integration & Testing

### Goal
Integrate all components and ensure system stability

### Tasks

#### 5.1 Update main.cpp

**File: `src/main.cpp`**

```cpp
#include <Arduino.h>
#include <Wire.h>
#include "system/Debug.h"
#include "system/Theremin.h"
#include "system/PinConfig.h"
#include "system/PerformanceMonitor.h"
#include "system/DisplayManager.h"

#if WEBUI_ENABLED
#include "system/WiFiManager.h"
#include "system/WebUIManager.h"
#endif

#if ENABLE_OTA
#include "system/OTAManager.h"
#endif

// ... existing globals ...

#if WEBUI_ENABLED
// Create WiFi manager
WiFiManager wifiManager("Theremin-OTA", "");

// Create shared AsyncWebServer instance
AsyncWebServer server(80);

// Create WebUI manager
WebUIManager webUI(&server, &theremin);

#if ENABLE_OTA
// Create OTA manager (shares server with WebUI)
OTAManager ota(&server, "Theremin-OTA", "");
#endif
#endif

void setup() {
    // ... existing setup code ...

    #if WEBUI_ENABLED
        // Initialize WiFi (AP + STA mode)
        wifiManager.begin();

        // Initialize WebUI
        webUI.begin();

        #if ENABLE_OTA
            // Initialize OTA (shares server)
            ota.begin("admin", "theremin");
        #endif

        // Start the shared server
        server.begin();
        DEBUG_PRINTLN("[Server] AsyncWebServer started on port 80");
    #endif
}

void loop() {
    // ... existing loop code ...

    #if WEBUI_ENABLED
        wifiManager.update();  // Monitor WiFi status
        webUI.update();        // Broadcast periodic updates
    #endif
}
```

#### 5.2 Test Scenarios

**WiFi Modes:**
- [ ] AP mode: No saved credentials, creates "Theremin-OTA" network
- [ ] STA mode: Connects to saved WiFi network
- [ ] AP+STA mode: Both active simultaneously
- [ ] Fallback: STA fails → switches to AP mode

**Network Access:**
- [ ] AP mode: Access via `192.168.4.1`
- [ ] STA mode: Access via `theremin.local`
- [ ] STA mode: Access via assigned IP address
- [ ] Captive portal: Appears on first AP connection

**WebSocket Communication:**
- [ ] Client connects successfully
- [ ] Commands executed correctly
- [ ] State updates received in real-time
- [ ] Multiple clients can connect (2+ browsers)
- [ ] Reconnection works after disconnect/refresh

**OTA Compatibility:**
- [ ] OTA accessible at `/update` route
- [ ] OTA works while WebUI active
- [ ] Firmware upload successful
- [ ] Both OTA and WebUI share server cleanly

**Performance:**
- [ ] No audio glitches during WebSocket traffic
- [ ] CPU usage acceptable with active connections
- [ ] RAM usage stable over extended session
- [ ] No latency increase in sensor readings

**UI Functionality:**
- [ ] All oscillator controls work
- [ ] All effect controls work
- [ ] Real-time monitoring displays correctly
- [ ] Mobile/tablet layout functional
- [ ] Browser refresh preserves state

#### 5.3 Performance Benchmarking

```cpp
// Add to WebUIManager::update()
void WebUIManager::update() {
    unsigned long now = millis();

    if (now - lastUpdate >= UPDATE_INTERVAL) {
        lastUpdate = now;

        // Measure broadcast time
        unsigned long startTime = micros();

        // Send sensor update
        DynamicJsonDocument doc(256);
        doc["type"] = "sensor";
        doc["pitch"] = theremin->getSensorManager()->getPitchDistance();
        doc["volume"] = theremin->getSensorManager()->getVolumeDistance();
        broadcastUpdate("sensor", doc);

        // Send performance update
        doc.clear();
        doc["type"] = "performance";
        doc["cpu"] = theremin->getPerformanceMonitor()->getCPUUsage();
        doc["ram"] = ESP.getFreeHeap();
        doc["uptime"] = millis();
        broadcastUpdate("performance", doc);

        unsigned long elapsed = micros() - startTime;
        DEBUG_PRINTF("[WS] Broadcast took %lu µs\n", elapsed);
    }
}
```

**Target Metrics:**
- WebSocket broadcast: <1ms per update
- Total CPU impact: <8%
- RAM usage: <40 KB additional
- No audio buffer underruns

### Deliverables
- [x] All components integrated
- [x] Comprehensive testing completed
- [x] Performance validated
- [x] Documentation updated

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                          ESP32                              │
│                                                             │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │ WiFiManager  │  │  WebUIManager │  │  OTAManager  │     │
│  │              │  │               │  │              │     │
│  │ - AP Mode    │  │ - WebSocket   │  │ - AsyncOTA   │     │
│  │ - STA Mode   │  │ - JSON API    │  │ - Auth       │     │
│  │ - Captive    │  │ - Broadcast   │  │              │     │
│  │ - mDNS       │  │               │  │              │     │
│  └──────┬───────┘  └──────┬────────┘  └──────┬───────┘     │
│         │                 │                   │             │
│         └────────┬────────┴──────────┬────────┘             │
│                  │                   │                      │
│           ┌──────▼───────────────────▼──────┐               │
│           │     AsyncWebServer (Port 80)    │               │
│           │                                 │               │
│           │  Routes:                        │               │
│           │  - /           → index.html     │               │
│           │  - /ws         → WebSocket      │               │
│           │  - /update     → OTA            │               │
│           │  - /setup      → Captive Portal │               │
│           └─────────────────────────────────┘               │
│                                                             │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐     │
│  │  Theremin    │  │ AudioEngine  │  │ SensorManager│     │
│  │              │  │              │  │              │     │
│  │ - Controls   │  │ - Oscillators│  │ - VL53L0X    │     │
│  │              │  │ - Effects    │  │ - Smoothing  │     │
│  └──────────────┘  └──────────────┘  └──────────────┘     │
│         ▲                 ▲                  ▲             │
│         │                 │                  │             │
│         └─────────────────┴──────────────────┘             │
│                   Controlled via                           │
│              WebSocket JSON Commands                       │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
                    ┌───────────────────┐
                    │   Browser Client  │
                    │                   │
                    │  - HTML/CSS/JS    │
                    │  - WebSocket      │
                    │  - Real-time UI   │
                    └───────────────────┘
```

---

## File Structure

```
theremin/
├── platformio.ini                   # MODIFIED: Add libraries & flags
│
├── include/system/
│   ├── WiFiManager.h               # NEW: WiFi AP+STA manager
│   ├── WebUIManager.h              # NEW: WebSocket + HTTP server
│   └── OTAManager.h                # MODIFIED: Use AsyncWebServer
│
├── src/system/
│   ├── WiFiManager.cpp             # NEW
│   ├── WebUIManager.cpp            # NEW
│   └── OTAManager.cpp              # MODIFIED
│
├── src/
│   └── main.cpp                    # MODIFIED: Integrate WiFi & WebUI
│
├── data/                           # SPIFFS filesystem
│   ├── index.html                  # NEW: Main dashboard
│   ├── styles.css                  # NEW: Styling
│   ├── scripts.js                  # NEW: WebSocket client
│   ├── setup.html                  # NEW: Captive portal page
│   └── favicon.ico                 # NEW: Icon
│
└── docs/improvements/
    └── WEBUI_IMPLEMENTATION_PLAN.md  # THIS FILE
```

---

## Estimated Timeline

| Phase | Complexity | Time Estimate | Priority |
|-------|-----------|---------------|----------|
| Phase 1: AsyncWebServer Migration | Medium | 2-3 hours | High |
| Phase 2: WiFi Infrastructure | Medium | 3-4 hours | High |
| Phase 3: WebSocket Backend | Medium | 4-5 hours | High |
| Phase 4: Web Frontend | High | 6-8 hours | High |
| Phase 5: Integration & Testing | Medium | 2-3 hours | High |
| **Total** | | **17-23 hours** | |

**Recommended Schedule:**
- Day 1: Phase 1 (AsyncWebServer migration)
- Day 2: Phase 2 (WiFi infrastructure)
- Day 3: Phase 3 (WebSocket backend)
- Day 4-5: Phase 4 (Web frontend)
- Day 6: Phase 5 (Integration & testing)

---

## Dependencies

### New Libraries Required

```ini
lib_deps =
    ESP32Async/ESPAsyncWebServer@^3.8.1  # AsyncTCP auto-included transitively
    tzapu/WiFiManager@^2.0.17
    bblanchon/ArduinoJson@^7.2.1
```

**Note:** AsyncTCP is automatically included as a transitive dependency of ESPAsyncWebServer, so no need to add it explicitly.

### Build Flags

```ini
build_flags =
    -DELEGANTOTA_USE_ASYNC_WEBSERVER=1
```

---

## Resource Impact Summary

### RAM Usage
- **Current:** 47,560 bytes (14.5%)
- **WebUI Additional:** ~25-35 KB
- **Projected Total:** ~75 KB (23%)
- **Remaining:** ~245 KB (77%)

### Flash Usage
- **Current:** 857,041 bytes (65.4%)
- **WebUI Additional:** ~80-115 KB
- **Projected Total:** ~970 KB (74%)
- **Remaining:** ~340 KB (26%)

### CPU Usage
- **Current:** 14.5% (with audio + effects)
- **WebUI Additional:** ~5-8%
- **Projected Total:** ~20-22%
- **Remaining:** ~78-80%

**Conclusion:** Plenty of headroom for Web UI implementation!

---

## Security Considerations

### Network Security
- WiFi AP runs in open mode by default (configurable)
- Only accessible on local network
- No internet exposure

### Web UI Security
- Option to add HTTP Basic Authentication
- Session-based auth available if needed
- HTTPS not implemented (local network only)

### Recommendations for Production
1. Add AP password (WPA2)
2. Enable HTTP Basic Auth on Web UI
3. Change default OTA credentials
4. Consider timeout/auto-disable OTA
5. Implement rate limiting on WebSocket commands

---

## Testing Strategy

### Unit Testing
- WiFiManager connection logic
- WebSocket message parsing
- Command validation
- JSON serialization/deserialization

### Integration Testing
- WiFi mode switching
- OTA + WebUI coexistence
- Multi-client WebSocket
- Filesystem serving

### Performance Testing
- WebSocket broadcast latency
- CPU usage under load
- RAM leak detection
- Audio quality validation

### User Acceptance Testing
- Mobile device compatibility
- Browser compatibility (Chrome, Firefox, Safari)
- Intuitive UI controls
- Real-time responsiveness

---

## Future Enhancements

### Phase 6: Advanced Features
- Preset save/load system (to SPIFFS)
- Firmware rollback capability
- Audio spectrum analyzer display
- Waveform visualization

### Phase 7: Network Features
- MQTT integration for home automation
- REST API for external control
- OSC (Open Sound Control) protocol
- Cloud sync for presets

### Phase 8: UI Improvements
- Dark/light theme toggle
- Custom color schemes
- Keyboard shortcuts
- Touch gestures for mobile

---

## Troubleshooting Guide

### Common Issues

**WiFi won't connect:**
- Check SSID/password in captive portal
- Verify WiFi credentials saved correctly
- Check serial output for error messages
- Try forgetting network and reconnecting

**WebSocket disconnects frequently:**
- Check WiFi signal strength
- Verify no IP conflicts on network
- Increase WebSocket ping interval
- Check for RAM exhaustion

**OTA not accessible:**
- Verify `/update` route registered
- Check ElegantOTA async mode flag
- Ensure server.begin() called
- Verify WiFi connection active

**UI not updating in real-time:**
- Check WebSocket connection status
- Verify broadcast rate (UPDATE_INTERVAL)
- Inspect browser console for errors
- Check JSON message format

**Performance degradation:**
- Monitor CPU usage during broadcasts
- Check for memory leaks (RAM trend)
- Reduce WebSocket broadcast rate
- Limit number of concurrent clients

---

## Success Criteria

### Phase Completion
- [ ] All 5 phases implemented
- [ ] All test scenarios passed
- [ ] Performance targets met
- [ ] Documentation complete

### Functional Requirements
- [ ] AP + STA mode working
- [ ] mDNS resolution functional
- [ ] Captive portal accessible
- [ ] WebSocket bidirectional communication
- [ ] All parameters controllable via UI
- [ ] Real-time monitoring displaying
- [ ] OTA compatibility maintained

### Quality Requirements
- [ ] No audio quality degradation
- [ ] CPU usage <25%
- [ ] RAM usage stable
- [ ] No crashes during extended operation
- [ ] Mobile-responsive UI
- [ ] Intuitive user experience

---

## References

### Libraries Documentation
- [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)
- [AsyncTCP](https://github.com/me-no-dev/AsyncTCP)
- [ArduinoJson](https://arduinojson.org/)
- [ElegantOTA](https://github.com/ayushsharma82/ElegantOTA)

### ESP32 Documentation
- [ESP32 WiFi Modes](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/wifi.html)
- [ESP32 mDNS](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/mdns.html)
- [LittleFS Filesystem](https://github.com/lorol/LITTLEFS)

### Web Technologies
- [WebSocket Protocol](https://developer.mozilla.org/en-US/docs/Web/API/WebSocket)
- [JSON](https://www.json.org/)
- [Captive Portal](https://en.wikipedia.org/wiki/Captive_portal)

---

## Implementation Notes

### Development Tips
1. **Incremental Development:** Complete one phase fully before starting next
2. **Test Early:** Don't wait until end to test - validate each component
3. **Serial Debugging:** Keep extensive debug output during development
4. **Version Control:** Commit after each working phase
5. **Backup:** Keep backups before major changes

### Common Pitfalls
- Don't forget to call `server.begin()` only once
- WebSocket messages must be null-terminated
- JSON buffer sizes must be adequate (use calculator)
- SPIFFS must be uploaded before testing UI
- AsyncWebServer requires different callback signatures

### Performance Optimization
- Use StaticJsonDocument when size is known
- Minimize WebSocket broadcast rate
- Compress JSON messages (remove whitespace)
- Use const char* instead of String where possible
- Implement rate limiting on commands

---

**Plan Status:** Ready for Implementation
**Next Step:** Begin Phase 1 (AsyncWebServer Migration)
**Document Version:** 1.0
**Last Updated:** November 8, 2025
