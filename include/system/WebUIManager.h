/*
 * WebUIManager.h
 *
 * WebSocket-based real-time control interface for ESP32 Theremin.
 * Provides bidirectional communication between web clients and Theremin system.
 *
 * Features:
 * - WebSocket endpoint at /ws
 * - JSON-based command protocol
 * - Real-time state broadcasting (10 Hz default)
 * - Multiple concurrent client support
 * - Automatic state sync on client connect
 *
 * Usage:
 *   WebUIManager webUI(&server, &theremin);
 *   webUI.begin();
 *
 *   In loop():
 *     webUI.update();  // Broadcasts periodic updates
 */

#pragma once

#include <Arduino.h>

#ifdef ENABLE_NETWORK

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "system/Theremin.h"

class WebUIManager {
 private:
  AsyncWebServer* server;
  AsyncWebSocket ws;
  Theremin* theremin;

  unsigned long lastUpdate;
  static const int UPDATE_INTERVAL = 100;  // 10 Hz broadcast rate (ms)

  // WebSocket event handlers
  static void onWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                                AwsEventType type, void* arg, uint8_t* data, size_t len);

  void handleWebSocketMessage(AsyncWebSocketClient* client, uint8_t* data, size_t len);

  // State broadcasting
  void sendFullState(AsyncWebSocketClient* client);
  void broadcastUpdate(const char* type, JsonDocument& doc);

  // Command handlers
  void handleOscillatorCommand(JsonDocument& doc);
  void handleEffectCommand(JsonDocument& doc);
  void handleSystemCommand(JsonDocument& doc);

  // Helper methods
  void sendOscillatorState(int oscNum, AsyncWebSocketClient* client = nullptr);
  void sendEffectState(const char* effectName, AsyncWebSocketClient* client = nullptr);
  void sendSensorState(AsyncWebSocketClient* client = nullptr);
  void sendPerformanceState(AsyncWebSocketClient* client = nullptr);

 public:
  /**
   * Constructor
   * @param srv Pointer to AsyncWebServer instance (shared with OTA)
   * @param thmn Pointer to Theremin instance for control
   */
  WebUIManager(AsyncWebServer* srv, Theremin* thmn);

  /**
   * Initialize WebSocket endpoint and register with server
   * Must be called before server.begin()
   */
  void begin();

  /**
   * Non-blocking update for periodic state broadcasts
   * Call in loop() for real-time monitoring updates
   */
  void update();

  /**
   * Get number of connected WebSocket clients
   * @return Number of active connections
   */
  size_t getClientCount() const { return ws.count(); }

  /**
   * Check if WebUI is initialized and running
   * @return true if running
   */
  bool isRunning() const { return server != nullptr; }
};

#endif  // ENABLE_NETWORK
